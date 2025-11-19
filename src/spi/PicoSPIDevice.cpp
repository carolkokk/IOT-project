//
// Created by Keijo Länsikunnas on 7.3.2025.
//

#include "PicoSPIDevice.h"

#include <hardware/gpio.h>

PicoSPIDevice::PicoSPIDevice(std::shared_ptr<PicoSPIBus> spi_bus, uint cs_pin) : spi_bus(spi_bus), cs_pin(cs_pin), cs(false) {
    if(cs_pin != PicoSPIBus::not_used) {
        cs = true;
        gpio_init(cs_pin);
        gpio_set_dir(cs_pin, GPIO_OUT);
        gpio_put(cs_pin, true);
    }
}

unsigned int PicoSPIDevice::write(const uint8_t *buffer, unsigned int length) {
    return spi_bus->write(buffer, length);
}

unsigned int PicoSPIDevice::read(uint8_t *buffer, unsigned int length) {
    return spi_bus->read(buffer, length);
}

unsigned int PicoSPIDevice::transaction(const uint8_t *wbuffer, uint8_t *rbuffer, unsigned int length) {
    return spi_bus->transaction(wbuffer, rbuffer, length);
}

void PicoSPIDevice::set_cs(bool value) {
    if(cs) gpio_put(cs_pin, value);
}
