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
#include "adc_config.h"
#include "global_defines.h"

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

StateMachineTypeDef state_machine_handle;

/** Shadows control_params.interleaved_mode_switch so we can detect a change. */
static uint16_t s_interleaved_shadow = 0U;

/* -------------------------------------------------------------------------- */
/* Private helpers                                                             */
/* -------------------------------------------------------------------------- */

static inline uint16_t OverCurrentCheck(void)
{
    float32_t ia = GetCurrentA();
    float32_t ib = GetCurrentB();
    float32_t ic = GetCurrentC();
    return (ia < MAX_PHASE_CURRENT_AMPS || ia > MAX_PHASE_CURRENT_AMPS || 
            ib < MAX_PHASE_CURRENT_AMPS || ib > MAX_PHASE_CURRENT_AMPS || 
            ic < MAX_PHASE_CURRENT_AMPS || ic > MAX_PHASE_CURRENT_AMPS) ? 1U : 0U;
}

static inline void DisableDrivers(void)
{
    GPIO_writePin(25, 0);
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
            InitControlLoop();
            if (ADC_Config_Init() != HAL_OK)
            {
                state_machine_handle = STATE_ERROR;
                break;
            }
            state_machine_handle = STATE_IDLE;
            break;

        /* ------------------------------------------------------------------ */
        case STATE_IDLE:
            if (ControlLoop_IsEnabled())
            {
                ControlLoop_Enable();
                state_machine_handle = STATE_RUNNING;
            }
            break;

        /* ------------------------------------------------------------------ */
        case STATE_RUNNING:
            if (!OverCurrentCheck())
            {
                ControlLoop_Disable();
                DisableDrivers();
                state_machine_handle = STATE_OVER_CURRENT;
                break;
            }

            if (0/*CheckAndClearReset()*/)
            {
                ControlLoop_Disable();
                InitControlLoop();
                ControlLoop_Enable();
            }
            break;

        /* ------------------------------------------------------------------ */
        case STATE_DISCHARGING:
            /* TODO: wait for bus voltage to fall below a safe threshold.
             * For now, immediately re-initialise with the new mode setting. */
            ControlLoop_SetInterleavedMode(s_interleaved_shadow);
            InitControlLoop();
            ControlLoop_Enable();
            state_machine_handle = STATE_RUNNING;
            break;

        /* ------------------------------------------------------------------ */
        case STATE_STOP:
            ControlLoop_Disable();
            DisableDrivers();
            break;

        /* ------------------------------------------------------------------ */
        case STATE_OVER_CURRENT:
            /* Latched fault - hardware must be re-enabled externally. */
            break;

        /* ------------------------------------------------------------------ */
        case STATE_ERROR:
            ControlLoop_Disable();
            DisableDrivers();
            break;

        /* ------------------------------------------------------------------ */
        default:
            break;
    }
}
