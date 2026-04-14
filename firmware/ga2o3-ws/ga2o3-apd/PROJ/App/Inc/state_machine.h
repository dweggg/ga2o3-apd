/**
 * @file state_machine.h
 * @author lan
 * @brief state machine
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"
#include "control_math.h"

typedef enum
{
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_RUNNING,
    STATE_DISCHARGING,
    STATE_STOP,
    STATE_ERROR,
    STATE_OVER_CURRENT,
} StateMachineTypeDef;

typedef struct
{
    uint16_t interleaved_mode_switch; //0 => non-interleaved, else interleaved
    uint16_t output_enabled; //0 => disabled, else enabled
    uint16_t current_control_enabled; //0 => disabled, else enabled
    uint16_t reset;//if reset != 0 detected(modified by MCUViewer), do reset and restart
    float32_t ts_us;
    float32_t current_feedback_amps;
    float32_t k_current_feedback;
    float32_t omega_rad;
    AlphaBetaTypeDef current_ab_amps;
    AlphaBetaTypeDef output_ab;
    float32_t sin_omega;
    float32_t cos_omega;
    float32_t output_duty;
    SogiTypeDef current_sogi;
    DqTypeDef idq_meas_amps;
    DqTypeDef idq_ref_amps;
    DqTypeDef idq_output_amps;
    AngleGenTypeDef angle_generation;
    PiTypeDef pi_id;
    PiTypeDef pi_iq;
    PolarTypeDef pi_output_polar;
    CartTypeDef pi_output_cart;

} ControlParamsTypeDef;

extern ControlParamsTypeDef control_params;

/**
 * @brief Initialize the state machine
 * 
 * @return none
 */
void InitStateMachine(void);

/**
 * @brief The state machine core, needs to be executed every control period or faster
 * 
 *@return none
 */
void TaskStateMachine(void);

void SetIdRef(float32_t ref);
void SetIqRef(float32_t ref);
void SetOpenLoopVoltage(float32_t voltage_rms, float32_t freq);
void EnableCurrentContrl(void);
void EnableCurrentContrl(void);


#ifdef __cplusplus
}
#endif
