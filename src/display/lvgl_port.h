//
// Created by Julija Ivaske on 15.1.2026.
//

#ifndef HUMIBOX_LVGL_PORT_H
#define HUMIBOX_LVGL_PORT_H

#include "lvgl.h"
#include "Display.h"
#include <memory>

class LVGLPort {
    public:
        LVGLPort(std::shared_ptr<Display> display);

        void init();

    private:
        std::shared_ptr<Display> drv;
        lv_display_t *disp;

        static void display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
};


#endif //HUMIBOX_LVGL_PORT_H
