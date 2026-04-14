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

#define PARAMS_SYS_CLOCK                    (DEVICE_SYSCLK_FREQ)
#define PARAMS_SCHEDULER_MAX_TASKS_LIMIT    (32) //max 32 tasks are allowed

#define PI2                                 (3.1415926535 * 2)
#define TR                                  (0.005)
#define L_H                                 (0.0036)
#define R_OHM                               (0.05)
#define VDC                                 (400)

#define MAX_PHASE_CURRENT_AMPS              (20)


#ifdef __cplusplus
}
#endif
#endif
