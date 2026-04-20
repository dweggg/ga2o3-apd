#include "main.h"
#include "bsp_cputimer.h"
#include "bsp_sci.h"
#include "F2837xD_GlobalPrototypes.h"
#include "state_machine.h"
#include "task_scheduler.h"
#include "math.h"
#include "board_test.h"
#include "global_defines.h"
#include "batch.h"



void ToggleLED(void)
{
    GPIO_togglePin(BLINKY_LED_GPIO);
}

void SendUartTest(void)
{
    SCI_writeCharBlockingFIFO(SCIA_BASE, 0xffff);
}
float sine_wave_test = 0.0f;
uint32_t sine_wave_index = 0;
float32_t _delta = 2.0f*M_PI / 10000;

#pragma CODE_SECTION(CreateSineWaveTest, ".TI.ramfunc");
void CreateSineWaveTest(void)
{
    
    
    if(sine_wave_index >= 10000)
    {
        sine_wave_index = 0;        
    }
    float32_t _angle = _delta * sine_wave_index;
    sine_wave_test = (float32_t)__sin(_angle);
    sine_wave_index++;
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


    StartBatch();
    CreateTask(RunTests, 1);

    CreateTask(ToggleLED, 2);

    // CreateTask(EnableGateDriver,1);
    // CreateTask(EnableVoltageSen,1);
   

    
    // CreateTask(CreateSineWaveTest, 10000);
    // CreateTask(SendUartTest, 300000);
    LoopTaskScheduler();
}

