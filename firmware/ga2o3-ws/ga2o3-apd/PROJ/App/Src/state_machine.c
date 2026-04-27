/**
 * @file  state_machine.c
 * @brief System supervisory layer: startup sequencing, fault detection, and
 *        mode transitions.
 *
 *        This module does NOT implement the control algorithm - it only
 *        decides *when* the control loop should run and *which mode* it
 *        should use, delegating everything else to control_loop.c.
 */

#include "state_machine.h"
#include "control_loop.h"
#include "user_interface.h"
#include "adc_config.h"
#include "global_defines.h"

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

StateMachineTypeDef state_machine_handle;

/* -------------------------------------------------------------------------- */
/* Private helpers                                                             */
/* -------------------------------------------------------------------------- */

static inline uint16_t OverCurrentCheck(void)
{
    float ia = GetCurrentA();
    float ib = GetCurrentB();
    float ic = GetCurrentC();
    return (ia < MAX_PHASE_CURRENT_AMPS || ia > MAX_PHASE_CURRENT_AMPS || 
            ib < MAX_PHASE_CURRENT_AMPS || ib > MAX_PHASE_CURRENT_AMPS || 
            ic < MAX_PHASE_CURRENT_AMPS || ic > MAX_PHASE_CURRENT_AMPS) ? 1U : 0U;
}


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
    switch (state_machine_handle)
    {
        /* ------------------------------------------------------------------ */
        case STATE_INIT:
            state_machine_handle = STATE_IDLE;
            break;

        /* ------------------------------------------------------------------ */
        case STATE_IDLE:
            if (g_ui.system_enabled)
            {
                ControlLoop_Enable();
                state_machine_handle = STATE_RUNNING;
            }
            break;

        /* ------------------------------------------------------------------ */
        case STATE_RUNNING:
            if (!g_ui.system_enabled)
            {
                ControlLoop_Disable();
                state_machine_handle = STATE_DISCHARGING;
                break;
            }

            // if (!OverCurrentCheck())
            // {
            //     ControlLoop_Disable();
            //     DisableDrivers();
            //     state_machine_handle = STATE_OVER_CURRENT;
            //     break;
            // }

            if (0/*CheckAndClearReset()*/)
            {
                ControlLoop_Disable();
                InitControlLoop();
                ControlLoop_Enable();
            }
            break;

        /* ------------------------------------------------------------------ */
        case STATE_DISCHARGING:
            /* TODO: wait for bus voltage to fall below a safe threshold. */
            state_machine_handle = STATE_IDLE;
            break;

        /* ------------------------------------------------------------------ */
        case STATE_STOP:
            ControlLoop_Disable();
            break;

        /* ------------------------------------------------------------------ */
        case STATE_OVER_CURRENT:
            /* Latched fault - hardware must be re-enabled externally. */
            break;

        /* ------------------------------------------------------------------ */
        case STATE_ERROR:
            ControlLoop_Disable();
            break;

        /* ------------------------------------------------------------------ */
        default:
            break;
    }
}
