/**
 * @file  control_loop.c
 * @brief Signal-processing chain: SOGI -> alpha-beta -> dq -> PI -> voltage clamp -> duty.
 *
 *        This module owns all algorithm state.  It has no knowledge of the
 *        system state machine or the batch test harness; it only exposes a
 *        clean enable/disable + reference API that those layers call into.
 */

#include "control_loop.h"
#include "global_defines.h"
#include "adc_config.h"
#include "control_math.h"
#include "task_scheduler.h"

/* -------------------------------------------------------------------------- */
/* Rate limiter slew rates                                                     */
/* -------------------------------------------------------------------------- */

static const float RL_RATE_ID_A_PER_S      = 1000.0f;   // d-axis current [A/s]
static const float RL_RATE_IQ_A_PER_S      = 1000.0f;   // q-axis current [A/s]
static const float RL_RATE_VOLTAGE_V_PER_S = 500.0f;    // open-loop voltage [V/s]
static const float RL_RATE_OMEGA_R_PER_S   = 1000.0f;   // open-loop freq [rad/s/s]

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

static uint16_t control_enabled     = 0U;
static uint16_t control_interleaved = 0U;
static uint16_t control_buck = 0U;

static ControlParamsTypeDef control_params;
/* -------------------------------------------------------------------------- */
/* Private helpers                                                             */
/* -------------------------------------------------------------------------- */

static void CalculateAndInitPI(PiTypeDef *pi_var, float L, float R, float tr_s, float ts_s)
{
    float kp = L * logf(9.0f) / tr_s;
    float ki = R * logf(9.0f) / tr_s;
    InitPiControl(pi_var, kp, ki, ts_s);
}

