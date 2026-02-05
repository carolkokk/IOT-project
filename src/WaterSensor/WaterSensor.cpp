#include "WaterSensor.h"

// Constructor
WaterSensor::WaterSensor(uint gpio_pin, bool active_low)
    : m_pin(gpio_pin), m_active_low(active_low) {
}

// Initialize GPIO
void WaterSensor::Init() {
    gpio_init(m_pin);
    gpio_set_dir(m_pin, GPIO_IN);

    // Enable internal pull-up for LM393 / open-drain sensors
    gpio_pull_up(m_pin);
}

// Read sensor state
bool WaterSensor::Read() const {
    // Raw GPIO level
    // true  -> HIGH
    // false -> LOW
    bool level_high = gpio_get(m_pin);

    // Convert to semantic meaning
    if (m_active_low) {
        return !level_high;   // LOW -> water detected
    } else {
        return level_high;    // HIGH -> water detected
    }
}
