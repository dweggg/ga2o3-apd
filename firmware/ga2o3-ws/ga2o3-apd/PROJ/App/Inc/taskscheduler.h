#pragma once
#ifndef __TASK_SCHEDULAR_H__
#define __TASK_SCHEDULAR_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"
#include "params.h"

typedef struct 
{
    uint32_t _lastTick;
    uint32_t _periodTicks;
    void (*callback)(void);
} TaskControlBlockTypeDef;

HAL_StatusTypeDef InitTaskScheduler();
void LoopTaskScheduler(void);
void TaskControl();
void Task1ms();
void Task10ms();
void Task100ms();
void Task500ms();
void Task1s();

#ifdef __cplusplus
}
#endif
#endif

