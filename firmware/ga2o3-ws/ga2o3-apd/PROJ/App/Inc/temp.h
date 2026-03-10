/**
 * @file temp.h
 * @author lan
 * @brief provide functions and definitions for temperature sensors.
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef __TEMP_H__
#define __TEMP_H__

#include "stdint.h"
#include "inc/hw_types.h"

typedef enum
{
    PHASE_A_TOP = 0,
    PHASE_A_BOTTOM,
    PHASE_B_TOP,
    PHASE_B_BOTTOM,
    PHASE_C_TOP,
    PHASE_C_BOTTOM
} IGBTPositionTypeDef;

/**
 * @brief Get IGBT temperature by its position
 * @param pos [in] The temperature of which IGBT
 * @return the actual temperature in celsius degree
 */
float32_t GetTempByPosition(IGBTPositionTypeDef pos);

/**
 * @brief Get the Temperature of All IGBTs
 * @param temp_arr [out] a pointer to an array that to be filled with temperatures, length >= 6
 * @return none
 */
void GetTempAll(float32_t *temp_arr);

/**
 * @brief initialize temperature related parameters and functions
 * @return none
 */
void InitTemp(void);

/**
 * @brief Scheduler task to calculate temperature 
 * @return none
 */
void TaskCalculateTemp(void);



#endif
