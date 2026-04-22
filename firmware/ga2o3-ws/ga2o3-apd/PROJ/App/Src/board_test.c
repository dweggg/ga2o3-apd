#include <stdio.h>
#include "adc_config.h"
#include "board_test.h"
#include "gpio.h"

int flt1, flt2,flt3, enable_gd,enable_pwm;

void EnableGateDriver(void)
{
  
    InitPWM(1);
    InitPWM(2);
    InitPWM(3);

    GPIO_setPadConfig(GD_EN, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_EN, GPIO_DIR_MODE_OUT);
    GPIO_writePin(GD_EN, 0); // off
    
    GPIO_setPadConfig(GD_FLT1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FLT1, GPIO_DIR_MODE_IN);
    GPIO_writePin(GD_FLT1, 0); // off
    
    GPIO_setPadConfig(GD_FLT2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FLT2, GPIO_DIR_MODE_IN);
    GPIO_writePin(GD_FLT2, 0); // off
    
    GPIO_setPadConfig(GD_FLT3, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(GD_FLT3, GPIO_DIR_MODE_IN);
    GPIO_writePin(GD_FLT3, 0); // off
    

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
