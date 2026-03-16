//
// Created by Keijo Länsikunnas on 6.3.2025.
//

#ifndef PICOSPIBUS_H
#define PICOSPIBUS_H

#include <climits>
#include "hardware/spi.h"
#include "spi_device.h"

class PicoSPIBus {
public:
    static const unsigned int not_used = UINT_MAX;

    struct SPI_config {
        uint data_bits;
        spi_cpol_t cpol;
        spi_cpha_t cpha;
        spi_order_t order;
        uint speed;
    };

    explicit PicoSPIBus(unsigned int bus_nr, unsigned int clk_pin, unsigned int dout_pin,
                        unsigned int din_pin = PicoSPIBus::not_used, SPI_config cfg = {
                            8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 1000000
                        });

    PicoSPIBus(const PicoSPIBus &) = delete;
    unsigned int write(const uint8_t *buffer, unsigned int length);

    unsigned int read(uint8_t *buffer, unsigned int length);

    unsigned int transaction(const uint8_t *wbuffer, uint8_t *rbuffer, unsigned int lengt);

private:
    spi_inst_t *spi;
};


#endif //PICOSPIBUS_H
