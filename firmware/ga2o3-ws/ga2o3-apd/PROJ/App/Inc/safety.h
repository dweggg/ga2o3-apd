/**
 * @file safety.h
 * @brief Safety checker: health checks, fault protection, and safe shutdown
 * @author lan
 * @date 2026
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef __SAFETY_H__
#define __SAFETY_H__

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Safety-related GPIO pins                                                    */
/* -------------------------------------------------------------------------- */

#define GD_ENABLE_PIN           25   // Gate driver enable control output
#define DISCHARGE_RELAY_PIN     32   // Discharge circuit control output
#define GD_FAULT1_READ_PIN      66   // Gate driver fault 1 input
#define GD_FAULT2_READ_PIN     131   // Gate driver fault 2 input
#define GD_FAULT3_READ_PIN     130   // Gate driver fault 3 input


#define MAX_PHASE_CURRENT_AMPS              (20)
#define MAX_TEMP_C                          (100)
#define MAX_DC_VOLTAGE                      (420)


typedef enum
{
    ERRNO_NO_ERR = 0,
    ERRNO_OV_ERR,
    ERRNO_OC_ERR,
    ERRNO_OT_ERR
} ErrorStatusTypeDef;

/**
 * @brief Initialize the safety checker
 */
void InitSafetyChecker(void);

/**
 * @brief Perform health checks (OV, OC, OT) and update fault state
 * @return Current error status
 */
ErrorStatusTypeDef TaskCheckSystemHealth(void);

/**
 * @brief Get current error status without checking conditions
 * @return Current error status (may be latched)
 */
ErrorStatusTypeDef GetErrorStatus(void);

/**
 * @brief Check if system is in a fault state
 * @return 1 if faulted, 0 if healthy
 */
uint16_t IsFaulted(void);

/**
 * @brief Safely shutdown system: disable PWM, drivers, and activate discharge
 */
void SafetyShutdown(void);

/**
 * @brief Recover from non-latched faults
 */
void SafetyRecovery(void);


/* -------------------------------------------------------------------------- */
/* Driver control helpers                                                      */
/* -------------------------------------------------------------------------- */

void EnableDrivers(void);

void DisableDrivers(void);

#endif


