#include "sm.h"

StateMachineTypeDef  GlobalStateMachineHandle;

void StateMachineInit(void)
{
    GlobalStateMachineHandle = STATE_INIT;
}

void StateMacineLoop(void)
{
    while (TRUE) 
    {
        switch (GlobalStateMachineHandle) 
        {
            case STATE_INIT:
                break;
        }
    };
}
