#include "Fan.h"

Fan::Fan(uint8_t pin)
    // default mode: output, not inverted (MOSFET driven by logic HIGH)
    : gpio(pin, false, false, false), running(false)
{
    fan_off();   // ensure fan is OFF at startup
}

void Fan::fan_on() {
    gpio.write(true);
    running = true;
}

void Fan::fan_off() {
    gpio.write(false);
    running = false;
}

bool Fan::check_fan_status() const {
    return running;
}
