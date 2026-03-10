#include "bsp_cputimer.h"
#include "device.h"
#include "interrupt.h"

extern struct CPUTIMER_VARS CpuTimer0;

HAL_StatusTypeDef bspInitCpuTimers()
{
#ifdef _LAUNCHXL_F28379D
//LAUNCHPAD SYSCLK 200MHz
//We only use timer 0 connected to PIE
//CPU timer is a counter-down timer, from MAX to 0.
    //Interrupt_register(INT_TIMER0, &_bspTimerIsr);
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFF);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0);
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    //CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    //Interrupt_enable(INT_TIMER0);
    CPUTimer_startTimer(CPUTIMER0_BASE);
    if(UINT32_MAX != CPUTimer_getTimerCount(CPUTIMER0_BASE))
    {
        return HAL_ERROR;
    }

    return HAL_OK;

#else
#error "It is not LANUNCHPAD XL, you need to write your own code to init cpu timers"
#endif
}

//Bcoz cpu timer count downward, make it another way around
uint32_t bspGetCpuTimerTicks()
{
    return UINT32_MAX - CPUTimer_getTimerCount(CPUTIMER0_BASE);
}


void bspRegisterInterrupt(void (*_IsrHandler)(void))
{
    EALLOW;
    PieVectTable.TIMER0_INT = _IsrHandler;
    EDIS;
}

__attribute__((weak)) __interrupt void _bspTimerIsr(void)
{
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
    return;
}

void bspConfigCPUTimer(uint32_t cpuTimer, float freq, float period)
{
    uint32_t temp;
    temp = (uint32_t)(freq / 1000000 * period);
    CPUTimer_setPeriod(cpuTimer, temp);

    CPUTimer_setPreScaler(cpuTimer, 0);
    CPUTimer_stopTimer(cpuTimer);
    CPUTimer_reloadTimerCounter(cpuTimer);
    CPUTimer_setEmulationMode(cpuTimer,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_enableInterrupt(cpuTimer);
}

void bspConfigCPUTimerMax(uint32_t cpuTimer)
{

    CPUTimer_setPreScaler(cpuTimer, 0);
    CPUTimer_stopTimer(cpuTimer);
    CPUTimer_reloadTimerCounter(cpuTimer);
    CPUTimer_setEmulationMode(cpuTimer,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_enableInterrupt(cpuTimer);
}
