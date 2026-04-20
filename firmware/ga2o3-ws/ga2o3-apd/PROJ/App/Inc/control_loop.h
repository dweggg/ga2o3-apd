#ifndef __CONTROL_LOOP_H__
#define __CONTROL_LOOP_H__

#include "global_defines.h"
#include "control_math.h"

typedef struct
{
    uint16_t interleaved; //0 -> non-interleaved, else interleaved
    uint16_t output_enabled; //0 -> disabled, else enabled
    uint16_t current_control_enabled; //0 -> disabled, else enabled

    float sampling_time;

    float voltage_open_loop_ac;
    float voltage_open_loop_pk;
    float duty_open_loop;
    float omega_rad;
    SogiTypeDef current_sogi;
    float sin_theta;
    float cos_theta;
    AngleGenTypeDef angle_generation;
    
    float current_feedback_amps;
    AlphaBetaTypeDef current_ab_amps;
    DqTypeDef idq_meas_amps;
    DqTypeDef idq_ref_amps;
    
    PiTypeDef pi_id;
    PiTypeDef pi_iq;
    PolarTypeDef pi_output_vs_delta;
    CartTypeDef pi_output_dq;
    AlphaBetaTypeDef voltage_ab;
    DqTypeDef pi_output_dq_sat;

    
    float duty_closed_loop;

} ControlParamsTypeDef;


/**
 * @brief Initialises PI controllers, SOGI, angle generator and zeroes all
 *        working variables. Call once on power-up and again after any mode
 *        change that requires a clean restart.
 */
void     InitControlLoop(void);

/**
 * @brief Cooperative-scheduler task.  Runs the full signal-processing chain
 *        (angle gen -> SOGI -> alpha-beta/dq -> PI -> clamp -> duty) if the loop is
 *        enabled, otherwise returns immediately.
 */
void     TaskControlLoop(void);

/* Enable / disable -------------------------------------------------------- */
void     ControlLoop_Enable(void);
void     ControlLoop_Disable(void);
uint16_t ControlLoop_IsEnabled(void);

/* Reference setpoints ------------------------------------------------------ */
void     ControlLoop_SetIdRef(float id_amps);
void     ControlLoop_SetIqRef(float iq_amps);
void     ControlLoop_SetOpenLoopVoltage(float voltage, float fundamental_frequency);


/* Mode --------------------------------------------------------------------- */
void     ControlLoop_SetInterleavedMode(uint16_t enabled);

#endif /* __CONTROL_LOOP_H__ */
