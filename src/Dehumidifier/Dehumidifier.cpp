#include "Dehumidifier.h"

Dehumidifier::Dehumidifier(uint8_t pin)
    //default mode: output, not inverted as MOSFET is driven by 1
    :gpio(pin,false,false,false), opened(false){
    dehum_off();
}

void Dehumidifier::dehum_on()
{
    gpio.write(true);
    opened = true;
}

void Dehumidifier::dehum_off()
{
    gpio.write(false);
    opened = false;
}

bool Dehumidifier::check_open() const
{
    return opened;
}