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
    STATE_ERROR,
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

#ifdef __cplusplus
}
#endif
