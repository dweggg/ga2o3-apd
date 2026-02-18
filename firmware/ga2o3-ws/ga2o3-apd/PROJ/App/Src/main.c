#include "main.h"

#define BLINKY_LED_GPIO    31

void main(void)
{
#ifdef _FLASH
    extern uint16_t RamfuncsLoadStart, RamfuncsLoadSize, RamfuncsRunStart;
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
    Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, DEVICE_FLASH_WAITSTATES);
//    InitFlash();                // config wait-states & enable ECC
#endif
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

//
// Step 3. Clear all interrupts and initialize PIE vector table:
// Disable CPU interrupts
//
    DINT;

//
// Initialize the PIE control registers to their default state.
// The default state is all PIE interrupts disabled and flags
// are cleared.
// This function is found in the F2837xD_PieCtrl.c file.
//
    InitPieCtrl();

//
// Disable CPU interrupts and clear all CPU interrupt flags:
//
    IER = 0x0000;
    IFR = 0x0000;

//
// Initialize the PIE vector table with pointers to the shell Interrupt
// Service Routines (ISR).
// This will populate the entire table, even if the interrupt
// is not used in this example.  This is useful for debug purposes.
// The shell ISR routines are found in F2837xD_DefaultIsr.c.
// This function is found in F2837xD_PieVect.c.
//
    InitPieVectTable();

//
// Enable global Interrupts and higher priority real-time debug events:
//
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    for(;;)
    {
        GPIO_togglePin(BLINKY_LED_GPIO);
        DEVICE_DELAY_US(500000);    // 0.5 s at 200 MHz core clock
    }
}