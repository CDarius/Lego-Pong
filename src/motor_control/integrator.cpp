// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "motor_control/integrator.hpp"

/* Rate integrator used for speed-based control */
/**
Pause the rate integrator error accumulation and store current accumulated error

:param time_now: Actual time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
*/
void pbio_rate_integrator_pause(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Pause only if running
    if (!itg->running) {
        return;
    }

    // The integrator is not running anymore
    itg->running = false;

    // Increment the paused integrator state with the integrated amount between the last resume and the newly enforced pause
    itg->rate_err_integral_paused += count_ref - itg->count_ref_resumed - count + itg->count_resumed;

    // Store time at which we started pausing
    itg->time_pause_begin = time_now;
}

/**
 Resume the rate integral error calculation

:param time_now: Actual time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
 */
void pbio_rate_integrator_resume(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Resume only if paused
    if (itg->running) {
        return;
    }

    // The integrator is running again
    itg->running = true;

    // Set starting point from which we resume, if needed
    // Begin integrating again from the current point
    itg->count_ref_resumed = count_ref;
    itg->count_resumed = count;
}

/**
Reset the rate acculated error and restart the integrator

:param time_now: Actual time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
 */
void pbio_rate_integrator_reset(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Set integral to 0
    itg->rate_err_integral_paused = 0;

    // Set state to paused
    itg->running = false;

    // Resume integration
    pbio_rate_integrator_resume(itg, time_now, count, count_ref);
}

/**
Compute the speed instantaneus and integral error

:param rate: Speed actual value (count/s)
:param rate_ref: Speed setpoint value (count/s)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
:param rate_err: Return istantaneus speed error
:param rate_err_integral: Return accumulated speed error (integral)
 */
void pbio_rate_integrator_get_errors(pbio_rate_integrator_t *itg,
                                int32_t rate,
                                int32_t rate_ref,
                                int32_t count,
                                int32_t count_ref,
                                int32_t *rate_err,
                                int32_t *rate_err_integral) {

    // The rate error is simply the instantaneous error
    *rate_err = rate_ref - rate;

    // The rate error integral is at least the value at which we paused it last
    *rate_err_integral = itg->rate_err_integral_paused;

    // If integrator is active, add the exact integral since its last restart
    if (itg->running) {
        *rate_err_integral += (count_ref - itg->count_ref_resumed) - (count - itg->count_resumed);
    }
}

/**
Return true when the motor is in stall (rate integrator)

:param time_now: Actual time (us)
:param rate: Speed actual value (count/s)
:param time_stall: Minimum pause time before test for stall (us)
:param rate_stall: Stall speed threshold (count/s)
:return: Return true when the motor is in stall
 */
bool pbio_rate_integrator_stalled(const pbio_rate_integrator_t *itg, int32_t time_now, int32_t rate, int32_t time_stall, int32_t rate_stall) {
    // If were running, we're not stalled
    if (itg->running) {
        return false;
    }

    // If we're still running faster than the stall limit, we're certainly not stalled.
    if (abs(rate) > rate_stall) {
        return false;
    }

    // If the integrator is paused for less than the stall time, we're still not stalled for now.
    if (time_now - itg->time_pause_begin < time_stall) {
        return false;
    }

    // All checks have failed, so we are stalled
    return true; 
}


/* Count integrator used for position-based control */
/**
Return current time when is running or the pause time when is not running

:param time_now: Current time (us)
:return: Integrator reference time (us)
 */
int32_t pbio_count_integrator_get_ref_time(const pbio_count_integrator_t *itg, int32_t time_now) {
    // The wall time at which we are is either the current time, or whenever we stopped last
    int32_t real_time = itg->trajectory_running ? time_now : itg->time_pause_begin;

    // But we want to evaluate the reference compensating for the time we spent waiting
    return real_time - itg->time_paused_total;
}

/**
Pause the count integrator

:param time_now: Current time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
 */
void pbio_count_integrator_pause(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Return if already paused
    if (!itg->trajectory_running) {
        return;
    }

    // Disable the integrator
    itg->trajectory_running = false;
    itg->time_pause_begin = time_now;
}

/**
Resume the count integrator

:param time_now: Current time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
 */
