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

enum screens {
    MAIN,
    SET_RH,
    PRESET_SELECT,
    SET_NETWORK,
    SCAN_NETWORK,
    ENTER_PASS,
};

struct Preset_Options {
    const char* name;
    uint8_t rh_val;
};

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

    screens current_screen;
    screens next_screen;

    // flags for events (maybe do event bits?)
    bool menu_selected;
    bool rh_val_saved;

    // data variables
    uint8_t menu_selection;
    uint8_t set_rh_value;
    //uint8_t target_rh = 50;

    // functions for loading different UI screens
    void load_main_screen(Message received, bool initial);
    static void dd_menu_callback(lv_event_t* e);

    void load_rh_set_screen(uint8_t target_rh);
    static void slider_event_cb(lv_event_t * e);
    static void save_btn_event_cb(lv_event_t * e);
    static void preset_btn_event_cb(lv_event_t * e);
    static void cancel_slider_btn_callback(lv_event_t *e);
    static void cancel_preset_btn_callback(lv_event_t *e);

    static constexpr Preset_Options PRESET_OPTIONS[] = {
        {"Acoustic gitar", 45},
        {"Electric guitar", 40},
        {"Violin", 50},
        {"Cigars", 65}
    };
    void load_preset_screen();
    static void preset_selection_cb(lv_event_t * e);
    static void save_preset_btn_callback(lv_event_t * e);

    void create_save_button(lv_event_cb_t* event_cb);

    // lvgl UI elements
    lv_obj_t *temp_label;
    lv_obj_t *rh_label;
    lv_obj_t *dropdown;
    lv_obj_t *tank_label;
    lv_obj_t *network_label;

    lv_style_t style_radio;
    lv_style_t style_radio_chk;

    // static to be used from multiple places
    lv_obj_t * slider_label;
};

#endif //UI_H
