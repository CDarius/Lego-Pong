#ifndef __MOTOR_CONTROL_MOTOR_HPP_
#define __MOTOR_CONTROL_MOTOR_HPP_

#include <Arduino.h>
#include "config.h"
#include "motor_control/enums.h"
#include "motor_control/error.hpp"
#include "motor_control/servo.hpp"
#include "motor_control/tacho.hpp"
#include "motor_control/dcmotor.hpp"

class Motor {
    private:
        Tacho _tacho = Tacho();
        DCMotor _dcmotor = DCMotor();
        pbio_servo_t _servo;
        pbio_error_t _servo_status = PBIO_SUCCESS;

        SemaphoreHandle_t _xMutex = xSemaphoreCreateMutex();

        float swLimitM, swLimitP;
        float homeSwitchPos;

    public:
        void begin(
            uint8_t encoderPi1,
            uint8_t encoderPi2,
            uint8_t pwmPi1,
            uint8_t pwmPi2,
            pbio_direction_t direction, 
            float gear_ratio, 
            pbio_control_settings_t *settings);

        float angle() const;
        void reset_angle(float angle);
        float speed() const;

        void stop();
        void brake();
        void hold();

        void dc(float duty);
        pbio_error_t run(float speed);
        pbio_error_t run_time(float speed, uint32_t time_ms, pbio_actuation_t then = PBIO_ACTUATION_HOLD, bool wait = true);
        pbio_error_t run_until_stalled(float speed, float duty_limit = 100.0, pbio_actuation_t then = PBIO_ACTUATION_COAST);
        pbio_error_t run_angle(float speed, float angle, pbio_actuation_t then = PBIO_ACTUATION_HOLD, bool wait = true);
        pbio_error_t run_target(float speed, float target_angle, pbio_actuation_t then = PBIO_ACTUATION_HOLD, bool wait = true);
        void track_target(float target_angle);
        pbio_error_t wait_for_completion();

        void update();

        float get_speed_limit();
        float get_acceleration_limit();
        uint8_t get_actuation_limit();
        pbio_error_t set_speed_limit(float speed);
        pbio_error_t set_acceleration_limit(float acceleration);
        pbio_error_t set_actuation_limit(uint8_t actuation);        
        void get_pid(uint16_t *kp, uint16_t *ki, uint16_t *kd, float *integral_deadzone, float *integral_rate) const;
        pbio_error_t set_pid(uint16_t kp, uint16_t ki, uint16_t kd, float integral_deadzone, float integral_rate);
        void get_target_tolerances(float *speed, float *position) const;
        pbio_error_t set_target_tolerances(float speed, float position);
        void get_stall_tolerances(float *speed, uint32_t *time_ms) const;
        pbio_error_t set_stall_tolerances(float speed, uint32_t time_ms);

        float getSwLimitMinus() const {
            return swLimitM;
        }

        void setSwLimitMinus(float value) {
            swLimitM = value;
        }

        float getSwLimitPlus() const {
            return swLimitP;
        }

        void setSwLimitPlus(float value) {
            swLimitP = value;
        }

        float getHomeSwitchPos() const {
            return homeSwitchPos;
        }

        void setHomeSwithcPos(float value) {
            homeSwitchPos = value;
        }
};

#endif