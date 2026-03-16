#include "Fan/Fan.h"

Fan::Fan(uint pin, uint frequency, float duty)
    : pwm(pin, frequency, duty), opened(false), duty(duty), frequency(frequency)
{
    // Start with fan OFF (duty forced to 0)
    set_speed(0.0f);
}

void Fan::fan_on() {
    // If speed was 0, default to 100%
    if (duty <= 0.0f) {
        set_speed(1.0f);
    } else {
        opened = true;
        pwm.change_duty(duty);
    }
}

void Fan::fan_off() {
    opened = false;
    duty = 0.0f;
    pwm.pwm_off();
}

void Fan::set_speed(float new_duty) {
    if (new_duty < 0.0f) new_duty = 0.0f;
    if (new_duty > 1.0f) new_duty = 1.0f;

    duty = new_duty;

    if (duty <= 0.0f) {
        fan_off();
        return;
    }

    opened = true;
    pwm.change_duty(duty);
}

void Fan::set_frequency(uint new_frequency) {
    frequency = new_frequency;
    pwm.change_frequency(new_frequency);
    // Re-apply duty to keep speed consistent
    if (opened && duty > 0.0f) {
        pwm.change_duty(duty);
    }
}

bool Fan::is_on() const {
    return opened;
}

float Fan::get_speed() const {
    return duty;
}
