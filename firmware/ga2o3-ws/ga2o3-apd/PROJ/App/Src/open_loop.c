
/**
  ******************************************************************************
  * @file    open_loop.c
  * @author  Arnold
  * @version V1.0
  * @date    2024-06-01
  * @brief   Open loop control implementation.
  ******************************************************************************
  * @attention
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "open_loop.h"
#include "global_defines.h"
#include "adc_config.h"
#include "control_math.h"
#include "task_scheduler.h"

static OpenLoopTypeDef openloop_params;

/* -------------------------------------------------------------------------- */
/* Rate limiter slew rates                                                     */
/* -------------------------------------------------------------------------- */
static const float RL_RATE_OMEGA_R_PER_S   = 1000.0f;   // open-loop freq [rad/s/s]
static const float RL_RATE_VOLTAGE_V_PER_S = 500.0f;    // open-loop voltage [V/s]



void     OpenLoop_Enable(void){openloop_params.openloop_enabled = 1U;}
void     OpenLoop_Disable(void){openloop_params.openloop_enabled = 0U;}






void     OpenLoop_SetPeakVoltage(float voltage, float frequency)
{
    openloop_params.voltage_open_loop_pk = voltage;
    openloop_params.omega_rad = TWO_PI * frequency;
}

void     InitOpenLoop(void)
{
    openloop_params.sampling_time  = 0.0001;
    openloop_params.output_enabled = 0U;
    openloop_params.openloop_enabled = 0U;

    openloop_params.voltage_open_loop_ac = 0.0f;
    openloop_params.voltage_open_loop_pk = 0.0f;

    openloop_params.frequency_hz = 0.0f;
    openloop_params.omega_rad = 0.0f;
    openloop_params.sin_theta = 0.0f;
    openloop_params.cos_theta = 0.0f;

    
    InitRateLimiter(&openloop_params.rl_omega, 0.0f, RL_RATE_OMEGA_R_PER_S,  openloop_params.sampling_time);
    InitRateLimiter(&openloop_params.rl_voltage_pk, 0.0f, RL_RATE_VOLTAGE_V_PER_S, openloop_params.sampling_time);

    InitAngleGen(&openloop_params.angle_generation, 50.0f, openloop_params.sampling_time);

    InitSogi(&openloop_params.current_sogi, 1.0f, openloop_params.sampling_time);
}

void     TaskOpenLoop(void)
{
    if (!openloop_params.openloop_enabled) { return; }

    /* --- Open loop voltage ------------------------------------------------- */

    float omega       = RunRateLimiter(&openloop_params.rl_omega,      openloop_params.omega_rad);
    float voltage_pk  = RunRateLimiter(&openloop_params.rl_voltage_pk, openloop_params.voltage_open_loop_pk);
    
    openloop_params.angle_generation.omega = omega;
    GenerateAngle(&openloop_params.angle_generation);

    openloop_params.cos_theta = cosf(openloop_params.angle_generation.theta);
    openloop_params.sin_theta = sinf(openloop_params.angle_generation.theta);


    float v_dc_half = GetVoltageDC() * 0.5f;
    
    
    openloop_params.voltage_open_loop_ac = voltage_pk * openloop_params.cos_theta * 0.5f + v_dc_half; // we take cos(theta) so that d produces active. in before it was sin(theta) but that was inconsistent with the transforms and was there by pure intuition and no actual reason.

    openloop_params.v_ol = openloop_params.voltage_open_loop_ac / (v_dc_half * 2.0f);
        // v_ol is between +vdc/2 and -vdc/2, so dividing by v_dc gives a duty between -0.5 to 0.5, thats why we add 0.5, to turn it into 0 to 1

    openloop_params.duty_open_loop = openloop_params.v_ol < 0.0f ? 0.0f : (openloop_params.v_ol > 1.0f ? 1.0f : openloop_params.v_ol);

    openloop_params.current =  GetCurrentC();

    RunSogi(&openloop_params.current_sogi, openloop_params.current, openloop_params.angle_generation.omega);

    openloop_params.current_ab_amps.alpha = openloop_params.current_sogi.alpha;
    openloop_params.current_ab_amps.beta  = openloop_params.current_sogi.beta;
    
    /* --- alpha-beta -> dq -------------------------------------------------- */
    openloop_params.idq_meas_amps = ConvertAlphabetaToDq(openloop_params.current_ab_amps,openloop_params.angle_generation.theta);
    
    
    SetDuty(PWM_CHANNEL_C, openloop_params.duty_open_loop);
    SetDuty(PWM_CHANNEL_A, (1-openloop_params.duty_open_loop));
}
