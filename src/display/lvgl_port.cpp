//
// Created by Julija Ivaske on 15.1.2026.
//

#include "lvgl_port.h"

#include "pico/time.h"

LVGLPort::LVGLPort(std::shared_ptr<Display> displ)
                    : drv(std::move(displ)), disp(nullptr) {
}

void LVGLPort::init() {
    lv_tick_set_cb([]() -> uint32_t { return to_ms_since_boot(get_absolute_time()); });
    lv_init();

    // double-buffer.each covers 1/10 of screen (240*320/10 = 7680 pixels, 2 bytes each)
    static lv_color_t buf1[7680];
    static lv_color_t buf2[7680];

    disp = lv_display_create(drv->get_width(), drv->get_height());
    lv_display_set_user_data(disp, this);
    lv_display_set_flush_cb(disp, display_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void LVGLPort::display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    auto *self = static_cast<LVGLPort*>(lv_display_get_user_data(disp));

    uint16_t x0 = area->x1;
    uint16_t y0 = area->y1;
    uint16_t x1 = area->x2;
    uint16_t y1 = area->y2;

    uint32_t pixels = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);

    // Swap bytes to big-endian for ILI9341
    auto *px16 = (uint16_t *)px_map;
    for (uint32_t i = 0; i < pixels; i++) {
        px16[i] = __builtin_bswap16(px16[i]);
    }

    self->drv->draw_pixels(x0, y0, x1, y1, px_map, pixels * 2);
    lv_display_flush_ready(disp);
}
