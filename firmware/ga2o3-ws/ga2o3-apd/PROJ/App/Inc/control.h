/**
 * @file control.h
 * @author lan
 * @brief implement control task
 * @version 0.1
 * @date 2026-03-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef __CONTROL_H__
#define __CONTROL_H__


typedef struct
{
    float error;
    float set_point;
    float sampling_time;

    float kp;

    float integral;
    float ki;
    float min_integral;
}pid ;


typedef struct
{
    float r;
    float theta;
 }polar ;


 typedef struct
 {
    float x;
    float y;
 }cart ;


 typedef struct
 {
    float alpha;
    float beta;
 }alphbeta ;

 typedef struct
 {
    float d;
    float q;
 }dq ;

typedef struct
{
    float input;
    float err;
    float alpha;
    float product_alpha_omega;
    float beta;
    float k;
    float sampling_time;
}sogi ;

typedef struct
{
int freq;
float sampling_time;
float theta;
float omega;

}angelgen ;






/**
 * @brief Initialize control related things
 * 
 * @return none
 */
void InitPIDControl();

/**
 * @brief The main control task, better put it into an interrupt
 * 
 * @return none
 */
float PIDControl(float measured_value, float set_point, float sampling_time);



/**
 * @brief The main control task, better put it into an interrupt
 * 
 * @return none
 */
 cart PolartoCartesian();


/**
 * @brief The main control task, better put it into an interrupt
 * 
 * @return none
 */
polar CartesiantoPolar(cart c);


/**
 * @brief The main control task, better put it into an interrupt
 * 
 * @return none
 */
 dq AlphabetaToDq(alphbeta alphabetavar);

/**
 * @brief The main control task, better put it into an interrupt
 * 
 * @return none
 */
 alphabeta DqToAlphabeta(dq dqvar);


#endif
