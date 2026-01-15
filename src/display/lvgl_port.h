//
// Created by Julija Ivaske on 15.1.2026.
//

#ifndef HUMIBOX_LVGL_PORT_H
#define HUMIBOX_LVGL_PORT_H

#include "lvgl.h"
#include "ili9341.h"
#include <memory>

class LVGLPort {
    public:
        LVGLPort(std::shared_ptr<ili9341> display);

        void init();
        void tick(uint32_t ms);

    private:
        std::shared_ptr<ili9341> display;
        lv_display_t *disp;

        static void display_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
        static LVGLPort *instance;
};


#endif //HUMIBOX_LVGL_PORT_H