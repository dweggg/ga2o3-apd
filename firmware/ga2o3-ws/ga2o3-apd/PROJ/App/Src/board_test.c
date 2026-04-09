#include <stdio.h>
#include "adc_config.h"

void EnableVoltageSen()
{
    HAL_StatusTypeDef ADC_Config_Init();
    
    float voltageA = 0.0f;
    float voltageB = 0.0f;
    float voltageC = 0.0f;
    float voltageDC = 0.0f;
    

    for(int i=0; i<10;i++)
    {
        void ADC_TriggerVoltages();

        voltageA = GetVoltageA();
        voltageB = GetVoltageB();
        voltageC = GetVoltageC();
        voltageDC = GetVoltageDC();

    }
    return;
}

