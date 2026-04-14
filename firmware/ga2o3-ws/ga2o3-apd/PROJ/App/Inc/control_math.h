/**
 * @file control_math.h
 * @author Arnold
 * @brief implement control task
 * @version 1.0
 * @date 2026-04-06
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef __CONTROL_H__
#define __CONTROL_H__

#include <math.h>

#define PI     3.14159265358979f    // pi constant for angle calculations
#define TWO_PI 6.28318530717959f    // 2*pi used for angle wrapping (0 to 2pi)
/* -----------------------------------------------------------------------
 * Structs
 * -------------------------------------------------------------------- */

typedef struct
{
    float error;
    float set_point;
    float sampling_time;
    float kp;
    float ki;
    float integral;
    float limit;
    float output;
} PiTypeDef;

typedef struct
{
    float r;
    float theta;
} PolarTypeDef;

typedef struct
{
    float x;
    float y;
} CartTypeDef;

typedef struct
{
    float alpha;
    float beta;
} AlphaBetaTypeDef;

typedef struct
{
    float d;
    float q;
} DqTypeDef;

typedef struct
{
    float alpha;
    float beta;
    float k;
    float sampling_time;
} SogiTypeDef;

typedef struct
{
    int   freq;
    float sampling_time;
    float theta;
    float omega;
} AngleGenTypeDef;


/* -----------------------------------------------------------------------
 * PID
 * -------------------------------------------------------------------- */

/**
 * @brief Initialize PID controller to zero and set gains
 *
 * @param [in,out] pid_var        PID struct to initialize
 * @param [in]     kp             proportional gain
 * @param [in]     ki             integral gain
 * @param [in]     limit          integral clamp value, applied as +/- limit
 * @param [in]     sampling_time  loop period in seconds
 */
void InitPiControl(PiTypeDef *pid_var, float kp, float ki, float limit, float sampling_time);

/**
 * @brief Run one PID cycle, call at fixed sampling_time interval
 *
 * @param [in,out] pid_var          PID struct
 * @param [in]     set_point        target reference value
 * @param [in]     measured_value   current process measurement
 */
void RunPiControl(PiTypeDef *pid_var, float set_point, float measured_value);


/* -----------------------------------------------------------------------
 * Coordinate transforms
 * -------------------------------------------------------------------- */

/**
 * @brief Convert polar coordinates to cartesian
 *
 * @param [in] p    polar input (r, theta)
 * @return          cartesian output (x, y)
 */
CartTypeDef PolarToCartesian(PolarTypeDef p);

/**
 * @brief Convert cartesian coordinates to polar
 *
 * @param [in] c    cartesian input (x, y)
 * @return          polar output (r, theta)
 */
PolarTypeDef CartesianToPolar(CartTypeDef c);


/* -----------------------------------------------------------------------
 * Frame transforms
 * -------------------------------------------------------------------- */

/**
 * @brief Park transform — stationary alphabeta frame to rotating dq frame
 *
 * @param [in] alphabeta_var    alphabeta frame input
 * @param [in] theta            rotor electrical angle in radians
 * @return                      dq frame output
 */
DqTypeDef ConvertAlphabetaToDq(AlphaBetaTypeDef alphabeta_var, float theta);

/**
 * @brief Inverse Park transform — rotating dq frame to stationary alphabeta frame
 *
 * @param [in] dq_var   dq frame input
 * @param [in] theta    rotor electrical angle in radians
 * @return              alphabeta frame output
 */
AlphaBetaTypeDef ConvertDqToAlphabeta(DqTypeDef dq_var, float theta);


/* -----------------------------------------------------------------------
 * SOGI
 * -------------------------------------------------------------------- */

/**
 * @brief Initialize SOGI struct
 *
 * @param [in,out] sogi_var      SOGI struct to initialize
 * @param [in]     gain          damping gain k, typically sqrt(2) = 1.414
 * @param [in]     sampling_time loop period in seconds
 */
void InitSogi(SogiTypeDef *sogi_var, float gain, float sampling_time);

/**
 * @brief Run one SOGI cycle, generates quadrature alpha and beta signals
 *
 * @param [in,out] sogi_var  SOGI struct
 * @param [in]     input     single phase AC input sample
 * @param [in]     omega     angular frequency in rad/s
 */
void RunSogi(SogiTypeDef *sogi_var, float input, float omega);


/* -----------------------------------------------------------------------
 * Angle generation
 * -------------------------------------------------------------------- */

/**
 * @brief Initialize angle generator
 *
 * @param [in,out] ag_var        angle generator struct to initialize
 * @param [in]     freq          fixed frequency in Hz
 * @param [in]     sampling_time loop period in seconds
 */
void InitAngleGen(AngleGenTypeDef *ag_var, int freq, float sampling_time);

/**
 * @brief Run one angle generation cycle, integrates omega to produce theta
 *
 * @param [in,out] ag_var    angle generator struct
 */
void GenerateAngle(AngleGenTypeDef *ag_var);


#endif /* __CONTROL_H__ */
