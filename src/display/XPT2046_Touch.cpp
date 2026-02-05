//
// Created by Julija Ivaske on 17.1.2026.
//

#include "XPT2046_Touch.h"

#define Z_THRESHOLD 200
#define MS_THRESHOLD 3

XPT2046_Touch::XPT2046_Touch(PicoSPIDevice *spi_device)
                            :spi_dev(spi_device),
                            rotation(3),
                            xraw(0), yraw(0), zraw(0), msraw(0x80000000) {
}

TS_Point XPT2046_Touch::getPoint() {
    update();
    return TS_Point(xraw, yraw, zraw);
}

bool XPT2046_Touch::touched() {
    update();
    return (zraw >= Z_THRESHOLD); // return true if touch is detected above threshold
}

void XPT2046_Touch::readData(uint16_t *x, uint16_t *y, uint16_t *z) {
    // gets raw coordinates
    update();
    *x = xraw;
    *y = yraw;
    *z = zraw;
}

int16_t XPT2046_Touch::bestTwoAvg(uint16_t x, uint16_t y, uint16_t z) {
    int16_t da, db, dc;
    int16_t reta = 0;

    if ( x > y ) da = x - y; else da = y - x;
    if ( x > z ) db = x - z; else db = z - x;
    if ( z > y ) dc = z - y; else dc = y - z;

    if ( da <= db && da <= dc ) reta = (x + y) >> 1;
    else if ( db <= da && db <= dc ) reta = (x + z) >> 1;
    else reta = (y + z) >> 1;   //    else if ( dc <= da && dc <= db ) reta = (x + y) >> 1;

    return (reta);
}


// this function detects pressure and if pressure is sufficient, gets x and y coordinates
void XPT2046_Touch::update() {
    int16_t data[6];

    //if (!isrWake) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - msraw < MS_THRESHOLD) return;

    if (!spi_dev) return;

    // starting spi transaction, cs active on LOW
    spi_dev->set_cs(0);

    // Read Z1 - use transaction to write dummy bytes while reading
    uint8_t tx[3] = {0xB1, 0x00, 0x00};
    uint8_t rx[3] = {0};
    spi_dev->transaction(tx, rx, 3);
    uint16_t z1 = ((rx[1] << 8) | rx[2]) >> 3;
    z1 &= 0x0FFF;
    int z = z1 + 4095;

    // Read Z2
    tx[0] = 0xC1; tx[1] = 0x00; tx[2] = 0x00;
    spi_dev->transaction(tx, rx, 3);
    uint16_t z2 = ((rx[1] << 8) | rx[2]) >> 3;
    z2 &= 0x0FFF;
    z -= z2;

    if (z >= Z_THRESHOLD) {
        // read x
        tx[0] = 0x91; tx[1] = 0x00; tx[2] = 0x00;

        // dummyx read
        spi_dev->transaction(tx, rx, 3);

        // first x
        spi_dev->transaction(tx, rx, 3);
        data[0] = (((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;

        // y read
        tx[0] = 0xD1;
        spi_dev->transaction(tx, rx, 3);
        data[1] = (((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;

        // second x
        tx[0] = 0x91;
        spi_dev->transaction(tx, rx, 3);
        data[2] = (((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;

        // second y
        tx[0] = 0xD1;
        spi_dev->transaction(tx, rx, 3);
        data[3] = (((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;

        // third x
        tx[0] = 0x91;
        spi_dev->transaction(tx, rx, 3);
        data[4] = (((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;

        // third y and then powerdown
        tx[0] = 0xD0;
        spi_dev->transaction(tx, rx, 3);
        data[5] = (((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;
    }
    spi_dev->set_cs(1);

    // Process pressure
    if (z < 0) z = 0;
    if (z < Z_THRESHOLD) {
        zraw = 0;
        /*if (z < Z_THRESHOLD_INT) {
            if (t_irq != 255) isrWake = false;
        }*/
        return;
    }
    zraw = z;

    // finding avg using best-two algorithm
    int16_t x = bestTwoAvg(data[0], data[2], data[4]);
    int16_t y = bestTwoAvg(data[1], data[3], data[5]);

    // rotation handling
    msraw = now;
    switch (rotation) {
        case 0:
            xraw = 4095 - y;
            yraw = x;
            break;
        case 1:
            xraw = x;
            yraw = y;
            break;
        case 2:
            xraw = y;
            yraw = 4095 - x;
            break;
        default: // 3
            xraw = 4095 - x;
            yraw = 4095 - y;
            break;
    }
}






