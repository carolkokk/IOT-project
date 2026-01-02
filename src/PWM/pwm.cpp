#include "PWM/PWM.h"
#include <cstdio>
#include "hardware/clocks.h"

//initiate PWM
PWM::PWM(uint pin, uint frequency,float duty)
    :pin(pin),frequency(frequency),duty(duty){
    //set the gpio as pwm channel
    gpio_set_function(pin,GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(pin);
    channel = pwm_gpio_to_channel(pin);
    wrap = (clock_get_hz(clk_sys) / frequency) - 1;

    //initial set up for PWM
    pwm_set_wrap(slice, wrap);
    pwm_off();
    pwm_set_enabled(slice, true);
}

void PWM::pwm_on(){
    pwm_set_chan_level(slice, channel, static_cast<uint16_t>(wrap * duty));
    printf("Set PWM Frequency: %d Hz (wrap=%u)\n", frequency, wrap);
}

void PWM::pwm_off(){
    pwm_set_chan_level(slice, channel, 0);
}

void PWM::change_frequency(uint freq)
{
    frequency = freq;
    wrap = (clock_get_hz(clk_sys) / frequency) - 1;
    pwm_set_wrap(slice, wrap);
    pwm_on();
}

void PWM::change_duty(float duty_val){
    //valid duty: 0-1
    if (duty_val < 0.0f) duty_val = 0.0f;
    if (duty_val > 1.0f) duty_val = 1.0f;

    duty = duty_val;
    pwm_on();
}