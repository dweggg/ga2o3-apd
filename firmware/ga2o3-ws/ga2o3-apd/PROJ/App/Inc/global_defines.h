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

#define L_H                                 (0.0036)
#define R_OHM                               (0.05)

#define MAX_PHASE_CURRENT_AMPS              (20)


// PWM channel assignments
#define CHANNEL_A  1U   // EPWM1
#define CHANNEL_B  2U   // EPWM2
#define CHANNEL_C  3U   // EPWM3

#ifdef __cplusplus
}
#endif
#endif
