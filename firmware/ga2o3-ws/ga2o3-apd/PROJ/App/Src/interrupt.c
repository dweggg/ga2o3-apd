#include "interrupt.h"
#include "recorder.h"
#include "control_loop.h"

#pragma CODE_SECTION(AdcInterrupt, ".TI.ramfunc")
void AdcInterrupt(void)
{
    TaskControlLoop();
    recorderStep();
}
