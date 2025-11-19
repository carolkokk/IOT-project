//
// Created by Keijo Länsikunnas on 18.2.2024.
//

#ifndef PICO_MODBUS_FRAMEBUF_H
#define PICO_MODBUS_FRAMEBUF_H

#include <string>
#include <cstdint>
#include "fonts/gfxfont.h"

class framebuf {
public:
    framebuf(uint16_t width_, uint16_t heigth_);
    virtual ~framebuf() = default;
    void fill(uint32_t color);
    void line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
    void hline(uint16_t x, uint16_t y, uint16_t w, uint32_t color);
    void vline(uint16_t x, uint16_t y, uint16_t h, uint32_t color);
    void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color, bool fill = false);
    void text(const char *str, uint16_t x, uint16_t y, uint32_t color = 1);
    void text(const std::string &str, uint16_t x, uint16_t y, uint32_t color = 1);
    void blit(framebuf &fb, int16_t x, int16_t y);
    void blit(framebuf &fb, int16_t x, int16_t y, uint32_t key);
    void blit(framebuf &fb, int16_t x, int16_t y, uint32_t key, const framebuf &palette);
    void blit(framebuf &fb, int16_t x, int16_t y, const framebuf &palette);
    void scroll(int16_t xstep, int16_t ystep);
    void setfont(const GFXfont *font = nullptr);
    static const uint32_t no_key_color = 0xFFFFFFFF; // assuming that color uses less than 32 bits
private:
    virtual void setpixel(uint16_t x, uint16_t y, uint32_t color) = 0;
    virtual uint32_t getpixel(uint16_t x, uint16_t y) const = 0;
    virtual void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) = 0;
    void drawchar(int c, uint32_t color);
    void drawgfxchar(int c, uint32_t color);
    void blit_fb(framebuf &source, int16_t x, int16_t y, uint32_t key, const framebuf *palette);
    uint16_t cursor_x;
    uint16_t cursor_y;
    const GFXfont *font;
protected:
    uint16_t width;
    uint16_t height;
};


#endif //PICO_MODBUS_FRAMEBUF_H
