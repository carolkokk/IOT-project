//
// Created by Keijo Länsikunnas on 6.3.2024.
//
// This is based on MicroPython modframebuf.c
// modframebuf.c licence attached below
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <cstring>
#include "mono_horiz.h"

mono_horiz::mono_horiz(uint16_t width_, uint16_t height_, uint16_t stride_, uint16_t buf_offset, bool format_msb) :
        framebuf(width_, height_), size(0), stride(stride_), buffer_offset(buf_offset), msb(format_msb) {
    if (stride < width) { stride = width; }
    if (stride % 8 != 0) { stride = (stride / 8 + 1) * 8; } // fix stride
    size = height * (stride / 8) + buffer_offset;
    buffer = std::shared_ptr<uint8_t>(new uint8_t[size]);
    // zero out the buffer
    std::memset(buffer.get(), 0, size);
}

mono_horiz::mono_horiz(const uint8_t *image, uint8_t width_, uint16_t height_, uint16_t stride_, uint16_t buf_offset,
                       bool format_msb)
        : framebuf(width_, height_), size(0), stride(stride_), buffer_offset(buf_offset), msb(format_msb) {
    if (stride < width) { stride = width; }
    if (stride % 8 != 0) { stride = (stride / 8 + 1) * 8; } // fix stride
    size = height * (stride / 8) + buffer_offset;
    buffer = std::shared_ptr<uint8_t>(new uint8_t[size]);
    // copy image to the buffer
    std::memcpy(buffer.get() + buffer_offset, image, size - buffer_offset);
}

void mono_horiz::setpixel(uint16_t x, uint16_t y, uint32_t color) {
    if(x >= width || y >= height) return;
    size_t index = ((x + y * stride) >> 3) + buffer_offset;
    unsigned int offset = msb ? x & 0x07 : 7 - (x & 0x07);
    buffer.get()[index] = (buffer.get()[index] & ~(0x01 << offset)) | ((color != 0) << offset);

}

uint32_t mono_horiz::getpixel(uint16_t x, uint16_t y) const {
    if(x >= width || y >= height) return 0;
    size_t index = ((x + y * stride) >> 3) + buffer_offset;
    unsigned int offset = msb ? x & 0x07 : 7 - (x & 0x07);
    return (buffer.get()[index] >> (offset)) & 0x01;
}

void mono_horiz::fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    unsigned int advance = stride >> 3;
    while (w--) {
        uint8_t *b = &(buffer.get()[(x >> 3) + y * advance + buffer_offset]);
        unsigned int offset = msb ? x & 7 : 7 - (x & 7);
        for (unsigned int hh = h; hh; --hh) {
            *b = (*b & ~(0x01 << offset)) | ((color != 0) << offset);
            b += advance;
        }
        ++x;
    }
}
