#include <stdio.h>
#include "adc_config.h"
#include "board_test.h"
#include "gpio.h"

int GD_EN_VAL =1;

void InitGateDriverTest()
{
    GPIO_setPadConfig(BLINKY_LED_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(BLINKY_LED_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_writePin(BLINKY_LED_GPIO, 0); // LED off

    GPIO_setPadConfig(GD_EN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_EN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_EN, 0); 

    GPIO_setPadConfig(GD_HS_PWM1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_HS_PWM1, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_HS_PWM1, 0); 

    GPIO_setPadConfig(GD_LS_PWM1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_LS_PWM1, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_LS_PWM1, 0); 

    GPIO_setPadConfig(GD_HS_PWM2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_HS_PWM2, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_HS_PWM2, 0); 

    GPIO_setPadConfig(GD_LS_PWM2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_LS_PWM2, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_LS_PWM2, 0); 

    GPIO_setPadConfig(GD_HS_PWM3, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_HS_PWM3, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_HS_PWM3, 0); 

    GPIO_setPadConfig(GD_LS_PWM3, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_LS_PWM3, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_LS_PWM3, 0); 

    GPIO_setPadConfig(GD_FLT1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FLT1, GPIO_DIR_MODE_IN);

    GPIO_setPadConfig(GD_FLT2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FLT2, GPIO_DIR_MODE_IN);

    GPIO_setPadConfig(GD_FLT3, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FLT3, GPIO_DIR_MODE_IN);

}

void EnableVoltageSen()
{
    HAL_StatusTypeDef ADC_Config_Init();
    
    float voltageA = 0.0f;
    float voltageB = 0.0f;
    float voltageC = 0.0f;
    float voltageDC = 0.0f;
    


    void ADC_TriggerVoltages();

    voltageA = GetVoltageA();
    voltageB = GetVoltageB();
    voltageC = GetVoltageC();
    voltageDC = GetVoltageDC();


    return;
}

void EnableGateDriver(void)
{
    int val1, val2,val3;
    GD_EN_VAL =!(GD_EN_VAL);
    
    

    GPIO_writePin(GD_EN,GD_EN_VAL);
    

    GPIO_writePin(GD_HS_PWM1,1);
    GPIO_writePin(GD_LS_PWM1,1);

    GPIO_writePin(GD_HS_PWM2,1);
    GPIO_writePin(GD_LS_PWM2,1);

    GPIO_writePin(GD_HS_PWM3,1);
    GPIO_writePin(GD_LS_PWM3,1);
    

    val1 = GPIO_readPin(GD_FLT1);
    val2 = GPIO_readPin(GD_FLT2);
    val3 = GPIO_readPin(GD_FLT3);

    if (val1==1 && val2==1 && val3==1)
    {
        GPIO_writePin(BLINKY_LED_GPIO, 0);
    }

    return;
}

void EnableCurrentSen()
{
    float CurrentA = 0.0f;
    float CurrentB = 0.0f;
    float CurrentC = 0.0f;


    CurrentA = GetCurrentA();
    CurrentB = GetCurrentB();
    CurrentC = GetCurrentC();

    
}
