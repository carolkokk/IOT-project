//
// Created by Keijo Länsikunnas on 7.3.2025.
//

#ifndef PICOSPIDEVICE_H
#define PICOSPIDEVICE_H


#include <memory>
#include "spi_device.h"
#include "PicoSPIBus.h"

class PicoSPIDevice : public spi_device {
public:
    explicit PicoSPIDevice(std::shared_ptr<PicoSPIBus> spi_bus, uint cs_pin = PicoSPIBus::not_used);
    PicoSPIDevice(const PicoSPIDevice &) = delete;

    virtual unsigned int write(const uint8_t *buffer, unsigned int length) override;
    virtual unsigned int read(uint8_t *buffer, unsigned int length) override;
    virtual unsigned int transaction(const uint8_t *wbuffer, uint8_t *rbuffer, unsigned int length) override;
    virtual void set_cs(bool value) override;

private:
    std::shared_ptr<PicoSPIBus> spi_bus;
    uint cs_pin;
    bool cs;
};



#endif //PICOSPIDEVICE_H
