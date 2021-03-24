/**
 * \file FastPID.c
 *
 * This is a port of Mike Matera's
 * [FastPID](https://github.com/mike-matera/FastPID) code.  From Mike's
 * `README`:
 *
 * > This PID controller is faster than alternatives for Arduino because it avoids
 * > expensive floating point operations. The PID controller is configured with
 * > floating point coefficients and translates them to fixed point internally. This
 * > imposes limitations on the domain of the coefficients. Setting the I and D
 * > terms to zero makes the controller run faster. The controller is configured to
 * > run at a fixed frequency and calling code is responsible for running at that
 * > frequency. The Ki and Kd parameters are scaled by the frequency to save time
 * > during the step() operation.
 *
 * (See FastPID.h for function documentation.)
 */

#include <stdint.h>
#include <stdbool.h>

#include "FastPID.h"

void pid_new(FastPID *pid, float kp, float ki, float kd,
        float hz, int bits, bool sign) {
    pid_configure(pid, kp, ki, kd, hz, bits, sign);
    pid_set_anti_windup(pid,-32768,32768);
}

void pid_set_cfg_err(FastPID *pid) {
  pid->_cfg_err = true;
  pid->_p = pid->_i = pid->_d = 0;
}

uint32_t pid_float_to_param(FastPID *pid, float in) {
  if (in > PARAM_MAX || in < 0) {
    pid_set_cfg_err(pid);
    return 0;
  }

  uint32_t param = in * PARAM_MULT;

  if (in != 0 && param == 0) {
    pid_set_cfg_err(pid);
    return 0;
  }
  
  return param;
}

void pid_set_anti_windup(FastPID *pid, int32_t min, int32_t max){
    pid->_anti_wind_max = max * PARAM_MULT;   
    pid->_anti_wind_min = min * PARAM_MULT;
}

void pid_clear(FastPID *pid) {
  pid->_last_sp = 0; 
  pid->_last_out = 0;
  pid->_sum = 0; 
  pid->_last_err = 0;
  pid->_cfg_err = false;
} 

bool pid_set_coefficients(FastPID *pid, float kp, float ki, float kd, float hz) {
  pid->_p = pid_float_to_param(pid, kp);
  pid->_i = pid_float_to_param(pid, ki / hz);
  pid->_d = pid_float_to_param(pid, kd * hz);
  return ! pid->_cfg_err;
}

bool pid_set_output(FastPID *pid, int bits, bool sign) {
  // Set output bits
  if (bits > 16 || bits < 1) {
    pid_set_cfg_err(pid);
  }
  else {
    if (bits == 16) {
      pid->_outmax = (0xFFFFULL >> (17 - bits)) * PARAM_MULT;
    }
    else{
      pid->_outmax = (0xFFFFULL >> (16 - bits)) * PARAM_MULT;
    }
    if (sign) {
      pid->_outmin = -((0xFFFFULL >> (17 - bits)) + 1) * PARAM_MULT;
    }
    else {
      pid->_outmin = 0;
    }
  }
  return ! pid->_cfg_err;
}

bool pid_set_limits(FastPID *pid, int16_t min, int16_t max)
{
  if (min >= max) {
    pid_set_cfg_err(pid);
    return ! pid->_cfg_err;
  }
  pid->_outmin = (int64_t)(min) * PARAM_MULT;
  pid->_outmax = (int64_t)(max) * PARAM_MULT;
  return ! pid->_cfg_err;
}

bool pid_configure(FastPID *pid, float kp, float ki, float kd, float hz, int bits, bool sign) {
  pid_clear(pid);
  pid_set_coefficients(pid, kp, ki, kd, hz);
  pid_set_output(pid, bits, sign);
  return ! pid->_cfg_err; 
}

int16_t pid_step(FastPID *pid, int16_t sp, int16_t fb) {

  // int16 + int16 = int17
  int32_t err = (int32_t)(sp) - (int32_t)(fb);
  int32_t P = 0, I = 0;
  int32_t D = 0;

  if (pid->_p) {
    // uint16 * int16 = int32
    P = (int32_t)(pid->_p) * (int32_t)(err);
  }

  if (pid->_i) {
    // int17 * int16 = int33
    pid->_sum += (int64_t)(err) * (int64_t)(pid->_i);

    // Limit sum to 32-bit signed value so that it saturates, never overflows.
    if (pid->_sum > pid->_anti_wind_max)
      pid->_sum = pid->_anti_wind_max;
    else if (pid->_sum < pid->_anti_wind_min)
      pid->_sum = pid->_anti_wind_min;

    // int32
    I = pid->_sum;
  }

  if (pid->_d) {
    // (int17 - int16) - (int16 - int16) = int19
    int32_t deriv = (err - pid->_last_err) - (int32_t)(sp - pid->_last_sp);
    pid->_last_sp = sp; 
    pid->_last_err = err; 

    // Limit the derivative to 16-bit signed value.
    if (deriv > DERIV_MAX)
      deriv = DERIV_MAX;
    else if (deriv < DERIV_MIN)
      deriv = DERIV_MIN;

    // int16 * int16 = int32
    D = (int32_t)(pid->_d) * (int32_t)(deriv);
  }

  // int32 (P) + int32 (I) + int32 (D) = int34
  int64_t out = (int64_t)(P) + (int64_t)(I) + (int64_t)(D);

  // Make the output saturate
  if (out > pid->_outmax) 
    out = pid->_outmax;
  else if (out < pid->_outmin) 
    out = pid->_outmin;

  // Remove the integer scaling factor. 
  int16_t rval = out >> PARAM_SHIFT;

  // Fair rounding.
  if (out & (0x1ULL << (PARAM_SHIFT - 1))) {
    rval++;
  }

  pid->_last_out = rval;
  return rval;
}

bool pid_err(FastPID *pid) {
    return pid->_cfg_err;
}