void pbio_count_integrator_resume(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Return if already trajectory_running
    if (itg->trajectory_running) {
        return;
    }

    // Then we must restart the time
    itg->trajectory_running = true;

    // Increment total wait time by time elapsed since we started pausing
    itg->time_paused_total += time_now - itg->time_pause_begin;

}

/**
Reset and restart the count integrator

:param time_now: Current time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
:param max: Integrator max value
 */
void pbio_count_integrator_reset(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref, int32_t max) {

    // Reset integrator state variables
    itg->count_err_integral = 0;
    itg->time_paused_total = 0;
    itg->time_prev = time_now;
    itg->time_pause_begin = time_now;
    itg->count_err_prev = 0;
    itg->trajectory_running = false;
    itg->count_err_integral_max = max;

    // Resume integration
    pbio_count_integrator_resume(itg, time_now, count, count_ref);

}

/**
Update the integrator error if running

:param time_now: Current time (us)
:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
:param count_target: Motion target position (count)
:param integral_range: Distance from target when the integrator can accumulate error (count)
:param integral_rate: Maximum integral change per update (count)
 */
void pbio_count_integrator_update(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref, int32_t count_target, int32_t integral_range, int32_t integral_rate) {
    // Integrate and update position error
    if (itg->trajectory_running) {

        // Previous error will be multiplied by time delta and then added to integral (unless we limit growth)
        int32_t cerr = itg->count_err_prev;

        // Check if integrator magnitude would decrease due to this error
        bool decrease = abs(itg->count_err_integral + cerr*(time_now - itg->time_prev)) < abs(itg->count_err_integral);
        
        // If not deceasing, so growing, limit error growth by maximum integral rate
        if (!decrease) {
            cerr = cerr >  integral_rate ?  integral_rate : cerr;
            cerr = cerr < -integral_rate ? -integral_rate : cerr;

            // It might be decreasing now after all (due to integral sign change), so re-evaluate
            decrease = abs(itg->count_err_integral + cerr*(time_now - itg->time_prev)) < abs(itg->count_err_integral);
        }

        // Add change if we are near target, or always if it decreases the integral magnitude
        if (abs(count_target - count_ref) <= integral_range || decrease) {
            itg->count_err_integral += cerr*(time_now - itg->time_prev);
        }

        // Limit integral to predefined bound
        if (itg->count_err_integral > itg->count_err_integral_max) {
            itg->count_err_integral = itg->count_err_integral_max;
        }
        if (itg->count_err_integral < -itg->count_err_integral_max) {
            itg->count_err_integral = -itg->count_err_integral_max;
        }
    }

    // Keep the error and time for use in next update
    itg->count_err_prev = count_ref - count;
    itg->time_prev = time_now;
}

/**
Calculate istantaneus and integral count error

:param count: Encoder count actual value (count)
:param count_ref: Encoder expected count (count)
:param count_err: Return istantaneus poistion error (count)
:param count_err_integral: Return accumulated position error (count)
 */
void pbio_count_integrator_get_errors(pbio_count_integrator_t *itg, int32_t count, int32_t count_ref, int32_t *count_err, int32_t *count_err_integral) {
    // Calculate current error state
    *count_err = count_ref - count;
    *count_err_integral = itg->count_err_integral;
}

/**
Return true when the motor is in stall (count integrator)

:param time_now: Current time (us)
:param rate: Speed actual value (count/s)
:param time_stall: Minimum pause time before test for stall (us)
:param rate_stall: Stall speed threshold (count/s)
:return: True when the motor is in stall (count integrator)
 */
bool pbio_count_integrator_stalled(const pbio_count_integrator_t *itg, int32_t time_now, int32_t rate, int32_t time_stall, int32_t rate_stall) {
    // If we're running and the integrator is not saturated, we're not stalled
    if (itg->trajectory_running && abs(itg->count_err_integral) < itg->count_err_integral_max) {
        return false;
    }

    // If we're still running faster than the stall limit, we're certainly not stalled.
    if (abs(rate) > rate_stall) {
        return false;
    }

    // If the integrator is paused for less than the stall time, we're still not stalled for now.
    if (time_now - itg->time_pause_begin < time_stall) {
        return false;
    }

    // All checks have failed, so we are stalled
    return true; 
};
