#ifndef UI_H
#define UI_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <memory>

#include "LVGLTouch.h"
#include "display/lvgl_port.h"
#include "display/ili9341.h"
#include "display/XPT2046_Touch.h"


class UI {
public:
    UI(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period, uint32_t stack_size = 4096, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    const char *name = "UI";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    TickType_t period;

    std::shared_ptr<PicoSPIBus> spi_0;
    std::shared_ptr<PicoSPIBus> spi_1;
    std::shared_ptr<PicoSPIDevice> display_device;
    std::shared_ptr<PicoSPIDevice> touch_device;
    std::shared_ptr<ili9341> display;
    std::shared_ptr<LVGLPort> lvgl_port;
    std::shared_ptr<XPT2046_Touch> touch;
    std::shared_ptr<LVGLTouch> lvgl_touch;

    // lvgl UI elements
    lv_obj_t *temp_label;
    lv_obj_t *rh_label;

};

#endif //UI_H
