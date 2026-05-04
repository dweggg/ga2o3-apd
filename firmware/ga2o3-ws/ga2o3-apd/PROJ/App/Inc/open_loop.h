
#include "stdint.h"
#include "control_math.h"
#include "global_defines.h"
#include "adc_config.h"
#include "control_math.h"
#include "task_scheduler.h"

typedef struct
{
    uint16_t output_enabled; //0 -> disabled, else enabled
    uint16_t openloop_enabled; 

    float voltage_open_loop_ac;
    float voltage_open_loop_pk;
    float current;
    
    float frequency_hz;
    float omega_rad;
    float sin_theta;
    float cos_theta;
    float sampling_time;
    float v_ol;

    SogiTypeDef current_sogi;
    AlphaBetaTypeDef current_ab_amps;

    DqTypeDef idq_meas_amps;
    DqTypeDef idq_ref_amps;
    

    float duty_open_loop;
  

    AngleGenTypeDef angle_generation;


    RateLimiterTypeDef rl_omega;        // ramp on open-loop angular frequency
    RateLimiterTypeDef rl_voltage_pk;   // ramp on open-loop peak voltage

}OpenLoopTypeDef;


void     OpenLoop_Enable(void);
void     OpenLoop_Disable(void);



void     OpenLoop_SetPeakVoltage(float voltage, float frequency);

void     InitOpenLoop(void);
void     TaskOpenLoop(void);



