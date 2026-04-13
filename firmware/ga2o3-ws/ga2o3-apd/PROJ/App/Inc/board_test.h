/**
 * @file board_test.h
 * @author Arnold
 * @brief Enable Different Functions of the system to test
 * @version 1.0
 * @date 2026-04-06
 *
 * @copyright Copyright (c) 2026
 *
 */


#include "bsp_adc.h"
#include "adc_config.h"

#define GD_EN       25 
#define GD_HS_PWM1  1  
#define GD_LS_PWM1  0
#define GD_HS_PWM2  3  
#define GD_LS_PWM2  2
#define GD_HS_PWM3  5
#define GD_LS_PWM3  4

#define GD_FLT1     66
#define GD_FLT2     131
#define GD_FLT3     130

#define BLINKY_LED_GPIO    10


void InitGateDriverTest(void);
void EnableGateDriver(void);


void EnableVoltageSen();

void EnableCurrentSen();

