//
// Created by Julija Ivaske on 15.1.2026.
//

#include "lvgl_port.h"

#include <cstdio>

#include "pico/time.h"

LVGLPort *LVGLPort::instance = nullptr;

LVGLPort::LVGLPort(std::shared_ptr<ili9341> display)
                    : display(display) {
    instance = this;
}

void LVGLPort::init() {
    lv_init();

    // buffer for pixels for 1/10 of the screen: (240*320)/10 = 7680
    // 1 pixel = 2 bytes
    static lv_color_t buf1[7680];
    static lv_color_t buf2[7680];

    // creating lvgl display
    disp = lv_display_create(display->get_width(), display->get_height());

    //set flush callback func
    lv_display_set_flush_cb(disp, display_flush_cb);

    // setting buffers
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void LVGLPort::display_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    //printf("FLUSH: x1=%d, y1=%d, x2=%d, y2=%d\n", area->x1, area->y1, area->x2, area->y2);

    // area dimension calculation
    uint16_t x0 = area->x1;
    uint16_t y0 = area->y1;
    uint16_t x1 = area->x2;
    uint16_t y1 = area->y2;

    //printf("Window: x0=%d, y0=%d, x1=%d, y1=%d\n", x0, y0, x1, y1);

    // set window
    //instance->display->set_window(x0, y0, x1, y1); // no need cause setting window already in draw_pixels func

    // num of pixel calculation
    uint32_t width = (x1 - x0 + 1);
    uint32_t height = (y1 -y0 + 1);
    uint32_t pixels = width * height;
    //int32_t bytes = pixels * 2;

    //printf("Writing %lu pixels (%lu bytes)\n", pixels, pixels * 2);
    auto *px16 = (uint16_t *)px_map;
    for (uint32_t i = 0; i < pixels; i++) {
        px16[i] = __builtin_bswap16(px16[i]);
    }

    // write pixels
    instance->display->draw_pixels(x0, y0, x1, y1, px_map, pixels * 2);
    lv_display_flush_ready(display);
    //printf("Flush complete\n");
}

void LVGLPort::tick(uint32_t ms) {
    lv_tick_inc(ms);
}
