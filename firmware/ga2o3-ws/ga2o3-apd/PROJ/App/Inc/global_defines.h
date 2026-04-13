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
#define TS_US                               (200)
#define CURRENT_FB_GAIN                     (-1)
#define FSW                                 (10000)
#define DEATTIME_US                         (1)
#define TR                                  (0.005)
#define L1_UH                               (3000)
#define R1_OHM                              (0.05)
#define VDC                                 (400)
#define AC_FREQ                             (50)
#define SOGI_GAIN                           (1)
#define ID_REF_AMPS                         (0)
#define IQ_REF_AMPS                         (5)

#define MAX_PHASE_CURRENT_AMPS              (20)


#ifdef __cplusplus
}
#endif
#endif
