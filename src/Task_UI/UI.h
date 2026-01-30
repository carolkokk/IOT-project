#ifndef UI_H
#define UI_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <memory>
#include "Structs.h"

#include "LVGLTouch.h"
#include "display/lvgl_port.h"
#include "display/ili9341.h"
#include "display/XPT2046_Touch.h"


class UI {
public:
    UI(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period, uint32_t stack_size = 4096*2, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    const char *name = "UI";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    TickType_t period;

    Message sensor_data;

    std::shared_ptr<PicoSPIBus> spi_0;
    std::shared_ptr<PicoSPIBus> spi_1;
    std::shared_ptr<PicoSPIDevice> display_device;
    std::shared_ptr<PicoSPIDevice> touch_device;
    std::shared_ptr<ili9341> display;
    std::shared_ptr<LVGLPort> lvgl_port;
    std::shared_ptr<XPT2046_Touch> touch;
    std::shared_ptr<LVGLTouch> lvgl_touch;

    void init_UI(void);

    enum screens {
        MAIN,
        SET_RH,
        SET_NETWORK,
        SCAN_NETWORK,
        ENTER_PASS,
    };

    screens current_screen;
    screens next_screen;

    // flags for events (maybe do event bits?)
    bool menu_selected;
    bool slider_val_saved;

    // data variables
    uint8_t menu_selection;
    int32_t slider_value;
    uint8_t target_rh = 50;

    // functions for loading different UI screens
    void load_main_screen(Message received, bool initial);
    static void dd_menu_callback(lv_event_t* e);

    void load_rh_set_screen();
    static void slider_event_cb(lv_event_t * e);
    static void btn_event_cb(lv_event_t * e);

    // lvgl UI elements
    lv_obj_t *temp_label;
    lv_obj_t *rh_label;
    lv_obj_t *dropdown;
    lv_obj_t *tank_label;
    lv_obj_t *network_label;

    // static to be used from multiple places
    lv_obj_t * slider_label;
};

#endif //UI_H
