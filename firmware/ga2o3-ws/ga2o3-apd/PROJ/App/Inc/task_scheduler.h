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

/**
 * @brief Initializes a timer.
 *
 * This function sets up and starts a timer instance pointed to by @p timer.
 *
 * @param timer Pointer to the timer variable to be initialized/started.
 */
void StartTimer(uint32_t *timer);

/**
 * @brief Evaluates the current state/value of a timer.
 *
 * This function retrieves or computes the current value of the given timer.
 *
 * @param timer The timer value/identifier to evaluate.
 * @return Current timer value or status indicator depending on implementation.
 */
uint32_t EvalTimer(uint32_t timer);

/**
 * @brief Get the period (in ticks/microseconds) of a task
 *
 * @param task_handler [in] the task function pointer
 * @return period in seconds (float), or 0 if task not found
 */
float GetTaskPeriod(void (*task_handler)(void));

#ifdef __cplusplus
}
#endif
#endif

