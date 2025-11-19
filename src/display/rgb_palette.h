//
// Created by Keijo Länsikunnas on 10.3.2025.
//

#ifndef RGB_PALETTE_H
#define RGB_PALETTE_H

#include "framebuf.h"

class rgb_palette : public framebuf {
public:
    explicit rgb_palette(int size, const uint32_t *color = nullptr);
    explicit rgb_palette(int size, uint32_t color); // first color is always set to zero (black-->black)
private:
    void setpixel(uint16_t x, uint16_t y, uint32_t color) override;
    uint32_t getpixel(uint16_t x, uint16_t y) const override;
    void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) override;

    uint32_t palette[16];
};



#endif //RGB_PALETTE_H
