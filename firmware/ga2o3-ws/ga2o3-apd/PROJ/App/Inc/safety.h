/**
 * @file safety.h
 * @author lan
 * @brief Safety checker, continuously check if any fault happens
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef __SAFETY_H__
#define __SAFETY_H__

typedef enum
{
    ERRNO_NO_ERR = 0,
    ERRNO_OV_ERR,
    ERRNO_OC_ERR,
    ERRNO_OT_ERR
} ErrorStatusTypeDef;

/**
 * @brief Initialize the checker
 * 
 * @return none
 */
void InitSafetyChecker(void);

/**
 * @brief Check if any parameter is over rate 
 * 
 * @param status [out] pointer to a variable, @sa ErrorStatusTypeDef 
 */
void TaskCheckSystemHealthStatus(ErrorStatusTypeDef *status);


#endif

