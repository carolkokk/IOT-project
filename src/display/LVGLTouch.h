//
// Created by Julija Ivaske on 17.1.2026.
//

#ifndef HUMIBOX_LVGL_TOUCH_H
#define HUMIBOX_LVGL_TOUCH_H

#include "lvgl.h"
#include "XPT2046_Touch.h"
#include <memory>

class LVGLTouch {
    public:
        LVGLTouch(std::shared_ptr<XPT2046_Touch> touch, uint16_t width, uint16_t height);
        bool init();

        // these values should be input after determining calibration
        void setCalibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);

        // getting lvgl input device
        lv_indev_t* getInputDevice() { return indev; }

    private:
        static void read_cb(lv_indev_t* in_dev, lv_indev_data_t* data);
        void read(lv_indev_data_t* data);

        // mapping raw coordinates to screen
        uint16_t mapX(uint16_t raw_x);
        uint16_t mapY(uint16_t raw_y);

        std::shared_ptr<XPT2046_Touch> touch;
        lv_indev_t *indev;
        uint16_t screen_width;
        uint16_t screen_height;

        //calibration values
        uint16_t raw_x_min;
        uint16_t raw_x_max;
        uint16_t raw_y_min;
        uint16_t raw_y_max;
};


#endif //HUMIBOX_LVGL_TOUCH_H
