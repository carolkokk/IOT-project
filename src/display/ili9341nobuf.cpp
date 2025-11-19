//
// ILI9341 no-buffer driver for RP2040 / Pico
//

#include "ili9341nobuf.h"

#include <hardware/gpio.h>
#include <pico/time.h>

// Size of 2.8" display
#define ILI9341_TFTWIDTH  240
#define ILI9341_TFTHEIGHT 320

// Special flag for command lists
#define ST_CMD_DELAY 0x80

// commands for ili9341 driver

#define ILI9341_SWRESET      0x01
#define ILI9341_SLEEP_IN     0x10
#define ILI9341_SLEEP_OUT    0x11
#define ILI9341_DISP_OFF     0x28
#define ILI9341_DISP_ON      0x29
#define ILI9341_CASET        0x2A
#define ILI9341_PASET        0x2B
#define ILI9341_RAMWR        0x2C
#define ILI9341_RAMRD        0x2E
#define ILI9341_MADCTL       0x36
#define ILI9341_PIXEL_FORMAT 0x3A

// Power / gamma control registers (standard ILI9341)
#define ILI9341_POWERB       0xCF
#define ILI9341_POWER_SEQ    0xED
#define ILI9341_POWERA       0xCB
#define ILI9341_PRC          0xF7
#define ILI9341_DTCA         0xE8
#define ILI9341_DTCB         0xEA
#define ILI9341_POWER1       0xC0
#define ILI9341_POWER2       0xC1
#define ILI9341_VCOM1        0xC5
#define ILI9341_VCOM2        0xC7
#define ILI9341_FRC          0xB1
#define ILI9341_DFC          0xB6
#define ILI9341_3GAMMA_EN    0xF2
#define ILI9341_GAMMASET     0x26
#define ILI9341_GMCTRP1      0xE0
#define ILI9341_GMCTRN1      0xE1

// MADCTL bits
#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08

// Some ready-made 16-bit ('565') color settings:
#define ILI9341_BLACK   0x0000
#define ILI9341_WHITE   0xFFFF
#define ILI9341_RED     0xF800
#define ILI9341_GREEN   0x07E0
#define ILI9341_BLUE    0x001F
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_ORANGE  0xFC00

// clang-format off
static const uint8_t ili9341_init[] = {
    23, // number of commands

    ILI9341_SWRESET, ST_CMD_DELAY,
      150, // delay 150 ms

    ILI9341_POWERB, 3,
      0x00, 0x83, 0x30,

    ILI9341_POWER_SEQ, 4,
      0x64, 0x03, 0x12, 0x81,

    ILI9341_DTCA, 3,
      0x85, 0x01, 0x79,

    ILI9341_POWERA, 5,
      0x39, 0x2C, 0x00, 0x34, 0x02,

    ILI9341_PRC, 1,
      0x20,

    ILI9341_DTCB, 2,
      0x00, 0x00,

    ILI9341_POWER1, 1,
      0x26,             // VRH[5:0]

    ILI9341_POWER2, 1,
      0x11,             // SAP[2:0];BT[3:0]

    ILI9341_VCOM1, 2,
      0x35, 0x3E,

    ILI9341_VCOM2, 1,
      0xBE,

    ILI9341_MADCTL, 1,
      0x48,             // row/col order + BGR; will be overwritten by setrotation()

    ILI9341_PIXEL_FORMAT, 1,
      0x55,             // 16-bit color

    ILI9341_FRC, 2,
      0x00, 0x1B,

    ILI9341_DFC, 3,
      0x0A, 0xA2, 0x27,

    ILI9341_3GAMMA_EN, 1,
      0x00,             // 3Gamma function disable

    ILI9341_GAMMASET, 1,
      0x01,             // Gamma curve 1

    ILI9341_GMCTRP1, 15,
      0x0F, 0x31, 0x2B, 0x0C, 0x0E,
      0x08, 0x4E, 0xF1, 0x37, 0x07,
      0x10, 0x03, 0x0E, 0x09, 0x00,

    ILI9341_GMCTRN1, 15,
      0x00, 0x0E, 0x14, 0x03, 0x11,
      0x07, 0x31, 0xC1, 0x48, 0x08,
      0x0F, 0x0C, 0x31, 0x36, 0x0F,

    ILI9341_SLEEP_OUT, ST_CMD_DELAY,
      120,

    ILI9341_DISP_ON, ST_CMD_DELAY,
      10
};
// clang-format on

ili9341nobuf::ili9341nobuf(std::shared_ptr<spi_device> dev,
                           uint dc_pin,
                           uint rst_pin,
                           uint bl_pin,
                           uint16_t w,
                           uint16_t h,
                           uint8_t rotation)
    : framebuf(w, h),
      spi(dev),
      gpio_dc(dc_pin),
      gpio_rst(rst_pin),
      gpio_bl(bl_pin) {

    gpio_init(gpio_dc);
    gpio_set_dir(gpio_dc, GPIO_OUT);

    if (gpio_bl != UINT_MAX) {
        gpio_init(gpio_bl);
        gpio_set_dir(gpio_bl, GPIO_OUT);
    }
    if (gpio_rst != UINT_MAX) {
        gpio_init(gpio_rst);
        gpio_set_dir(gpio_rst, GPIO_OUT);
    }

    // default signal levels
    set_dc(1);
    set_rst(1);
    spi->set_cs(1);
    sleep_ms(100);

    // For ILI9341 we don't need any offsets; everything starts at (0, 0)
    _xstart   = 0;
    _ystart   = 0;
    _colstart = 0;
    _colstart2 = 0;
    _rowstart = 0;
    _rowstart2 = 0;

    windowWidth  = w;
    windowHeight = h;

    // Reset panel if RST pin is available
    if (gpio_rst != UINT_MAX) {
        set_rst(0);
        sleep_ms(20);
        set_rst(1);
        sleep_ms(150);
    }

    // Run init command list
    displayinit(ili9341_init);

    // Set logical rotation
    setrotation(rotation);

    // Backlight on
    set_bl(1);
}

