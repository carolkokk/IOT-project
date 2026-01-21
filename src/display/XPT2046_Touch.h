//
// Created by Julija Ivaske on 17.1.2026.
//

// XPT2046 library integrated from Arduino library by Paul Stoffregen

#ifndef HUMIBOX_XPT2046_TOUCH_H
#define HUMIBOX_XPT2046_TOUCH_H

#include "pico/stdlib.h"
#include "PicoSPIDevice.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

class TS_Point {
    public:
        TS_Point(void): x(0), y(0), z(0) {}
        TS_Point(int16_t x, int16_t y, int16_t z): x(x), y(y), z(z) {}
        bool operator==(TS_Point p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
        bool operator!=(TS_Point p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
        int16_t x, y, z;
};

class XPT2046_Touch {
    public:
        XPT2046_Touch(PicoSPIDevice* spi_device);

        //bool begin();
        TS_Point getPoint();
        bool touched();
        void readData(uint16_t* x, uint16_t* y, uint16_t* z);
        bool bufferEmpty();
        uint8_t bufferSize() { return 1; }
        void setRotation(uint8_t n) { rotation = n % 4; }

        uint16_t getRawX() const { return xraw; }
        uint16_t getRawY() const { return yraw; }
        uint16_t getRawZ() const { return zraw; }

    private:
        void update();
        static int16_t bestTwoAvg(uint16_t x, uint16_t y, uint16_t z);

        PicoSPIDevice* spi_dev;
        uint8_t rotation;
        uint16_t xraw, yraw, zraw;
        uint32_t msraw;

};


#endif //HUMIBOX_XPT2046_TOUCH_H