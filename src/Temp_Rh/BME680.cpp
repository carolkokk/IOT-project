//
// Created by Julija Ivaske on 24.11.2025.
//

#include "BME680.h"
#include "BME680_wrapper.h"

BME680::BME680(std::shared_ptr<PicoI2C> i2cbus, uint8_t address):
    i2c(i2cbus), address(address) {

    auto *context = new BME_I2C_Context{i2cbus.get(), address};

    dev.intf = BME68X_I2C_INTF;
    dev.read = bme_i2c_read;
    dev.write = bme_i2c_write;
    dev.delay_us = bme_delay_us;
    dev.intf_ptr = context;

    //initialize
    bme68x_init(&dev);

    //read default configuration
    bme68x_get_conf(&conf, &dev);

    //oversampling settings
    conf.os_temp = BME68X_OS_2X;
    conf.os_hum = BME68X_OS_4X;
    conf.os_pres = BME68X_OS_1X;

    bme68x_set_conf(&conf, &dev);

    //disabling heater cause gas is not measured
    heat.enable = BME68X_DISABLE;
    bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heat, &dev);
}

bool BME680::read_data(double &temp, double &rh) {
    bme68x_set_op_mode(BME68X_FORCED_MODE, &dev);
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t n_fields;
    struct bme68x_data data{};
    int8_t result = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &dev);
    if (result != BME68X_OK || n_fields == 0) return false;

    temp = data.temperature;
    rh = data.humidity;
    return true;
}

/*double BME680::read_temp() {
    bme68x_set_op_mode(BME68X_FORCED_MODE, &dev);
    vTaskDelay(100);

    uint8_t n_fields;
    struct bme68x_data data{};

    bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &dev);

    return data.temperature;
}

double BME680::read_rh()
{
    bme68x_set_op_mode(BME68X_FORCED_MODE, &dev);
    sleep_ms(100);

    uint8_t n_fields;
    struct bme68x_data data{};

    bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &dev);

    return data.humidity;
}*/

