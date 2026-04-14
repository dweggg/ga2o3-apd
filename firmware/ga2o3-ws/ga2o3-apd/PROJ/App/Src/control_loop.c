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

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

static uint16_t control_enabled     = 0U;
static uint16_t control_interleaved = 0U;

/* -------------------------------------------------------------------------- */
/* Private helpers                                                             */
/* -------------------------------------------------------------------------- */
static void CalculateAndInitPI(PiTypeDef *pi_var, float L, float R, float tr_s, float ts_s)
{
    float kp     = L  * logf(9.0f) / tr_s;
    float ki     = R * logf(9.0f) / tr_s;
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
    control_params.sampling_time          = 111111;
    control_params.current_feedback_amps  = 0.0f;
    control_params.omega_rad              = 0.0f;
    control_params.sin_theta              = 0.0f;
    control_params.cos_theta              = 0.0f;
    control_params.idq_meas_amps.d        = 0.0f;
    control_params.idq_meas_amps.q        = 0.0f;
    control_params.idq_ref_amps.d         = 0.0f;
    control_params.idq_ref_amps.q         = 0.0f;
    control_params.pi_output_dq.x       = 0.0f;
    control_params.pi_output_dq.y       = 0.0f;
    control_params.pi_output_vs_delta.r      = 0.0f;
    control_params.pi_output_vs_delta.theta  = 0.0f;
    control_params.reset                  = 0;

    CalculateAndInitPI(&control_params.pi_id, L_H, R_OHM, 0.01f, control_params.sampling_time);
    CalculateAndInitPI(&control_params.pi_iq, L_H, R_OHM, 0.01f, control_params.sampling_time);

    InitAngleGen(&control_params.angle_generation, 50.0f, control_params.sampling_time);
    InitSogi(&control_params.current_sogi, 1.0f, control_params.sampling_time);

    control_enabled     = 0U;
    control_interleaved = 0U;
}

void     ControlLoop_Enable(void)                        { control_enabled     = 1U; }
void     ControlLoop_Disable(void)                       { control_enabled     = 0U; }
uint16_t ControlLoop_IsEnabled(void)                     { return control_enabled;   }
void     ControlLoop_SetIdRef(float id_amps)         { control_params.idq_ref_amps.d = id_amps; }
void     ControlLoop_SetIqRef(float iq_amps)         { control_params.idq_ref_amps.q = iq_amps; }
void     ControlLoop_SetInterleavedMode(uint16_t enabled){ control_interleaved = enabled; }

/* -------------------------------------------------------------------------- */
/* Scheduler task                                                              */
/* -------------------------------------------------------------------------- */

#pragma CODE_SECTION(TaskControlLoop, ".TI.ramfunc")
void TaskControlLoop(void)
{
    if (!control_enabled) { return; }

    /* --- Angle generation ------------------------------------------------- */
    GenerateAngle(&control_params.angle_generation);

    /* --- Mode-specific PWM modulation placeholder ------------------------- */
    if (control_interleaved)
    {
        /* interleaved: phase-shift carrier between channels */
    }
    else
    {
        /* non-interleaved: single carrier */
    }

    /* --- SOGI: single-phase current -> alpha-beta ---------------------------------- */
    float i_fb = GetCurrentC();
    RunSogi(&control_params.current_sogi,
             i_fb,
             control_params.angle_generation.omega);

    control_params.current_ab_amps.alpha = control_params.current_sogi.alpha;
    control_params.current_ab_amps.beta  = control_params.current_sogi.beta;

    /* --- alpha-beta -> dq ---------------------------------------------------------- */
    control_params.idq_meas_amps = ConvertAlphabetaToDq(
        control_params.current_ab_amps,
        control_params.angle_generation.theta);

    /* --- PI controllers --------------------------------------------------- */
    RunPiControl(&control_params.pi_id,
                  control_params.idq_ref_amps.d,
                  control_params.idq_meas_amps.d,
                  GetVoltageDC() * 0.5f);

    RunPiControl(&control_params.pi_iq,
                  control_params.idq_ref_amps.q,
                  control_params.idq_meas_amps.q,
                  GetVoltageDC() * 0.5f);

    /* --- Cartesian -> polar -> clamp -> back to cartesian ------------------- */
    control_params.pi_output_dq.x = control_params.pi_id.output;
    control_params.pi_output_dq.y = control_params.pi_iq.output;

    control_params.pi_output_vs_delta  = CartesianToPolar(control_params.pi_output_dq);
    Clamp(&control_params.pi_output_vs_delta, GetVoltageDC() * 0.5f);
    control_params.pi_output_dq   = PolarToCartesian(control_params.pi_output_vs_delta);

    control_params.pi_output_dq_sat.d = control_params.pi_output_dq.x;
    control_params.pi_output_dq_sat.q = control_params.pi_output_dq.y;

    /* --- dq -> alpha-beta -> normalised duty cycle --------------------------------- */
    control_params.voltage_ab = ConvertDqToAlphabeta(control_params.pi_output_dq_sat, control_params.angle_generation.theta);

    control_params.output_duty = control_params.voltage_ab.alpha / (GetVoltageDC() * 0.5f);
    /* SetDuty(CHANNEL_A, control_params.output_duty); */
}