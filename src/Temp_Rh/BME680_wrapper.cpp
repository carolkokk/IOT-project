//
// Created by Julija Ivaske on 24.11.2025.
//

#include <cstring>

#include "BME680_wrapper.h"
#include "PicoI2C.h"

extern "C" {
int8_t bme_i2c_read(uint8_t reg, uint8_t *data, uint32_t len, void *intf_ptr) {
    auto context = static_cast<BME_I2C_Context*>(intf_ptr);

    uint8_t regbuffer = reg;

    context->bus->write(context->addr, &regbuffer, 1);
    context->bus->read(context->addr, data, len);

    return BME68X_OK;
}

int8_t bme_i2c_write(uint8_t reg, uint8_t *data, uint32_t len, void *intf_ptr) {
    auto context = static_cast<BME_I2C_Context*>(intf_ptr);

    uint8_t buffer[1 + len];
    buffer[0] = reg;
    memcpy(buffer + 1, data, len);

    context->bus->write(context->addr, buffer, len + 1);

    return BME68X_OK;
}

void bme_delay_us(uint32_t us, void *) {
    sleep_us(us);
}
}