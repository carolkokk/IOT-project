//
// Created by Keijo Länsikunnas on 10.3.2025.
//

#include "rgb_palette.h"

#include <cstring>

rgb_palette::rgb_palette(int size, const uint32_t *color) : framebuf(size > 16 ? 16 : size, 1) {
    if(color)
        for(int i = 0; i < size; ++i) palette[i] = color[i];
    else
        fill_rect(0, 0, width, 1, 0);
}

rgb_palette::rgb_palette(int size, uint32_t color) : framebuf(size > 16 ? 16 : size, 1) {
    for(auto &c : palette) c = color;
    palette[0] = 0;
}

void rgb_palette::setpixel(uint16_t x, uint16_t y, uint32_t color) {
    if(x < width) palette[x] = color;
}

uint32_t rgb_palette::getpixel(uint16_t x, uint16_t y) const {
    if(x < width) return palette[x];
    return 0;
}

void rgb_palette::fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    if(x < width && x + w <= width) {
        for(int i = x; i < x + w; ++i) palette[i] = color;
    }
}
