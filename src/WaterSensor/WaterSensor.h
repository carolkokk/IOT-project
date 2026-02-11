#ifndef WATER_SENSOR_H
#define WATER_SENSOR_H

#include <cstdint>
#include "pico/stdlib.h"

// Water sensor abstraction
// One object represents one physical water sensor
class WaterSensor {
public:
    // Constructor
    // gpio_pin: GPIO connected to sensor OUT
    // active_low:
    //   true  -> LOW means water detected
    //   false -> HIGH means water detected
    WaterSensor(uint gpio_pin, bool active_low = true);

    // Initialize GPIO hardware
    void Init();

    // Read sensor state
    // return true  -> water detected
    // return false -> no water
    bool Read() const;

private:
    uint m_pin;          // GPIO pin number
    bool m_active_low;   // Logic polarity
};

#endif // WATER_SENSOR_H