#include "sm.h"

StateMachineTypeDef  GlobalStateMachineHandle;

void InitStateMachine(void)
{
    GlobalStateMachineHandle = STATE_INIT;
}

void StateMachineLoop(void)
{
    switch (GlobalStateMachineHandle) 
    {
        case STATE_INIT:
        break;
    }
}
