#ifndef FAN_H
#define FAN_H

#include "pico/stdlib.h"
#include "GPIO/GPIO.h"

class Fan {
public:
    Fan(uint8_t pin);

    void fan_on();
    void fan_off();
    bool check_fan_status() const;

private:
    GPIO gpio;
    bool running;
};

#endif // FAN_H
