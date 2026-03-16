#ifndef HUMIBOX_IDISPLAY_H
#define HUMIBOX_IDISPLAY_H
#include <cstdint>
#include <cstddef>

class Display {
public:
    virtual void draw_pixels(uint16_t x0, uint16_t y0,
                             uint16_t x1, uint16_t y1,
                             const uint8_t *data, size_t len) = 0;
    virtual uint16_t get_width()  const = 0;
    virtual uint16_t get_height() const = 0;
    virtual ~Display() = default;
};
#endif
