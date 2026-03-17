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
        root_task->pointer_to_next_task = root_task;// make a loop
        root_task->pointer_to_previous_task = NULL; 
        root_task->callback = TaskIdle;
        root_task->last_tick = 0;
        root_task->period_ticks = 0;
        scheduler_core.task_list_pointer = root_task;
        scheduler_core.task_list_root = root_task;
        scheduler_core.task_list_idle = root_task;
    }
    else
    {
        while (true);// failed on Ini task scheduler, 
    }
    scheduler_core.scheduler_enabled = 1;
    scheduler_core.tasks_in_list = 1;
    scheduler_core.scheduler_idle_count = 0;
    
    return;
}

void LoopTaskScheduler(void)
{
    while(1)
    {
        if(!scheduler_core.scheduler_enabled) continue;
        
        uint32_t current_tick = bspGetCpuTimerTicks();
        
        for(uint16_t i = 0; i < scheduler_core.tasks_in_list; i++)
        {        
            if(current_tick - scheduler_core.task_list_pointer->last_tick >= scheduler_core.task_list_pointer->period_ticks )
            {
                scheduler_core.task_list_pointer->callback();
                scheduler_core.task_list_pointer->last_tick = current_tick;
            }
            scheduler_core.task_list_pointer = scheduler_core.task_list_pointer->pointer_to_next_task;
        }
    }
}

HAL_StatusTypeDef CreateTask(void (*task_handler)(void), uint32_t hertz)
{
    if(scheduler_core.tasks_in_list > PARAMS_SCHEDULER_MAX_TASKS_LIMIT) //we assume the tasks number will not reach 32 
    {
        return  HAL_ERROR;
    }
    if(NULL == task_handler)
    {
        return HAL_ERROR;
    }
    uint32_t us_period = PARAMS_SYS_CLOCK / hertz;
    TaskControlBlockTypeDef *this_task = malloc(sizeof(TaskControlBlockTypeDef));
    this_task->callback = task_handler;
    this_task->last_tick = 0;
    this_task->period_ticks = us_period;
    scheduler_core.task_list_pointer = scheduler_core.task_list_root;
    if(1 == scheduler_core.tasks_in_list) // if there is only idle task
    {
        this_task->pointer_to_previous_task = NULL;
        this_task->pointer_to_next_task = scheduler_core.task_list_pointer;
        scheduler_core.task_list_pointer->pointer_to_previous_task = this_task;
        scheduler_core.task_list_pointer->pointer_to_next_task = this_task;
        scheduler_core.task_list_root = this_task;
        scheduler_core.tasks_in_list ++;
        return HAL_OK;
    }
    for(uint16_t i = 0; i < scheduler_core.tasks_in_list - 1; i++)//tasks_in_list - 1 means we always keep the idle task at the end of task list 
    {        
        if(scheduler_core.task_list_pointer->period_ticks < us_period) //make sure that task with shorter period gets higher priority
        {
            scheduler_core.task_list_pointer = scheduler_core.task_list_pointer->pointer_to_next_task;
            continue;
        }
        //insert new task
        this_task->pointer_to_previous_task = scheduler_core.task_list_pointer->pointer_to_previous_task;
        this_task->pointer_to_next_task = scheduler_core.task_list_pointer;
        ((TaskControlBlockTypeDef *)this_task->pointer_to_previous_task)->pointer_to_next_task = this_task;
        scheduler_core.task_list_pointer->pointer_to_previous_task = this_task;
        scheduler_core.task_list_pointer = this_task;
        scheduler_core.tasks_in_list ++;
        if(0 == i)
        {
            scheduler_core.task_list_root = this_task;
            scheduler_core.task_list_idle->pointer_to_next_task = this_task;
        }
        return HAL_OK;
    }
}

void TaskIdle(void)
{
    scheduler_core.scheduler_idle_count ++;
    return;
}
