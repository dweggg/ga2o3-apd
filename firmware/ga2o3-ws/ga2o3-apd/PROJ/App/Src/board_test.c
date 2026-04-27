#include <stdio.h>
#include "adc_config.h"
#include "board_test.h"
#include "gpio.h"


int flt1, flt2,flt3, enable_gd,enable_pwm;

int enable_voltage; 
float voltageread_DC,voltageread_1,voltageread_2,voltageread_3;

int enable_current; 
float currentread_A,currentread_B,currentread_C;

void EnableGateDriver(void)
{
  
    InitPWM(1);
    InitPWM(2);
    InitPWM(3);

    GPIO_setPadConfig(GD_EN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_EN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_EN, 0); // off
    

    return;
}


void RunDriver(void){

    GPIO_writePin(GD_EN,enable_gd);

    
    flt1 = GPIO_readPin(GD_FLT1);
    flt2 = GPIO_readPin(GD_FLT2);
    flt3 = GPIO_readPin(GD_FLT3);


    if (enable_pwm) {
        EnablePWM(1);
        EnablePWM(2);
        EnablePWM(3);
        
    }else {
        DisablePWM(1);
        DisablePWM(2);
        DisablePWM(3);
        
    }
}

void InitVoltageSense(void)
{
    HAL_StatusTypeDef ADC_Config_Init(void);
    ADC_TriggerVoltages();
}

void RunVoltageSense(void){

    

    


    if (enable_voltage) 
    {
        ADC_TriggerVoltages();
        voltageread_DC = GetVoltageDC();
        voltageread_1 = GetVoltageA();
        voltageread_2 = GetVoltageB();
        voltageread_3 = GetVoltageC();


        
    }

}


void InitCurrentSense(void)
{
    HAL_StatusTypeDef ADC_Config_Init(void);
}

void RunCurrentSense(void){

    

    


    if (enable_current) 
    {
        ADC_TriggerCurrents();
        currentread_A = GetCurrentA();
        currentread_B = GetCurrentB();
        currentread_C = GetCurrentC();
       


        
    }
    
        
    }
