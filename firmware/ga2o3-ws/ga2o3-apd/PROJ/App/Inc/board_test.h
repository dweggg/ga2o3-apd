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

#define GD_FLT1     66
#define GD_FLT2     131
#define GD_FLT3     130

void EnableGateDriver(void);

void RunDriver(void);
void InitVoltageSense(void);
void RunVoltageSense(void);

void InitCurrentSense(void);
void RunCurrentSense(void);