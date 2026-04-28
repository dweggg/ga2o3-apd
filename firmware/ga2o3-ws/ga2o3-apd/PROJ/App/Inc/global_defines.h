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

#define L_H                                 (0.001)
#define R_OHM                               (5.8)

typedef enum {
    PWM_CHANNEL_A = 1U,
    PWM_CHANNEL_B = 2U,
    PWM_CHANNEL_C = 3U,
} PwmChannelTypeDef;

#ifdef __cplusplus
}
#endif
#endif
