#include "state_machine.h"

StateMachineTypeDef  state_machine_handle;

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
