#include "viewer.h"
#include "stdint.h"
#include "device.h"

#pragma CODE_SECTION(serialDriverSendData, ".TI.ramfunc");

void serialDriverSendData(uint16_t* buf, uint16_t size)
{
    for(uint16_t i = 0; i < size; i++)
    {
        SCI_writeCharBlockingFIFO(SCIA_BASE, buf[i]);
    }
    //SCI_writeCharArray(SCIA_BASE, buf, size);
}
