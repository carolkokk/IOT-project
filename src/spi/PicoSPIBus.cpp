//
// Created by Keijo Länsikunnas on 6.3.2025.
//

#include "PicoSPIBus.h"

#include <hardware/gpio.h>

PicoSPIBus::PicoSPIBus(unsigned int bus_nr, unsigned int clk_pin, unsigned int dout_pin, unsigned int din_pin,
                       SPI_config cfg) {
    if (bus_nr == 0) {
        spi = spi0;
    } else {
        spi = spi1;
    }
    spi_init(spi, cfg.speed);
    spi_set_format(spi, cfg.data_bits, cfg.cpol, cfg.cpha, SPI_MSB_FIRST);

    gpio_set_function(dout_pin, GPIO_FUNC_SPI);
    if (din_pin != PicoSPIBus::not_used)gpio_set_function(din_pin, GPIO_FUNC_SPI);
    gpio_set_function(clk_pin, GPIO_FUNC_SPI);
}

unsigned int PicoSPIBus::write(const uint8_t *buffer, unsigned int length) {
    return spi_write_blocking(spi, buffer, length);
}

unsigned int PicoSPIBus::read(uint8_t *buffer, unsigned int length) {
    return spi_read_blocking(spi, 0xFF, buffer, length);
}

unsigned int PicoSPIBus::transaction(const uint8_t *wbuffer, uint8_t *rbuffer, unsigned int length) {
    return spi_write_read_blocking(spi, wbuffer, rbuffer, length);
}
