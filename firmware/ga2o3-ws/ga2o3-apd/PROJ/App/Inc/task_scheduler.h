/**
 * @file task_scheduler.h
 * @author lan
 * @brief utils and core of task scheduler,
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#ifndef __TASK_SCHEDULAR_H__
#define __TASK_SCHEDULAR_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "global_defines.h"
#include "bsp_hal.h"

/**
 * @brief Initialize task scheduler
 * 
 * @return none
 */
void InitTaskScheduler(void);

/**
 * @brief infinite loop of task scheduler
 * 
 * @return none
 */
void LoopTaskScheduler(void);

/**
 * @brief Create a Task to task scheduler
 * 
 * @param task_handler [in] the task function
 * @param hertz [in] the frequency that task will be executed
 * @return @sa HAL_StatusTypeDef,
 */
HAL_StatusTypeDef CreateTask(void (*task_handler)(void), uint32_t hertz);

/**
 * @brief The idle task
 * 
 * @return none
 */
void TaskIdle(void);

#ifdef __cplusplus
}
#endif
#endif

