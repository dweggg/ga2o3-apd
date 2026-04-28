// @file control_math.c
// @author Arnold
// @brief implement control task
// @version 1.0
// @date 2026-04-06
//
// @copyright Copyright (c) 2026

#include "control_math.h"
#include <math.h>

// @brief Initialize PID controller to zero and set gains
// @param [in,out] pid_var       PID struct to initialize
// @param [in]     kp            proportional gain
// @param [in]     ki            integral gain
// @param [in]     sampling_time loop period in seconds
// @return none
void InitPiControl(PiTypeDef *pid_var, float kp, float ki, float sampling_time)
{
    // Initialize outputs and state to zero
    pid_var->error_k         = 0.0f;
    pid_var->error_k1         = 0.0f;
    pid_var->integral_k      = 0.0f;
    pid_var->integral_k1      = 0.0f;
    pid_var->output        = 0.0f;
    pid_var->set_point     = 0.0f;

    // Store gains
    pid_var->kp            = kp;
    pid_var->ki            = ki;

    // Store setpoint and timing
    pid_var->sampling_time = sampling_time;

    // Apply integral clamp symmetrically around zero
    pid_var->limit_p         = 0.0f;
    pid_var->limit_n         = 0.0f;
}


// @brief Run one PID cycle, call at fixed sampling_time interval
// @param [in,out] pid_var        PID struct
// @param [in]     set_point      target reference value
// @param [in]     measured_value current process measurement
// @param [in]     limit          integral clamp value, applied as +/- limit
// @return none
void RunPiControl(PiTypeDef *pid_var, float set_point, float measured_value, float limit_p, float limit_n)
{
    pid_var->set_point   = set_point;
    pid_var->limit_p     = limit_p;
    pid_var->limit_n     = limit_n;

    // Calculate error between target and measured
    pid_var->error_k = pid_var->set_point - measured_value;

    // Accumulate error over time — integral term
    pid_var->integral_k = pid_var->ki * pid_var->sampling_time * (pid_var->error_k + (pid_var->output - pid_var->output_presat)/pid_var->kp) + pid_var->integral_k1;

    // Sum P and I contributions to produce output
    pid_var->output_presat = (pid_var->kp * pid_var->error_k) + pid_var->integral_k;

    // Clamp integral to prevent windup
    if (pid_var->output_presat > pid_var->limit_p)
    {
        pid_var->output = pid_var->limit_p;
    }
    else if (pid_var->output_presat < pid_var->limit_n)
    {
        pid_var->output = pid_var->limit_n;
    }
    else
    {
        pid_var->output = pid_var->output_presat;
    }

    pid_var->error_k1 = pid_var->error_k;
    pid_var->integral_k1 = pid_var->integral_k;
    
    return;
}


/* -----------------------------------------------------------------------
 * Rate limiter
 * -------------------------------------------------------------------- */

// @brief Initialize rate limiter to a known output value
// @param [in,out] rl_var        rate limiter struct to initialize
// @param [in]     initial_value starting output — set to current plant state
//                               to avoid a step kick on the first call
// @param [in]     rate          maximum slew rate in units/second (symmetric ±)
// @param [in]     sampling_time loop period in seconds
// @return none
void InitRateLimiter(RateLimiterTypeDef *rl_var, float initial_value, float rate, float sampling_time)
{
    rl_var->output        = initial_value;
    rl_var->rate          = rate;
    rl_var->sampling_time = sampling_time;
}

// @brief Run one rate-limiter cycle
//        Clamps the per-sample step to ±(rate * sampling_time) so the output
//        ramps toward the target at most 'rate' units per second in both
//        directions (symmetric). Drop this in front of any reference inside
//        your control loop and read rl_var->output.
//
// @param [in,out] rl_var    rate limiter struct
// @param [in]     target    desired value to ramp toward
// @return                   current rate-limited output (also in rl_var->output)
float RunRateLimiter(RateLimiterTypeDef *rl_var, float target)
{
    float max_step = rl_var->rate * rl_var->sampling_time;
    float delta    = target - rl_var->output;

    if (delta > max_step)
    {
        delta = max_step;
    }
    else if (delta < -max_step)
    {
        delta = -max_step;
    }

    rl_var->output += delta;
    return rl_var->output;
}


/* -----------------------------------------------------------------------
 * Coordinate transforms
 * -------------------------------------------------------------------- */

// @brief Convert polar coordinates to cartesian
// @param [in] p    polar input (r, theta)
// @return          cartesian output (x, y)
CartTypeDef PolarToCartesian(PolarTypeDef p)
{
    CartTypeDef c = {0};

    // x = r * cos(theta), y = r * sin(theta)
    c.x = p.r * cosf(p.theta);
    c.y = p.r * sinf(p.theta);
    return c;
}

// @brief Convert cartesian coordinates to polar
// @param [in] c    cartesian input (x, y)
// @return          polar output (r, theta)
PolarTypeDef CartesianToPolar(CartTypeDef c)
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
DqTypeDef ConvertAlphabetaToDq(AlphaBetaTypeDef alphabeta_var, float theta)
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
AlphaBetaTypeDef ConvertDqToAlphabeta(DqTypeDef dq_var, float theta)
{
    AlphaBetaTypeDef alphabeta_var = {0};

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
void InitSogi(SogiTypeDef *sogi_var, float gain, float sampling_time)
{
    // Initialize integrator states to zero
    sogi_var->alpha         = 0.0f;
    sogi_var->beta          = 0.0f;

    // Store damping gain and timing
    sogi_var->k             = gain;
    sogi_var->sampling_time = sampling_time;
    return;
}

// @brief Run one SOGI cycle, generates quadrature alpha and beta signals
// @param [in,out] sogi_var  SOGI struct
// @param [in]     input     single phase AC input sample
// @param [in]     omega     angular frequency in rad/s
// @return none
void RunSogi(SogiTypeDef *sogi_var, float input, float omega)
{
    float err = (input * sogi_var->k) - sogi_var->alpha;
    // First integrator — alpha tracks the input in-phase
    sogi_var->alpha += ((err - sogi_var->beta) * omega) * sogi_var->sampling_time;
    // Second integrator — beta is 90 degrees shifted version of alpha
    sogi_var->beta  += sogi_var->alpha * omega * sogi_var->sampling_time;
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
