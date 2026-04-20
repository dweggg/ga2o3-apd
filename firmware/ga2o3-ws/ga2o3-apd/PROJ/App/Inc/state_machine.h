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
