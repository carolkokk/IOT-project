//
// Created by Keijo Länsikunnas on 27.2.2025.
//

#ifndef ILI9341NOBUF_H
#define ILI9341NOBUF_H

#include <climits>
#include <memory>
#include <hardware/spi.h>
#include "framebuf.h"
#include "spi_device.h"

class ili9341nobuf : public framebuf {
public:
    explicit ili9341nobuf(std::shared_ptr<spi_device> dev, uint dc_pin, uint rst_pin = UINT_MAX, uint bl_pin = UINT_MAX,
                         uint16_t width = 240, uint16_t height = 320, uint8_t rotation = 3);
    void show();
private:
    void setpixel(uint16_t x, uint16_t y, uint32_t color) override;
    uint32_t getpixel(uint16_t x, uint16_t y) const override;
    void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) override;

    // private functions for handling the device
    void displayinit(const uint8_t *addr);
    void setrotation(uint8_t rotation);
    void command(uint8_t cmd, const uint8_t* data, size_t len);
    void write(const uint8_t *data, size_t len);
    void write(uint8_t value);
    void write(uint16_t value);
    void write(uint32_t value);
    void setaddrwindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void writecommand(uint8_t cmd);
    void set_dc(bool value) const;
    void set_rst(bool value) const;
    void set_bl(bool value) const;

    uint8_t _colstart = 0; ///< Some displays need this changed to offset
    uint8_t _rowstart = 0; ///< Some displays need this changed to offset
    uint8_t _colstart2 = 0; ///< Offset from the right
    uint8_t _rowstart2 = 0; ///< Offset from the bottom
    uint8_t _xstart = 0;
    uint8_t _ystart = 0;

    uint16_t windowWidth;
    uint16_t windowHeight;
    std::shared_ptr<spi_device> spi;
    uint gpio_dc;
    uint gpio_rst;
    uint gpio_bl;
};



#endif //ILI9341NOBUF_H
