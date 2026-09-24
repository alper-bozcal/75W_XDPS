/*---------------------------------- Includes ----------------------------------*/
#include "PID.h"

/*---------------------------------- Function Implementations ----------------------------------*/

void initPID(PID_t *pid, int16_t Kp, int16_t Ki, int16_t Kd, int16_t Ka, int32_t output_min, int32_t output_max) {
  pid->Kp           = Kp;
  pid->Ki           = Ki;
  pid->Kd           = Kd;
  pid->Ka           = Ka;
  pid->integral_min = pid->output_min = output_min;
  pid->integral_max = pid->output_max = output_max;
  pid->setpoint     = 0;
  pid->error        = 0;
  pid->prev         = 0;
  pid->proportional = 0;
  pid->integral     = 0;
  pid->derivative   = 0;
  pid->flagAntiWindup = DISABLED;
}

void updatePID(PID_t *pid, int32_t setpoint, int32_t feedback) {
  // Store previous error
  pid->prev = pid->error;
  // Update setpoint
  pid->setpoint = setpoint;
  // Calculate error
  pid->error = pid->setpoint - feedback;
  
  // Calculate proportional, integral and derivative parts with scaling
  // Gains are scaled by 100 to allow fractional gains (e.g., Kp=5 means 0.05)
  pid->proportional = (pid->Kp * pid->error) / 100;
  
  // Integral term accumulation with scaling
  int32_t integral_increment = (pid->Ki * pid->error) / 100;
  pid->integral += integral_increment;
  
  // Derivative term with scaling
  pid->derivative = (pid->Kd * (pid->error - pid->prev)) / 100;
  
  // Anti-windup with scaled integral limits
  if (pid->integral > pid->integral_max){
    pid->integral = pid->integral_max;
    pid->flagAntiWindup = ENABLED;
  }else if (pid->integral < pid->integral_min){
    pid->integral = pid->integral_min;
    pid->flagAntiWindup = ENABLED;
  }else{
    pid->flagAntiWindup = DISABLED;
  }
  
  // Calculate output of the PID controller a.k.a PWM Duty Cycle
  pid->output = pid->proportional + pid->integral + pid->derivative;
  
  // Limit the output
  if (pid->output > pid->output_max) {pid->output = pid->output_max;}
  if (pid->output < pid->output_min) {pid->output = pid->output_min;}
}

void resetPID(PID_t *pid) {
  pid->proportional = 0;
  pid->integral     = 0;
  pid->derivative   = 0;
  pid->output       = 0;
  pid->error        = 0;
  pid->prev         = 0;
  pid->setpoint     = 0;
  pid->flagAntiWindup = DISABLED;
}
