#include "viewer.h"
#include "stdint.h"
#include "device.h"
#include "bsp_cputimer.h"

#pragma CODE_SECTION(serialDriverSendData, ".TI.ramfunc");
uint32_t _tim;

void serialDriverSendData(uint16_t* buf, uint16_t size)
{
    _tim = bspGetCpuTimerTicks();
    for(uint16_t i = 0; i < size; i++)
    {
        SCI_writeCharBlockingFIFO(SCIA_BASE, buf[i]);
    }
    _tim = bspGetCpuTimerTicks() - _tim;
    //SCI_writeCharArray(SCIA_BASE, buf, size);
}
