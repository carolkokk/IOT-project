//
// Created by Julija Ivaske on 24.11.2025.
//

#ifndef HUMIBOX_BME680_H
#define HUMIBOX_BME680_H

#include <memory>
#include <i2c/PicoI2C.h>
#include "BME68x_SensorAPI/bme68x.h"

class BME680 {
    public:
        BME680(std::shared_ptr<PicoI2C> i2cbus, uint8_t address);
        double read_temp();
        double read_rh();
    private:
        std::shared_ptr<PicoI2C> i2c;
        uint8_t address;

        // structures that bme api expects
        struct bme68x_dev dev{};
        struct bme68x_conf conf{};
        struct bme68x_heatr_conf heat{};
};


#endif //HUMIBOX_BME680_H