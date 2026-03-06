#include "main.h"
#include "bsp_cputimer.h"
#include "bsp_sci.h"
#include "F2837xD_GlobalPrototypes.h"
#include "sm.h"
#include "taskscheduler.h"

#define BLINKY_LED_GPIO    31

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

    DINT;

    Interrupt_initModule();
    Interrupt_initVectorTable();

//
// Enable global Interrupts and higher priority real-time debug events:
//

    

    

    //EALLOW;  // This is needed to write to EALLOW protected registers
    //PieVectTable.TIMER0_INT = &_bspTimerIsr;
    //EDIS;    // This is needed to disable write to EALLOW protected registers

    Interrupt_initVectorTable();
    bspInitCpuTimers();
    bspInitSCI();
    EINT;
    ERTM;
    InitStateMachine();
    InitTaskScheduler();

    for(;;)
    {
        LoopTaskScheduler();
    }
}

