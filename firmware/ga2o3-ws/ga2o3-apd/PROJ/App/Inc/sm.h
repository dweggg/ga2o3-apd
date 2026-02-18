#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

typedef enum _StateMachineTypeDef
{
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_RUNNING,
    STATE_ERROR,
    __dummy = 0xFFFFu //make sure it's 16 bit
} StateMachineTypeDef;

extern StateMachineTypeDef  GlobalStateMachineHandle;

void StateMachineInit(void);
void StateMacineLoop(void);

#ifdef __cplusplus
}
#endif
