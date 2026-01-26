#ifndef FAN_H
#define FAN_H

#include "PWM/pwm.h"

/**
 * @brief Fan actuator controlled via PWM.
 */
class Fan {
public:
    /**
     * @param pin       GPIO pin used for PWM output.
     * @param frequency PWM frequency in Hz (e.g. 25000).
     * @param duty      Initial duty cycle in range [0.0 .. 1.0].
     */
    Fan(uint pin, uint frequency = 25000, float duty = 0.0f);

    void fan_on();
    void fan_off();

    /** Set fan speed as duty cycle [0..1]. If duty==0, fan is considered OFF. */
    void set_speed(float duty);

    /** Change PWM frequency (Hz). Keeps current duty. */
    void set_frequency(uint frequency);

    bool is_on() const;
    float get_speed() const;

private:
    PWM pwm;
    bool opened;
    float duty;
    uint frequency;
};

#endif // FAN_H
