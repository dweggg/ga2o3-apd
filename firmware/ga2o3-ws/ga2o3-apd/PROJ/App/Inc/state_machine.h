/**
 * @file state_machine.h
 * @brief System state machine: manages startup, running, and fault states
 * @author lan
 * @date 2026
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
 */
void InitStateMachine(void);

/**
 * @brief The state machine core task - performs health checks and state transitions
 *        Call every control period or faster. Safety checks are run unconditionally
 *        on every cycle to catch faults immediately.
 */
void TaskStateMachine(void);

/**
 * @brief Get current state machine state (for debugging/monitoring)
 * @return Current state machine state
 */
StateMachineTypeDef GetStateMachineState(void);

#ifdef __cplusplus
}
#endif

