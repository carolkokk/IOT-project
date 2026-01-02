#ifndef PWM_H
#define PWM_H

#include "pico/stdlib.h"
#include "GPIO/GPIO.h"
#include "pico/types.h"
#include "hardware/pwm.h"

class PWM{
public:
    PWM(uint pin, uint frequency,float duty);
    void pwm_on();
    void pwm_off();
    void change_frequency(uint frequency);
    void change_duty(float duty);

private:
    uint pin;
    uint frequency;
    uint slice;
    uint channel;
    uint wrap;
    float duty;
};


#endif //PWM_H
