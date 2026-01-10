#ifndef HUMIDIFIER_H
#define HUMIDIFIER_H

#include "PWM/PWM.h"

class Humidifier{
public:
    Humidifier(uint pin, uint frequency, float duty);

    void humidifier_on();
    void humidifier_off();
    bool check_humidifier_status() const;

private:
    PWM pwm;
    bool opened;
};


#endif //HUMIDIFIER_H
