#include "Humidifier.h"

Humidifier::Humidifier(uint pin, uint frequency, float duty)
    :pwm(pin,frequency,duty)
{
    //make sure that the humidifier is off in the beginning
    opened = false;
    humidifier_off();
}

void Humidifier::humidifier_on(){
    opened = true;
    pwm.pwm_on();
}

void Humidifier::humidifier_off(){
    opened = false;
    pwm.pwm_off();
}

bool Humidifier::check_humidifier_status() const{
    return opened;
}
