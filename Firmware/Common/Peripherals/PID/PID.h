#ifndef PID_H
#define PID_H
/*---------------------------------- Includes ----------------------------------*/
#include "stdint.h"

#define DISABLED 0
#define ENABLED  1

/*---------------------------------- Types ----------------------------------*/
/**
 * @brief PID Controller struct
 */
typedef struct PID_s{
  int16_t  Kp;           // Proportional gain
  int16_t  Ki;           // Integral gain
  int16_t  Kd;           // Derivative gain
  int16_t  Ka;           // Anti-windup gain
  int32_t integral_min; // Integral minimum value
  int32_t integral_max; // Integral maximum value
  int32_t output_min;   // Output minimum value
  int32_t output_max;   // Output maximum value
  int32_t setpoint;     // Setpoint of the PID controller
  int32_t error;        // Error of the PID controller
  int32_t prev;         // Previous error of the PID controller
  int32_t proportional; // Proportional part of the PID controller
  int32_t integral;     // Integral part of the PID controller
  int32_t derivative;   // Derivative part of the PID controller
  int32_t output;       // Output of the PID controller
  uint32_t flagAntiWindup :1;  // Anti-windup triggered flag
}PID_t;

/*---------------------------------- Function Prototypes ----------------------------------*/

/**
 * @brief Set PID controller parameters
 */
void initPID(PID_t *pid, int16_t Kp, int16_t Ki, int16_t Kd, int16_t Ka, int32_t output_min, int32_t output_max);

/**
 * @brief Update PID controller according to feedback
 */
void updatePID(PID_t *pid, int32_t setpoint, int32_t feedback);

/**
 * @brief Reset PID controller to zero state
 * @note  Does not reset the Gain and limit parameters
 */
void resetPID(PID_t *pid);

#endif
