#include "state_machine.h"
#include "global_defines.h"
#include "adc_config.h"

StateMachineTypeDef  state_machine_handle;
uint16_t __inner_interleaved_mode_switch = 0;

#pragma CODE_SECTION(TaskStateMachine, ".TI.ramfunc");

void InitStateMachine(void)
{
    state_machine_handle = STATE_INIT;
}

static inline void CalculateAndInitPI(PiTypeDef *pid_var, float32_t L1, float32_t R1, float32_t tr, float32_t ts)
{
    float32_t _kp = L1 * log(9.0f) / tr;
    float32_t _ki = R1 * log(9.0f) / tr;
    float32_t _limit = 1000.0f; // need to revise
    InitPiControl(pid_var, _kp, _ki, _limit, ts);
}

static inline void InitBlocks(void)
{
    control_params.ts_us = TS_US;
    control_params.interleaved_mode_switch = 0; //default: non-interleaved
    control_params.output_enabled = 0; //default: output disabled
    control_params.current_feedback_amps = 0.0f;
    control_params.k_current_feedback = CURRENT_FB_GAIN;
    control_params.omega_rad = 0.0f;
    control_params.sin_omega = 0.0f;
    control_params.cos_omega = 0.0f;
    control_params.idq_meas_amps.d = 0.0f;
    control_params.idq_meas_amps.q = 0.0f;
    control_params.idq_ref_amps.d = ID_REF_AMPS;
    control_params.idq_ref_amps.q = IQ_REF_AMPS;
    control_params.pi_output_cart.x = 0.0f;
    control_params.pi_output_cart.y = 0.0f;
    control_params.pi_output_polar.r = 0.0f;
    control_params.pi_output_polar.theta = 0.0f;
    control_params.reset = 0;

    CalculateAndInitPI(&control_params.pi_id, L1_UH, R1_OHM, TR, TS_US);
    CalculateAndInitPI(&control_params.pi_iq, L1_UH, R1_OHM, TR, TS_US);

    InitAngleGen(&control_params.angle_generation, AC_FREQ, TS_US);
    InitSogi(&control_params.current_sogi, SOGI_GAIN, TS_US);
}

static inline uint16_t OverCurrentCheck(void)
{
    float32_t _ia = GetCurrentA();
    float32_t _ib = GetCurrentB();
    float32_t _ic = GetCurrentC();
    if(_ia > MAX_PHASE_CURRENT_AMPS || _ib > MAX_PHASE_CURRENT_AMPS || _ic > MAX_PHASE_CURRENT_AMPS)
    {
        return false;
    }
    return true;
}

static inline void Clamp(PolarTypeDef *polar, float32_t limit)
{
    if(polar->r > limit)
    {
        polar->r = limit;
    }
    if(polar->r < -limit)
    {
        polar->r = -limit;
    }
}

static inline void DisableDrivers(void)
{
    GPIO_writePin(25, 0);
}

static inline uint16_t CheckResetEnabled(void)
{
    if(control_params.reset)
    {
        control_params.reset = 0;
        return 1;
    }
    return 0;
}

void TaskStateMachine(void)
{
    switch (state_machine_handle) 
    {
        case STATE_INIT:
            InitBlocks();
            HAL_StatusTypeDef fb = ADC_Config_Init();
            if(fb != HAL_OK)
            {
                state_machine_handle = STATE_ERROR;
            }
            state_machine_handle = STATE_IDLE;
            break;
        case STATE_IDLE:
            if(control_params.output_enabled)
            {
                state_machine_handle = STATE_RUNNING;
            }
            break;
        case STATE_RUNNING:
            GenerateAngle(&control_params.angle_generation);
            if(!OverCurrentCheck())
            {
                state_machine_handle = STATE_OVER_CURRENT;
                break;
            }
            if(control_params.interleaved_mode_switch != __inner_interleaved_mode_switch) // if this variable chenged by outside
            {
                state_machine_handle = STATE_DISCHARGING;//we need to restart whole thing
                __inner_interleaved_mode_switch = control_params.interleaved_mode_switch;
                break;
            }
            if(__inner_interleaved_mode_switch)
            {
                //interleaved
            }
            else
            {
                //non-interleaved
            }
            //Run sogi
            float32_t i_fb = GetCurrentC();
            RunSogi(&control_params.current_sogi, i_fb, control_params.angle_generation.omega);
            control_params.current_ab_amps.alpha = control_params.current_sogi.alpha;
            control_params.current_ab_amps.beta = control_params.current_sogi.beta;
            //Run AB => DQ
            control_params.idq_meas_amps = ConvertAlphabetaToDq(control_params.current_ab_amps, control_params.angle_generation.theta);
            RunPiControl(&control_params.pi_id, control_params.idq_ref_amps.d, control_params.idq_meas_amps.d);
            RunPiControl(&control_params.pi_iq, control_params.idq_ref_amps.q, control_params.idq_meas_amps.q);
            //Convert to polar
            control_params.pi_output_cart.x = control_params.pi_id.output; //D axis aligned with x
            control_params.pi_output_cart.y = control_params.pi_iq.output;
            control_params.pi_output_polar = CartesianToPolar(control_params.pi_output_cart);
            Clamp(&control_params.pi_output_polar, VDC / 2);
            //Convert back to xy
            control_params.pi_output_cart = PolarToCartesian(control_params.pi_output_polar);
            control_params.idq_output_amps.d = control_params.pi_output_cart.x;
            control_params.idq_output_amps.q = control_params.pi_output_cart.y;
            //DQ to AB
            control_params.output_ab = ConvertDqToAlphabeta(control_params.idq_output_amps, control_params.angle_generation.theta);
            //normalization
            control_params.output_duty = control_params.output_ab.alpha / (VDC / 2);
            //SetDuty(3, control_params.output_duty);
            break;
        case STATE_DISCHARGING:
            //DisablePWM(1);
            break;
        case STATE_STOP:
            break;
        case STATE_ERROR:
            break;
        case STATE_OVER_CURRENT:
            break;
        default:
            break;
    }
}
