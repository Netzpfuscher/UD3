/**
 * \file FastPID.h
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
 */

#ifndef _fastpid_h
#define _fastpid_h

#include <stdint.h>
#include <stdbool.h>

#define INTEG_MAX    (INT32_MAX)
#define INTEG_MIN    (INT32_MIN)
#define DERIV_MAX    (INT16_MAX)
#define DERIV_MIN    (INT16_MIN)

#define PARAM_SHIFT  8
#define PARAM_BITS   16
#define PARAM_MAX    (((0x1ULL << PARAM_BITS)-1) >> PARAM_SHIFT) 
#define PARAM_MULT   (((0x1ULL << PARAM_BITS)) >> (PARAM_BITS - PARAM_SHIFT)) 

/**
 * A fixed-point PID controller with 32-bit internal calculation pipeline.
 */
typedef struct FastPID {
    uint32_t _p,
             _i,
             _d;
    int32_t _anti_wind_max,
            _anti_wind_min;
    int64_t _outmax,    //!< maximum output value
            _outmin;    //!< minimum output value
    bool _cfg_err;      //!< `true` if pid parameters were invalid

    int16_t _last_sp,   //!< setpoint in previous call to `pid_step()`
            _last_out;  //!< output in previous call to `pid_step()`
    int64_t _sum;       //!< `I` term of pid equation
    int32_t _last_err;  //!< error in previous call to `pid_step()`
} FastPID;

/**
 * Create a new FastPID controller with the given parameters.  Interally, this
 * is simply a wrapper around `pid_configure()`.
 */
void pid_new(FastPID *pid, float kp, float ki, float kd,
        float hz, int bits, bool sign);

/**
 * Set the PID coefficients. The coefficients ki and kd are scaled by
 * the value of hz. The hz value informs the PID of the rate you will
 * call step(). Calling code is responsible for calling step() at the
 * rate in hz. Returns false if a configuration error has occured.
 */
bool pid_set_coefficients(FastPID *pid, float kp, float ki, float kd, float hz);

/**
 * Set the ouptut configuration by bits/sign. The ouput range will be:
 *
 * For signed equal to true:
 *
 *     2^(n-1) - 1 down to -2^(n-1)
 *
 * For signed equal to false:
 *
 *     2^n-1 down to 0
 */
bool pid_set_output(FastPID *pid, int bits, bool sign);

/**
 * Set the ouptut range directly. The effective range is:
 *
 *      Min: -32768 to 32766
 *      Max: -32767 to 32767
 *
 * Min must be greater than max.
 *
 * Returns false if a configuration error has occured. 
 */
bool pid_set_limits(FastPID *pid, int16_t min, int16_t max);

/**
 * Reset the controller. This should be done before changing the
 * configuration in any way.
 */
void pid_clear(FastPID *pid);

/**
 * Bulk configure the controller. Equivalent to:
 *
 *      pid_clear(pid);
 *      pid_set_coefficients(pid, kp, ki, kd, hz);
 *      pid_set_output(pid, bits, sign);
 */
bool pid_configure(FastPID *pid, float kp, float ki, float kd,
        float hz, int bits, bool sign);

/**
 * Run a single step of the controller and return the next output.
 */
int16_t pid_step(FastPID *pid, int16_t sp, int16_t fb);

/**
 * Test for a confiuration error. The controller will not run if this
 * function returns true.
 */
bool pid_err(FastPID *pid);

/**
 * Convert a floating point parameter to the equivalent integer value.
 */
uint32_t pid_float_to_param(FastPID *pid, float in);

void pid_set_anti_windup(FastPID *pid, int32_t min, int32_t max);

#endif // _fastpid_h
