#include "state_machine.h"

StateMachineTypeDef  state_machine_handle;

#pragma CODE_SECTION(TaskStateMachine, ".TI.ramfunc");

void InitStateMachine(void)
{
    state_machine_handle = STATE_INIT;
}

void TaskStateMachine(void)
{
    switch (state_machine_handle) 
    {
        case STATE_INIT:
        break;
    }
}
