#pragma once
#ifndef __BSP_HAL__
#define __BSP_HAL__

typedef enum 
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U,
    HAL_UNKNOWN = 0xFFU,
    __dummy = 0XFFFFU //make sure its 16bit
} HAL_StatusTypeDef;



#endif