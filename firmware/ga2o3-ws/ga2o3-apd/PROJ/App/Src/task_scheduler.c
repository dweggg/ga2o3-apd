#include "task_scheduler.h"
#include "bsp_cputimer.h"
#include "state_machine.h"
#include "global_defines.h"
#include "stdlib.h"

#pragma CODE_SECTION(LoopTaskScheduler, ".TI.ramfunc");
#pragma CODE_SECTION(TaskIdle, ".TI.ramfunc");

typedef struct 
{
    uint32_t last_tick;
    uint32_t period_ticks;
    void (*callback)(void);
    void *pointer_to_next_task;
    void *pointer_to_previous_task;
} TaskControlBlockTypeDef;

typedef struct
{
    uint16_t scheduler_enabled;
    uint16_t tasks_in_list;
    uint32_t scheduler_idle_count;
    TaskControlBlockTypeDef *task_list_pointer;
    TaskControlBlockTypeDef *task_list_root;
    TaskControlBlockTypeDef *task_list_idle;
} SchedulerCoreTypeDef;

SchedulerCoreTypeDef scheduler_core;

void InitTaskScheduler(void)
{
    TaskControlBlockTypeDef *root_task = malloc(sizeof(TaskControlBlockTypeDef));
    if(root_task != NULL)
    {
        root_task->pointer_to_next_task = root_task; // circular: points to itself
        root_task->pointer_to_previous_task = root_task;
        root_task->callback = TaskIdle;
        root_task->last_tick = 0;
        root_task->period_ticks = UINT32_MAX; // idle always sorts last
        scheduler_core.task_list_root = root_task;
        scheduler_core.task_list_idle = root_task;
        scheduler_core.task_list_pointer = root_task;
    }
    else
    {
        while(true);
    }
    scheduler_core.scheduler_enabled = 1;
    scheduler_core.tasks_in_list = 1;
    scheduler_core.scheduler_idle_count = 0;
}


void LoopTaskScheduler(void)
{
    while(1)
    {
        if(!scheduler_core.scheduler_enabled) continue;

        uint32_t current_tick = bspGetCpuTimerTicks();
        TaskControlBlockTypeDef *task = scheduler_core.task_list_root; // always start from root

        for(uint16_t i = 0; i < scheduler_core.tasks_in_list; i++)
        {
            if(current_tick - task->last_tick >= task->period_ticks)
            {
                task->callback();
                task->last_tick = current_tick;
            }
            task = (TaskControlBlockTypeDef *)task->pointer_to_next_task;
        }
    }
}

HAL_StatusTypeDef CreateTask(void (*task_handler)(void), uint32_t hertz)
{
    if(scheduler_core.tasks_in_list >= PARAMS_SCHEDULER_MAX_TASKS_LIMIT) return HAL_ERROR;
    if(NULL == task_handler) return HAL_ERROR;

    uint32_t period_ticks = PARAMS_SYS_CLOCK / hertz;

    TaskControlBlockTypeDef *new_task = malloc(sizeof(TaskControlBlockTypeDef));
    if(NULL == new_task) return HAL_ERROR;

    new_task->callback = task_handler;
    new_task->last_tick = 0;
    new_task->period_ticks = period_ticks;

    // Walk list to find insertion point (sorted ascending by period, idle always last)
    // Idle has period UINT32_MAX so new tasks always insert before it
    TaskControlBlockTypeDef *current = scheduler_core.task_list_root;
    for(uint16_t i = 0; i < scheduler_core.tasks_in_list; i++)
    {
        if(period_ticks <= current->period_ticks)
        {
            break; // insert before current
        }
        current = (TaskControlBlockTypeDef *)current->pointer_to_next_task;
    }

    // Insert new_task before current
    TaskControlBlockTypeDef *prev = (TaskControlBlockTypeDef *)current->pointer_to_previous_task;
    new_task->pointer_to_next_task = current;
    new_task->pointer_to_previous_task = prev;
    prev->pointer_to_next_task = new_task;
    current->pointer_to_previous_task = new_task;

    // Update root if inserted at the front
    if(period_ticks <= scheduler_core.task_list_root->period_ticks)
    {
        scheduler_core.task_list_root = new_task;
    }

    scheduler_core.tasks_in_list++;
    return HAL_OK;
}

void TaskIdle(void)
{
    scheduler_core.scheduler_idle_count ++;
    return;
}

void StartTimer(uint32_t *timer)
{
    *timer = (uint32_t)bspGetCpuTimerTicks();
}

uint32_t EvalTimer(uint32_t timer)
{
    uint32_t current_tick = bspGetCpuTimerTicks();
    uint32_t elapsed_ticks = current_tick - timer;
    return (uint32_t)(elapsed_ticks / (PARAMS_SYS_CLOCK / 1000));
}

float GetTaskPeriod(void (*task_handler)(void))
{
    if(NULL == task_handler)
    {
        return 0;
    }
    
    TaskControlBlockTypeDef *current_task = scheduler_core.task_list_root;
    
    for(uint16_t i = 0; i < scheduler_core.tasks_in_list; i++)
    {
        if(current_task->callback == task_handler)
        {
            return (float)current_task->period_ticks / (float)PARAMS_SYS_CLOCK;
        }
        current_task = (TaskControlBlockTypeDef *)current_task->pointer_to_next_task;
    }
    
    return 0;  // task not found
}
