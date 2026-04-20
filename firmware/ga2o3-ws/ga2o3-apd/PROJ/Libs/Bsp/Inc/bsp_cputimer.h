#pragma once
#ifndef __BSP_CPUTIMER_H__
#define __BSP_CPUTIMER_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "bsp_cputimer.h"
#include "bsp_hal.h"
#include "stdint.h"


HAL_StatusTypeDef bspInitCpuTimers();
void bspRegisterInterrupt(void (*_IsrHandler)(void));
uint32_t bspGetCpuTimerTicks();
void bspConfigCPUTimer(uint32_t cpuTimer, float freq, float period);
void bspConfigCPUTimerMax(uint32_t cpuTimer);



#ifdef __cplusplus
}
#endif
#endif

