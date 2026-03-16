//
// Created by Julija Ivaske on 13.1.2026.
//

#ifndef HUMIBOX_ILI9341_H
#define HUMIBOX_ILI9341_H

#include <cstdint>
#include <memory>
#include "PicoSPIDevice.h"
#include "Display.h"

class ili9341 : public Display {
    public:
        ili9341(std::shared_ptr<PicoSPIDevice> spi_dev,
                uint dc,
                uint rst = UINT_MAX,
                uint bl = UINT_MAX,
                uint16_t width = 240,
                uint16_t height = 320,
                uint8_t rotation = 3);

        void draw_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint8_t *data, size_t len) override;

        uint16_t get_width()  const override { return width; }
        uint16_t get_height() const override { return height; }

    private:
        void init(const uint8_t *addr);
        void setrotation(uint8_t rotation);
        void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
        void command(uint8_t cmd, const uint8_t* data, size_t len);
        void write(const uint8_t *data, size_t len);
        void write(uint8_t value);
        void write(uint16_t value);
        void write(uint32_t value);
        void set_dc(bool value) const;
        void set_rst(bool value) const;
        void set_bl(bool value) const;

        std::shared_ptr<PicoSPIDevice> spi;  // shared ownership
        uint gpio_dc;
        uint gpio_rst;
        uint gpio_bl;
        uint16_t _baseWidth;   // physical panel dimensions (before rotation)
        uint16_t _baseHeight;
        uint16_t width;        // logical dimensions (after rotation)
        uint16_t height;

        uint16_t _xstart;
        uint16_t _ystart;
};

#endif //HUMIBOX_ILI9341_H

