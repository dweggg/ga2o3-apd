/**
 * @file  state_machine.c
 * @brief System supervisory layer: startup sequencing, fault detection, and
 *        mode transitions.
 *
 *        This module does NOT implement the control algorithm - it only
 *        decides *when* the control loop should run and *which mode* it
 *        should use. All safety checks and hardware control are delegated
 *        to the safety module.
 */

#include "state_machine.h"
#include "control_loop.h"
#include "safety.h"
#include "global_defines.h"
#include "user_interface.h"

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

static StateMachineTypeDef state_machine_handle = STATE_INIT;

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

void InitStateMachine(void)
{
    state_machine_handle = STATE_INIT;
}

#pragma CODE_SECTION(TaskStateMachine, ".TI.ramfunc")
void TaskStateMachine(void)
{
    // Check system health on every cycle (latches faults)
    TaskCheckSystemHealth();

    switch (state_machine_handle)
    {
        /* ------------------------------------------------------------------ */
        case STATE_INIT:
            state_machine_handle = STATE_IDLE;
            break;

        /* ------------------------------------------------------------------ */
        case STATE_IDLE:
            // System waiting for enable signal
            // When enabled, transition to RUNNING
            if (GetUiSystemEnabled())
            {
                ControlLoop_Enable();
                state_machine_handle = STATE_RUNNING;
            }
            break;

        /* ------------------------------------------------------------------ */
        case STATE_RUNNING:
            // System actively controlling. Check for disable or fault.
            if (!GetUiSystemEnabled())
            {
                ControlLoop_Disable();
                state_machine_handle = STATE_DISCHARGING;
                break;
            }

            // If a fault occurred, transition to error state
            if (IsFaulted())
            {
                state_machine_handle = STATE_ERROR;
                break;
            }

            break;

        /* ------------------------------------------------------------------ */
        case STATE_DISCHARGING:
            /* TODO: wait for bus voltage to fall below a safe threshold. */
            state_machine_handle = STATE_IDLE;
            break;

        /* ------------------------------------------------------------------ */
        case STATE_ERROR:
            // Latched fault - system must be manually reset/re-enabled
            ControlLoop_Disable();
            break;

        /* ------------------------------------------------------------------ */
        case STATE_STOP:
            ControlLoop_Disable();
            break;

        /* ------------------------------------------------------------------ */
        default:
            break;
    }
}

//@brief Get current state (for debugging/monitoring)
//@return Current state machine state
StateMachineTypeDef GetStateMachineState(void)
{
    return state_machine_handle;
}