void ili9341nobuf::show() {
    // No-op: data is written directly
}

void ili9341nobuf::setpixel(uint16_t x, uint16_t y, uint32_t color) {
    if (x < width && y < height) {
        spi->set_cs(0);
        setaddrwindow(x, y, 1, 1);
        write(static_cast<uint16_t>(color));
        spi->set_cs(1);
    }
}

uint32_t ili9341nobuf::getpixel(uint16_t x, uint16_t y) const {
    // Not implemented (requires RAMRD + readback)
    (void)x;
    (void)y;
    return 0;
}

void ili9341nobuf::fill_rect(uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h,
                             uint32_t color) {
    if ((w == 0) || (h == 0)) return;
    if (x >= width || y >= height) return;

    // Clip rectangle to screen
    if (x + w > width)  w = width  - x;
    if (y + h > height) h = height - y;

    spi->set_cs(0);
    setaddrwindow(x, y, w, h);

    uint16_t c = static_cast<uint16_t>(color);
    for (uint32_t i = static_cast<uint32_t>(w) * h; i > 0; --i) {
        write(c);
    }
    spi->set_cs(1);
}

void ili9341nobuf::displayinit(const uint8_t *addr) {
    uint8_t numCommands, cmd, numArgs;
    uint16_t ms;

    numCommands = *addr++;         // Number of commands
    while (numCommands--) {
        cmd     = *addr++;         // Command
        numArgs = *addr++;         // Num args, possibly with delay flag
        ms      = numArgs & ST_CMD_DELAY;
        numArgs &= ~ST_CMD_DELAY;  // Mask out delay bit

        command(cmd, addr, numArgs);
        addr += numArgs;

        if (ms) {
            ms = *addr++;          // Delay time (ms)
            if (ms == 255) ms = 500;
            sleep_ms(ms);
        }
    }
}

void ili9341nobuf::setrotation(uint8_t r) {
    uint8_t madctl = 0;

    r &= 3; // 0-3

    switch (r) {
        case 0:
            madctl = ILI9341_MADCTL_BGR;
            width  = windowWidth;
            height = windowHeight;
            _xstart = 0;
            _ystart = 0;
            break;
        case 1:
            madctl = ILI9341_MADCTL_MV | ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR;
            width  = windowHeight;
            height = windowWidth;
            _xstart = 0;
            _ystart = 0;
            break;
        case 2:
            madctl = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR;
            width  = windowWidth;
            height = windowHeight;
            _xstart = 0;
            _ystart = 0;
            break;
        case 3:
            madctl = ILI9341_MADCTL_MV | ILI9341_MADCTL_MY |
                      ILI9341_MADCTL_BGR;
            width  = windowHeight;
            height = windowWidth;
            _xstart = 0;
            _ystart = 0;
            break;
    }

    command(ILI9341_MADCTL, &madctl, 1);
}

void ili9341nobuf::command(uint8_t cmd, const uint8_t *data, size_t len) {
    spi->set_cs(0);

    set_dc(0);          // command mode
    write(cmd);
    set_dc(1);          // data mode

    if (len > 0) {
        write(data, len);
    }

    spi->set_cs(1);
}

void ili9341nobuf::write(const uint8_t *data, size_t len) {
    spi->write(data, len);
}

void ili9341nobuf::write(uint8_t value) {
    spi->write(&value, 1);
}

void ili9341nobuf::write(uint16_t value) {
    // 16-bit big endian
    uint8_t data[] = {
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value & 0xFF)
    };
    spi->write(data, 2);
}

void ili9341nobuf::write(uint32_t value) {
    // 32-bit big endian
    uint8_t data[] = {
        static_cast<uint8_t>(value >> 24),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value & 0xFF)
    };
    spi->write(data, 4);
}

void ili9341nobuf::setaddrwindow(uint16_t x, uint16_t y,
                                 uint16_t w, uint16_t h) {
    x += _xstart;
    y += _ystart;

    uint16_t x0 = x;
    uint16_t x1 = x + w - 1;
    uint16_t y0 = y;
    uint16_t y1 = y + h - 1;

    // Column address set
    writecommand(ILI9341_CASET);
    uint8_t caset_data[] = {
        static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0 & 0xFF),
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF)
    };
    write(caset_data, sizeof(caset_data));

    // Page address set
    writecommand(ILI9341_PASET);
    uint8_t paset_data[] = {
        static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0 & 0xFF),
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF)
    };
    write(paset_data, sizeof(paset_data));

    // Write to RAM
    writecommand(ILI9341_RAMWR);
}

void ili9341nobuf::writecommand(uint8_t cmd) {
    set_dc(0);
    write(cmd);
    set_dc(1);
}

void ili9341nobuf::set_dc(bool value) const {
    gpio_put(gpio_dc, value ? 1 : 0);
}

void ili9341nobuf::set_rst(bool value) const {
    if (gpio_rst != UINT_MAX) {
        gpio_put(gpio_rst, value ? 1 : 0);
    }
}

void ili9341nobuf::set_bl(bool value) const {
    if (gpio_bl != UINT_MAX) {
        gpio_put(gpio_bl, value ? 1 : 0);
    }
}
