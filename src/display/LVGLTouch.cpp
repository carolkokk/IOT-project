//
// Created by Julija Ivaske on 17.1.2026.
//

#include "LVGLTouch.h"

#include <cstdio>

LVGLTouch* LVGLTouch::instance = nullptr;

LVGLTouch::LVGLTouch(XPT2046_Touch *touch, uint16_t width, uint16_t height)
    : touch(touch), indev(nullptr), screen_width(width), screen_height(height),
    raw_x_min(100), raw_x_max(3000), raw_y_min(100), raw_y_max(3000) // need to be calibrated for these!
{
    instance = this;
}

bool LVGLTouch::init() {
    if (!touch) return false;

    indev = lv_indev_create();
    if (!indev) return false;

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, read_cb);

    return true;
}

void LVGLTouch::setCalibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max) {
    raw_x_min = x_min;
    raw_x_max = x_max;
    raw_y_min = y_min;
    raw_y_max = y_max;
}

void LVGLTouch::read_cb(lv_indev_t *in_dev, lv_indev_data_t *data) {
    if (instance) {
        instance->read(data);
    }
}

void LVGLTouch::read(lv_indev_data_t *data) {
    if (!touch) {
        printf("Touch not initialized\n");
        return;
    }

    uint16_t raw_x, raw_y, raw_z;
    touch->readData(&raw_x, &raw_y, &raw_z);  // calls update() once

    if (raw_z >= 400) {
        printf("RAW: x=%d, y=%d\n", raw_x, raw_y);
        data->point.x = mapX(raw_x);
        data->point.y = mapY(raw_y);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    //printf("Mapped: x=%d, y=%d\n", data->point.x, data->point.y);
}

uint16_t LVGLTouch::mapX(uint16_t raw_x) {
    // to not go out of bounds
    if (raw_x < raw_x_min) raw_x = raw_x_min;
    if (raw_x > raw_x_max) raw_x = raw_x_max;

    uint32_t mapped = (uint32_t)(raw_x - raw_x_min) * screen_width / (raw_x_max - raw_x_min);
    if (mapped >= screen_width) mapped = screen_width - 1;

    return (uint16_t)mapped;
}

uint16_t LVGLTouch::mapY(uint16_t raw_y) {
    if (raw_y < raw_y_min) raw_y = raw_y_min;
    if (raw_y > raw_y_max) raw_y = raw_y_max;

    uint32_t mapped = (uint32_t)(raw_y - raw_y_min) * screen_height / (raw_y_max - raw_y_min);
    if (mapped >= screen_height) mapped = screen_height - 1;

    return (uint16_t)mapped;
}



