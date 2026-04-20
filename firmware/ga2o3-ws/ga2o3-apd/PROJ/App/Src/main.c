#include "main.h"
#include "bsp_cputimer.h"
#include "bsp_sci.h"
#include "F2837xD_GlobalPrototypes.h"
#include "state_machine.h"
#include "task_scheduler.h"
#include "control_loop.h"
#include "global_defines.h"
#include "batch.h"


#define BLINKY_LED_GPIO    34

void ToggleLED(void)
{
    GPIO_togglePin(BLINKY_LED_GPIO);
}

void main(void)
{
//
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xD_SysCtrl.c file.
//
    Device_init();

//
// Step 2. Initialize GPIO:
// This example function is found in the F2837xD_Gpio.c file and
// illustrates how to set the GPIO to it's default state.
//
    Device_initGPIO();
    GPIO_setPadConfig(BLINKY_LED_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(BLINKY_LED_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_writePin(BLINKY_LED_GPIO, 0); // LED off
    

    Interrupt_initModule();
    Interrupt_initVectorTable();
    Interrupt_enableMaster();


    bspInitCpuTimers();
    bspInitSCI();


    // InitGateDriverTest();
    EINT;
    ERTM;


    InitStateMachine();
    InitTaskScheduler();

    CreateTask(ToggleLED, 2);   
    CreateTask(TaskStateMachine, 1000);   
    CreateTask(TaskControlLoop, 10000);   

    LoopTaskScheduler();
}

