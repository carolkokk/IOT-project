//
// Created by Julija Ivaske on 13.1.2026.
//

#ifndef HUMIBOX_ILI9341_H
#define HUMIBOX_ILI9341_H

#include <cstdint>
#include <memory>
#include "PicoSPIDevice.h"

class ili9341 {
    public:
        ili9341(std::shared_ptr<PicoSPIDevice> spi_dev,
                uint dc,
                uint rst = UINT_MAX,
                uint bl = UINT_MAX,
                uint16_t width = 240,
                uint16_t height = 320,
                uint8_t rotation = 3);

        // functions that lvgl library will use to display things
        void set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
        void draw_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint8_t *data, size_t len);


        //uint16_t width() const { return _width; }
        //uint16_t height() const { return _height; }

    private:
        void init(const uint8_t *addr);
        void setrotation(uint8_t rotation);
        void command(uint8_t cmd, const uint8_t* data, size_t len);
        void write(const uint8_t *data, size_t len);
        void write(uint8_t value);
        void write(uint16_t value);
        void write(uint32_t value);
        void writecommand(uint8_t cmd);
        void set_dc(bool value) const;
        void set_rst(bool value) const;
        void set_bl(bool value) const;

        std::shared_ptr<PicoSPIDevice> spi;
        uint gpio_dc;
        uint gpio_rst;
        uint gpio_bl;
        uint16_t windowWidth;
        uint16_t windowHeight;
        uint16_t width;
        uint16_t height;

        uint16_t _xstart;
        uint16_t _ystart;
};

#endif //HUMIBOX_ILI9341_H

