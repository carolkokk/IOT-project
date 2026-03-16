#ifndef DEHUMIDIFIER_H
#define DEHUMIDIFIER_H
#include "pico/stdlib.h"
#include "GPIO/GPIO.h"

class Dehumidifier{
public:
    Dehumidifier(uint8_t pin);
    void dehum_on();
    void dehum_off();
    bool check_open() const;

private:
    GPIO gpio;
    bool opened;
};

#endif //DEHUMIDIFIER_H
