#include "taskscheduler.h"
#include "bsp_cputimer.h"
#include "sm.h"

#define TICKS_CTRL_PERDIOD              (PARAMS_SYS_CLOCK / PARAMS_CTRL_FREQUENCY)
#define TICKS_1MS_PERIOD                (PARAMS_SYS_CLOCK / 1000U)
#define TICKS_10MS_PERIOD               (PARAMS_SYS_CLOCK / 100U)
#define TICKS_100MS_PERIOD              (PARAMS_SYS_CLOCK / 10U)
#define TICKS_500MS_PERIOD              (PARAMS_SYS_CLOCK / 2U)
#define TICKS_1S_PERIOD                 (PARAMS_SYS_CLOCK)

TaskControlBlockTypeDef _taskControl;
TaskControlBlockTypeDef _task1ms;
TaskControlBlockTypeDef _task10ms;
TaskControlBlockTypeDef _task100ms;
TaskControlBlockTypeDef _task500ms;
TaskControlBlockTypeDef _task1s;

HAL_StatusTypeDef InitTaskScheduler()
{
    uint32_t _now = bspGetCpuTimerTicks();
    _taskControl._lastTick = _now;
    _taskControl._periodTicks = TICKS_CTRL_PERDIOD;
    _taskControl.callback = &TaskControl;

    _task1ms._lastTick = _now;
    _task1ms._periodTicks = TICKS_1MS_PERIOD;
    _task1ms.callback = &Task1ms;

    _task10ms._lastTick = _now;
    _task10ms._periodTicks = TICKS_10MS_PERIOD;
    _task10ms.callback = &Task10ms;

    _task100ms._lastTick = _now;
    _task100ms._periodTicks = TICKS_100MS_PERIOD;
    _task100ms.callback = &Task100ms;

    _task500ms._lastTick = _now;
    _task500ms._periodTicks = TICKS_500MS_PERIOD;
    _task500ms.callback = &Task500ms;

    _task1s._lastTick = _now;
    _task1s._periodTicks = TICKS_1S_PERIOD;
    _task1s.callback = &Task1s;
    return HAL_OK;
}



static inline void ExecTaskScheduler(TaskControlBlockTypeDef *tcb, uint32_t now)
{
    if (now - tcb->_lastTick>= tcb->_periodTicks)
    {
        tcb->_lastTick = now;
        tcb->callback();
    }
}

void LoopTaskScheduler(void)
{
    uint32_t _now = bspGetCpuTimerTicks();
    ExecTaskScheduler(&_taskControl, _now);
    ExecTaskScheduler(&_task1ms, _now);
    ExecTaskScheduler(&_task10ms, _now);
    ExecTaskScheduler(&_task100ms, _now);
    ExecTaskScheduler(&_task500ms, _now);
    ExecTaskScheduler(&_task1s, _now);
}
void TaskControl()
{
    //do control things.
    StateMachineLoop();
    
}

void Task1ms()
{
    return;
}

void Task10ms()
{
    return;
}

void Task100ms()
{
    return;
}

void Task500ms()
{
    GPIO_togglePin(31);
    return;
}

void Task1s()
{
    return;
}
