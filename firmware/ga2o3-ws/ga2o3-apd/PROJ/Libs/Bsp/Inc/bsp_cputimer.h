#pragma once
#ifndef __BSP_CPUTIMER_H__
#define __BSP_CPUTIMER_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"
#include "params.h"


HAL_StatusTypeDef bspInitCpuTimers();
void bspRegisterInterrupt(void (*_IsrHandler)(void));
uint32_t bspGetCpuTimerTicks();
void bspConfigCPUTimer(uint32_t cpuTimer, float freq, float period);
void bspConfigCPUTimerMax(uint32_t cpuTimer);

__attribute__((weak)) void _bspTimerIsr(void);


#ifdef __cplusplus
}
#endif
#endif

