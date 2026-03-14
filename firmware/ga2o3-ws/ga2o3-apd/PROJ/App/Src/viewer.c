#include "viewer.h"
#include "stdint.h"
#include "device.h"


void serialDriverSendData(uint16_t* buf, uint16_t size)
{
    SCI_writeCharArray(SCIA_BASE, buf, size);
}
