#pragma once
#ifndef __PARAMS_H__
#define __PARAMS_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

#define PARAMS_SYS_CLOCK                (DEVICE_SYSCLK_FREQ)
#define PARAMS_CTRL_FREQUENCY           (5000U)

typedef enum 
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U,
    HAL_UNKNOWN = 0xFFU,
    __dummy = 0XFFFFU //make sure its 16bit
} HAL_StatusTypeDef;



#ifdef __cplusplus
}
#endif
#endif
