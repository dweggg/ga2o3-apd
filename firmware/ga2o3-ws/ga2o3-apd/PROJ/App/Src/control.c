#include "control.h"
#include <math.h>


void InitPIDControl(pid *pid_var,float set_point,float kp, float ki,float limit, float sampling_time)
{
    //Initization to zero
    pid_var->error = 0.0f;
    pid_var->integral = 0.0f;
    pid_var->output = 0.0f;

    //Setup Values
    pid_var->kp = kp ;
    pid_var->ki = ki;
    pid_var->set_point = set_point;
    pid_var->min_integral = -limit;
    pid_var->max_integral = limit;
    pid_var->sampling_time = sampling_time;
}

void PIDControl(pid *pid_var, float measured_value)
{
    
    pid_var->error = pid_var->set_point - measured_value;             // Calculation Error 

    pid_var->integral += pid_var->error *  pid_var->sampling_time;   

    if (pid_var->integral > pid_var->max_integral)
    pid_var->integral = pid_var->max_integral;           

    else if (pid_var->integral < pid_var->min_integral)
    {
        pid_var->integral = pid_var->min_integral;
    }


    pid_var->output = (pid_var->kp * pid_var->error) + (pid_var->ki * pid_var->integral); // Calculating PID output 
  

    return;
}

cart PolartoCartesian(polar p)
{
    cart c ={0};
    c.x = p.r*cosf(p.theta);
    c.y = p.r*sinf(p.theta);
    return c ;
}

polar CartesiantoPolar(cart c)
{
    polar p = {0};
    p.r = sqrtf(c.x * c.x + c.y * c.y);
    p.theta = atan2f(c.y, c.x);

    return p;
}
dq AlphabetaToDq(alphabeta alphabeta_var, float theta)
{
    dq dq_var = {0};
    dq_var.d =  alphabeta_var.alpha * cosf(theta) + alphabeta_var.beta * sinf(theta);
    dq_var.q = -alphabeta_var.alpha * sinf(theta) + alphabeta_var.beta * cosf(theta);
    return dq_var;

}

alphabeta DqToAlphabeta(dq dq_var, float theta)
{
    alphabeta alphabeta_var = {0};
    alphabeta_var.alpha = dq_var.d * cosf(theta) - dq_var.q * sinf(theta);
    alphabeta_var.beta  = dq_var.d * sinf(theta) + dq_var.q * cosf(theta);
    return alphabeta_var;
}


void InitSogiPll(sogi *sogi_var,float gain, float sampling_time)
{
    sogi_var->alpha = 0.0f;
    sogi_var->beta = 0.0f;
    sogi_var->k = gain; 
    sogi_var->sampling_time = sampling_time;

return ;    
}

void SogiPll(sogi *sogi_var,float input,float omega)
{
    sogi_var->err = input*sogi_var->k- sogi_var->alpha;
    sogi_var->alpha += ((sogi_var->err - sogi_var->beta)*omega)*sogi_var->sampling_time;    //alpha calculation 
    sogi_var->beta += sogi_var->alpha*omega*sogi_var->sampling_time;                        //beta calculation       
    return;

}

typedef struct
{
    int freq;
    float sampling_time;
    float theta;
    float omega;

}angelgen ;

void InitAngleGen(angelgen *ag_var, int freq, float sampling_time)
{
    ag_var->theta = 0;
    ag_var->omega = 2*PI*freq;
    ag_var->sampling_time = sampling_time;
}
void AngleGen(angelgen *ag_var)
{
    
    ag_var->theta = ag_var->theta + ag_var->omega * ag_var->sampling_time;
        
    if (ag_var->theta > 2.0f * PI)
        ag_var->theta -= 2.0f * PI;
    if (ag_var->theta < 0.0f)
        ag_var->theta += 2.0f * PI;

}