static inline void Clamp(PolarTypeDef *polar, float limit)
{
    if (polar->r >  limit) { polar->r =  limit; }
    if (polar->r < -limit) { polar->r = -limit; }
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

void InitControlLoop(void)
{
    control_params.sampling_time          = (float)GetTaskPeriod(TaskControlLoopDC);
    control_params.current_feedback_amps  = 0.0f;
    control_params.omega_rad              = 0.0f;
    control_params.sin_theta              = 0.0f;
    control_params.cos_theta              = 0.0f;
    control_params.idq_meas_amps.d        = 0.0f;
    control_params.idq_meas_amps.q        = 0.0f;
    control_params.idq_ref_amps.d         = 0.0f;
    control_params.idq_ref_amps.q         = 0.0f;
    control_params.pi_output_dq.x         = 0.0f;
    control_params.pi_output_dq.y         = 0.0f;
    control_params.pi_output_vs_delta.r   = 0.0f;
    control_params.pi_output_vs_delta.theta = 0.0f;

    CalculateAndInitPI(&control_params.pi_id, L_H, R_OHM, 0.01f, control_params.sampling_time);
    CalculateAndInitPI(&control_params.pi_iq, L_H, R_OHM, 0.01f, control_params.sampling_time);

    InitAngleGen(&control_params.angle_generation, 50.0f, control_params.sampling_time);
    InitSogi(&control_params.current_sogi, 1.0f, control_params.sampling_time);

    // Rate limiters — all start from zero, consistent with the zeroed refs above
    float ts = control_params.sampling_time;
    InitRateLimiter(&control_params.rl_id,         0.0f, RL_RATE_ID_A_PER_S,      ts);
    InitRateLimiter(&control_params.rl_iq,         0.0f, RL_RATE_IQ_A_PER_S,      ts);
    InitRateLimiter(&control_params.rl_voltage_pk, 0.0f, RL_RATE_VOLTAGE_V_PER_S, ts);
    InitRateLimiter(&control_params.rl_omega,      0.0f, RL_RATE_OMEGA_R_PER_S,   ts);

    control_enabled     = 0U;
    control_interleaved = 0U;
}

void     ControlLoop_Enable(void)                        { control_enabled     = 1U; }
void     ControlLoop_Disable(void)                       { control_enabled     = 0U; }
uint16_t ControlLoop_IsEnabled(void)                     { return control_enabled;   }

// Setters store the *target*; the rate limiter in TaskControlLoop does the ramping
void     ControlLoop_SetIdRef(float id_amps)             { control_params.idq_ref_amps.d        = id_amps;              }
void     ControlLoop_SetIqRef(float iq_amps)             { control_params.idq_ref_amps.q        = iq_amps;              }
void     ControlLoop_SetOpenLoopVoltage(float voltage, float fundamental_frequency)
{
    control_params.voltage_open_loop_pk = voltage;
    control_params.omega_rad            = TWO_PI * fundamental_frequency;
}
void     ControlLoop_SetInterleavedMode(uint16_t enabled){ control_interleaved = enabled; }
void     ControlLoop_SetBuckMode(uint16_t enabled){ control_buck = enabled; }

/* -------------------------------------------------------------------------- */
/* Scheduler task                                                              */
/* -------------------------------------------------------------------------- */

#pragma CODE_SECTION(TaskControlLoop, ".TI.ramfunc")
void TaskControlLoop(void)
{
    if (!control_enabled) { return; }
    
    if (!control_buck) {

        /* -------------------------------------------------------------------------- */
        /* Power Cycling!                                                             */
        /* -------------------------------------------------------------------------- */


        /* --- Rate-limit all external references -------------------------------- */
        float id_ref      = RunRateLimiter(&control_params.rl_id,         control_params.idq_ref_amps.d);
        float iq_ref      = RunRateLimiter(&control_params.rl_iq,         control_params.idq_ref_amps.q);
        float voltage_pk  = RunRateLimiter(&control_params.rl_voltage_pk, control_params.voltage_open_loop_pk);
        float omega       = RunRateLimiter(&control_params.rl_omega,      control_params.omega_rad);

        /* --- Angle generation ------------------------------------------------- */
        control_params.angle_generation.omega = omega;
        GenerateAngle(&control_params.angle_generation);

        control_params.cos_theta = cosf(control_params.angle_generation.theta);
        control_params.sin_theta = sinf(control_params.angle_generation.theta);

        /* --- Open loop voltage ------------------------------------------------- */

        float v_dc_half = GetVoltageDC() * 0.5f;

        control_params.voltage_open_loop_ac = voltage_pk * control_params.sin_theta;
        float v_ol = control_params.voltage_open_loop_ac / (v_dc_half);
        control_params.duty_open_loop = v_ol < 0.0f ? 0.0f : (v_ol > 1.0f ? 1.0f : v_ol);

        /* --- SOGI: single-phase current -> alpha-beta -------------------------- */
        float i_fb = GetCurrentC();
        RunSogi(&control_params.current_sogi, i_fb, omega);

        control_params.current_ab_amps.alpha = control_params.current_sogi.alpha;
        control_params.current_ab_amps.beta  = control_params.current_sogi.beta;

        /* --- alpha-beta -> dq -------------------------------------------------- */
        control_params.idq_meas_amps = ConvertAlphabetaToDq(
            control_params.current_ab_amps,
            control_params.angle_generation.theta);

        /* --- PI controllers --------------------------------------------------- */
        RunPiControl(&control_params.pi_id,
                    id_ref,
                    control_params.idq_meas_amps.d,
                    v_dc_half, - v_dc_half);

        RunPiControl(&control_params.pi_iq,
                    iq_ref,
                    control_params.idq_meas_amps.q,
                    v_dc_half, - v_dc_half);

        /* --- Cartesian -> polar -> clamp -> back to cartesian ------------------ */
        control_params.pi_output_dq.x = control_params.pi_id.output;
        control_params.pi_output_dq.y = control_params.pi_iq.output;

        control_params.pi_output_vs_delta = CartesianToPolar(control_params.pi_output_dq);
        Clamp(&control_params.pi_output_vs_delta, v_dc_half);
        control_params.pi_output_dq       = PolarToCartesian(control_params.pi_output_vs_delta);

        control_params.pi_output_dq_sat.d = control_params.pi_output_dq.x;
        control_params.pi_output_dq_sat.q = control_params.pi_output_dq.y;

        /* --- dq -> alpha-beta -> normalised duty cycle ------------------------- */
        control_params.voltage_ab = ConvertDqToAlphabeta(control_params.pi_output_dq_sat, control_params.angle_generation.theta);
        float v_cl = control_params.voltage_ab.alpha / (v_dc_half);
        control_params.duty_closed_loop = v_cl < 0.0f ? 0.0f : (v_cl > 1.0f ? 1.0f : v_cl);

        /* --- Mode-specific PWM modulation -------------------------------------- */
        if (control_interleaved)
        {
            SetPhaseShift(PWM_CHANNEL_A, PWM_CHANNEL_B, 0.5F);   // 180 deg
            SetDuty(PWM_CHANNEL_A, control_params.duty_closed_loop);
            SetDuty(PWM_CHANNEL_B, control_params.duty_closed_loop);
            SetDuty(PWM_CHANNEL_C, control_params.duty_open_loop);
        }
        else
        {
            SetDuty(PWM_CHANNEL_A, control_params.duty_closed_loop);
            SetDuty(PWM_CHANNEL_C, control_params.duty_open_loop);
        }

    } else {

        /* -------------------------------------------------------------------------- */
        /* Current Controlled Buck                                                    */
        /* -------------------------------------------------------------------------- */


        float id_ref = control_params.idq_ref_amps.d;

        control_params.idq_meas_amps.d =  GetCurrentC();

        /* --- PI controller --------------------------------------------------- */
        RunPiControl(&control_params.pi_id,
                    id_ref,
                    control_params.idq_meas_amps.d,
                    GetVoltageDC() * 0.9f, 0.0f);



        float v_cl = control_params.pi_id.output / (GetVoltageDC());
        control_params.duty_closed_loop = v_cl < 0.0f ? 0.0f : (v_cl > 1.0f ? 1.0f : v_cl);

        /* --- Single channel output -------------------------------------- */   
        SetDuty(PWM_CHANNEL_C, control_params.duty_closed_loop);

    }
}




/* -------------------------------------------------------------------------- */
/* DC current control task                                                 */
/* -------------------------------------------------------------------------- */

void TaskControlLoopDC(void)
{
    if (!control_enabled) { return; }

}
