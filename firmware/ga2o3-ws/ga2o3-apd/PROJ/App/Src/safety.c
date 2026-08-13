/**
 * @file safety.c
 * @brief Safety module: health checks, fault protection, and safe shutdown
 * @author lan
 * @date 2026
 */

#include "safety.h"
#include "adc_config.h"
#include "bsp_epwm.h"
#include "user_interface.h"
#include "control_loop.h"
#include "global_defines.h"

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

static ErrorStatusTypeDef safety_error_state = ERRNO_NO_ERR;

/* -------------------------------------------------------------------------- */
/* Private helpers - Health checks                                             */
/* -------------------------------------------------------------------------- */

//@brief Check for overcurrent condition across all phases
//@return 1 if overcurrent detected, 0 otherwise
static uint16_t CheckOverCurrent(void)
{
    float ia = GetCurrentA();
    float ib = GetCurrentB();
    float ic = GetCurrentC();

    return (ia > MAX_PHASE_CURRENT_AMPS || ia < -MAX_PHASE_CURRENT_AMPS ||
            ib > MAX_PHASE_CURRENT_AMPS || ib < -MAX_PHASE_CURRENT_AMPS ||
            ic > MAX_PHASE_CURRENT_AMPS || ic < -MAX_PHASE_CURRENT_AMPS) ? 1U : 0U;
}

//@brief Check for overtemperature condition on any MOSFET
//@return 1 if overtemperature detected, 0 otherwise
static uint16_t CheckOverTemperature(void)
{
    float temp_ah = GetTempAH();
    float temp_al = GetTempAL();
    float temp_bh = GetTempBH();
    float temp_bl = GetTempBL();

    return (temp_ah > MAX_TEMP_C || temp_al > MAX_TEMP_C ||
            temp_bh > MAX_TEMP_C || temp_bl > MAX_TEMP_C) ? 1U : 0U;
}

//@brief Check for overvoltage on DC link
//@return 1 if overvoltage detected, 0 otherwise
static uint16_t CheckOverVoltage(void)
{
    float vdc = GetVoltageDC();
    return (vdc > MAX_DC_VOLTAGE) ? 1U : 0U;
}

//@brief Check for gate driver fault signals
//@return 1 if any GD fault detected, 0 otherwise
static uint16_t CheckGdFault(void)
{
    uint32_t fault1 = GPIO_readPin(GD_FAULT1_READ_PIN);
    uint32_t fault2 = GPIO_readPin(GD_FAULT2_READ_PIN);
    uint32_t fault3 = GPIO_readPin(GD_FAULT3_READ_PIN);

    return (fault1 || fault2 || fault3) ? 1U : 0U;
}

/* -------------------------------------------------------------------------- */
/* Public API - System control                                                 */
/* -------------------------------------------------------------------------- */

void InitSafetyChecker(void)
{
    safety_error_state = ERRNO_NO_ERR;

    // Configure GD fault input pins
    GPIO_setPadConfig(GD_FAULT1_READ_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FAULT1_READ_PIN, GPIO_DIR_MODE_IN);

    GPIO_setPadConfig(GD_FAULT2_READ_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FAULT2_READ_PIN, GPIO_DIR_MODE_IN);

    GPIO_setPadConfig(GD_FAULT3_READ_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FAULT3_READ_PIN, GPIO_DIR_MODE_IN);

    // Configure discharge relay output pin
    GPIO_setPadConfig(DISCHARGE_RELAY_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DISCHARGE_RELAY_PIN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(DISCHARGE_RELAY_PIN, 0);  // Start with discharge on, normally closed relay
}

//@brief Safely shutdown system: disable drivers, disable PWM, activate discharge
void SafetyShutdown(void)
{
    // Disable control loop first
    ControlLoop_Disable();

    // Disable all PWM outputs (hard disable via action qualifier)
    DisablePWM(PWM_CHANNEL_A);
    DisablePWM(PWM_CHANNEL_B);
    DisablePWM(PWM_CHANNEL_C);

    // Disable gate drivers
    GPIO_writePin(GD_ENABLE_PIN, 0);

    // Activate discharge relay to bleed off DC bus energy (normally closed relay)
    GPIO_writePin(DISCHARGE_RELAY_PIN, 0);
}

//@brief Safely enable system after fault recovery
void SafetyRecovery(void)
{
    // Dectivate discharge relay (normally closed relay)
    GPIO_writePin(DISCHARGE_RELAY_PIN, 1);
    safety_error_state = ERRNO_NO_ERR;
}

//@brief Check system health and update error state
//@return Current error status
ErrorStatusTypeDef TaskCheckSystemHealth(void)
{
    // Skip checks if already in fault state (latched)
    if (safety_error_state != ERRNO_NO_ERR)
    {
        return safety_error_state;
    }

    // Check conditions in priority order (GD fault highest priority)
    if (CheckGdFault())
    {
        safety_error_state = ERRNO_OC_ERR;  // Reuse OC error code for GD fault
        SafetyShutdown();
        return safety_error_state;
    }

    if (CheckOverVoltage())
    {
        safety_error_state = ERRNO_OV_ERR;
        SafetyShutdown();
        return safety_error_state;
    }

    if (CheckOverCurrent())
    {
        safety_error_state = ERRNO_OC_ERR;
        SafetyShutdown();
        return safety_error_state;
    }

    if (CheckOverTemperature())
    {
        safety_error_state = ERRNO_OT_ERR;
        SafetyShutdown();
        return safety_error_state;
    }

    return safety_error_state;
}

//@brief Get current error status without checking conditions
//@return Current error status (may be latched)
ErrorStatusTypeDef GetErrorStatus(void)
{
    return safety_error_state;
}

//@brief Check if system is in a fault state
//@return 1 if faulted, 0 if healthy
uint16_t IsFaulted(void)
{
    return (safety_error_state != ERRNO_NO_ERR) ? 1U : 0U;
}

/* -------------------------------------------------------------------------- */
/* Driver control helpers                                                      */
/* -------------------------------------------------------------------------- */

void EnableDrivers(void)
{
    GPIO_writePin(GD_ENABLE_PIN, 1);
}

void DisableDrivers(void)
{
    GPIO_writePin(GD_ENABLE_PIN, 0);
}
