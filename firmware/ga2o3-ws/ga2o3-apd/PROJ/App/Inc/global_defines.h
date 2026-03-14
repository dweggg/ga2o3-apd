/**
 * @file global_defines.h
 * @author lan
 * @brief define system parameters
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#ifndef __GLOBAL_DEFINES_H__
#define __GLOBAL_DEFINES_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "device.h"

#define PARAMS_SYS_CLOCK                (DEVICE_SYSCLK_FREQ)
#define PARAMS_TICKS_PER_US             (PARAMS_SYS_CLOCK / 1000000)
#define PARAMS_CTRL_FREQUENCY           (5000U)

#define PARAMS_SCHEDULER_MAX_TASKS_LIMIT    (32) //max 32 tasks are allowed

#ifdef __cplusplus
}
#endif
#endif
