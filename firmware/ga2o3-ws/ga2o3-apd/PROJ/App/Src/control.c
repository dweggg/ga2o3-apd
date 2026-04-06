// @file control.c
// @author Arnold
// @brief implement control task
// @version 1.0
// @date 2026-04-06
//
// @copyright Copyright (c) 2026

#include "control.h"
#include <math.h>

// @brief Initialize PID controller to zero and set gains
// @param [in,out] pid_var       PID struct to initialize
// @param [in]     set_point     target reference value
// @param [in]     kp            proportional gain
// @param [in]     ki            integral gain
// @param [in]     limit         integral clamp value, applied as +/- limit
// @param [in]     sampling_time loop period in seconds
// @return none
void InitPIDControl(PidTypeDef *pid_var,float set_point,float kp, float ki,float limit, float sampling_time)
{
    // Initialize outputs and state to zero
    pid_var->error         = 0.0f;
    pid_var->integral      = 0.0f;
    pid_var->output        = 0.0f;

    // Store gains
    pid_var->kp            = kp;
    pid_var->ki            = ki;

    // Store setpoint and timing
    pid_var->set_point     = set_point;
    pid_var->sampling_time = sampling_time;

    // Apply integral clamp symmetrically around zero
    pid_var->min_integral  = -limit;
    pid_var->max_integral  =  limit;
}


// @brief Run one PID cycle, call at fixed sampling_time interval
// @param [in,out] pid_var         PID struct
// @param [in]     measured_value  current process measurement
// @return none
void PIDControl(PidTypeDef *pid_var, float measured_value)
{    
    // Calculate error between target and measured
    pid_var->error = pid_var->set_point - measured_value;

    // Accumulate error over time — integral term
    pid_var->integral += pid_var->error * pid_var->sampling_time;

    // Clamp integral to prevent windup
    if (pid_var->integral > pid_var->max_integral)
    {
        pid_var->integral = pid_var->max_integral;
    }
    else if (pid_var->integral < pid_var->min_integral)
    {
        pid_var->integral = pid_var->min_integral;
    }

    // Sum P and I contributions to produce output
    pid_var->output = (pid_var->kp * pid_var->error) + (pid_var->ki * pid_var->integral);
  

    return;
}

/* -----------------------------------------------------------------------
 * Coordinate transforms
 * -------------------------------------------------------------------- */

// @brief Convert polar coordinates to cartesian
// @param [in] p    polar input (r, theta)
// @return          cartesian output (x, y)
CartTypeDef ConvertPolarToCartesian(PolarTypeDef p)
{
    CartTypeDef c ={0};

    // x = r * cos(theta), y = r * sin(theta)
    c.x = p.r * cosf(p.theta);
    c.y = p.r * sinf(p.theta);
    return c ;
}

// @brief Convert cartesian coordinates to polar
// @param [in] c    cartesian input (x, y)
// @return          polar output (r, theta)
PolarTypeDef ConvertCartesianToPolar(CartTypeDef c)
{
    PolarTypeDef p = {0};

    // magnitude from Pythagoras
    p.r = sqrtf(c.x * c.x + c.y * c.y);

    // angle using four-quadrant arctangent
    p.theta = atan2f(c.y, c.x);

    return p;
}


/* -----------------------------------------------------------------------
 * Frame transforms
 * -------------------------------------------------------------------- */

// @brief Park transform — stationary alphabeta frame to rotating dq frame
// @param [in] alphabeta_var    alphabeta frame input
// @param [in] theta            rotor electrical angle in radians
// @return                      dq frame output
DqTypeDef ConvertAlphabetaToDq(AlphabetaTypeDef alphabeta_var, float theta)
{
    DqTypeDef dq_var = {0};

    
    dq_var.d =  alphabeta_var.alpha * cosf(theta) + alphabeta_var.beta * sinf(theta);
    dq_var.q = -alphabeta_var.alpha * sinf(theta) + alphabeta_var.beta * cosf(theta);

    return dq_var;
}

// @brief Inverse Park transform — rotating dq frame to stationary alphabeta frame
// @param [in] dq_var   dq frame input
// @param [in] theta    rotor electrical angle in radians
// @return              alphabeta frame output
AlphabetaTypeDef ConvertDqToAlphabeta(DqTypeDef dq_var, float theta)
{
    AlphabetaTypeDef alphabeta_var = {0};

    
    alphabeta_var.alpha = dq_var.d * cosf(theta) - dq_var.q * sinf(theta);
    alphabeta_var.beta  = dq_var.d * sinf(theta) + dq_var.q * cosf(theta);

    return alphabeta_var;
}

/* -----------------------------------------------------------------------
 * SOGI
 * -------------------------------------------------------------------- */

// @brief Initialize SOGI struct
// @param [in,out] sogi_var      SOGI struct to initialize
// @param [in]     gain          damping gain k, typically sqrt(2) = 1.414
// @param [in]     sampling_time loop period in seconds
// @return none
void InitSogi(SogiTypeDef *sogi_var,float gain, float sampling_time)
{
    // Initialize integrator states to zero
    sogi_var->alpha         = 0.0f;
    sogi_var->beta          = 0.0f;

    // Store damping gain and timing
    sogi_var->k             = gain;
    sogi_var->sampling_time = sampling_time;
    return ;    
}

// @brief Run one SOGI cycle, generates quadrature alpha and beta signals
// @param [in,out] sogi_var  SOGI struct
// @param [in]     input     single phase AC input sample
// @param [in]     omega     angular frequency in rad/s
// @return none
void RunSogi(SogiTypeDef *sogi_var,float input,float omega)
{
    float err = (input * sogi_var->k)- sogi_var->alpha;
    // First integrator — alpha tracks the input in-phase
    sogi_var->alpha += ((err - sogi_var->beta)*omega)*sogi_var->sampling_time;    
     // Second integrator — beta is 90 degrees shifted version of alpha
    sogi_var->beta += sogi_var->alpha*omega*sogi_var->sampling_time;                      
    return;

}

/* -----------------------------------------------------------------------
 * Angle generation
 * -------------------------------------------------------------------- */

// @brief Initialize angle generator
// @param [in,out] ag_var        angle generator struct to initialize
// @param [in]     freq          fixed frequency in Hz
// @param [in]     sampling_time loop period in seconds
// @return none
void InitAngleGen(AngleGenTypeDef *ag_var, int freq, float sampling_time)
{
    // Start angle at zero
    ag_var->theta         = 0.0f;

    // Convert frequency in Hz to angular frequency in rad/s
    ag_var->omega         = 2.0f * PI * freq;

    ag_var->sampling_time = sampling_time;
}

// @brief Run one angle generation cycle, integrates omega to produce theta
// @param [in,out] ag_var    angle generator struct
// @return none
void GenerateAngle(AngleGenTypeDef *ag_var)
{
    // Integrate angular frequency to get angle
    ag_var->theta += ag_var->omega * ag_var->sampling_time;

    // Wrap angle to keep within 0 to 2*pi
    if (ag_var->theta > TWO_PI)
    {
        ag_var->theta -= TWO_PI;
    }
    if (ag_var->theta < 0.0f)
    {
        ag_var->theta += TWO_PI;
    }
}


