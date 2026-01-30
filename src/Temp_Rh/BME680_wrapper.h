//
// Created by Julija Ivaske on 24.11.2025.
//

#ifndef HUMIBOX_BME680_WRAPPER_H
#define HUMIBOX_BME680_WRAPPER_H

#include "BME68x_SensorAPI/bme68x.h"
#include "i2c/PicoI2C.h"

struct BME_I2C_Context {
    PicoI2C *bus;
    uint8_t addr;
};

extern "C" {
int8_t bme_i2c_read(uint8_t reg, uint8_t *data, uint32_t len, void *intf_ptr);
int8_t bme_i2c_write(uint8_t reg, const uint8_t *data, uint32_t len, void *intf_ptr);
void   bme_delay_us(uint32_t us, void *intf_ptr);
}

#endif //HUMIBOX_BME680_WRAPPER_H