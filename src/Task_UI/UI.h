#ifndef UI_H
#define UI_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "EEPROM/EEPROM.h"
#include <memory>

#include "event_groups.h"
#include "Structs.h"

#include "LVGLTouch.h"
#include "display/lvgl_port.h"
#include "display/Display.h"
#include "display/XPT2046_Touch.h"

enum Screens {
    MAIN,
    SET_RH,
    PRESET_SELECT,
    NETWORK,
    AVAILABLE_NETWORKS,
    ENTER_PASS,
    CONNECTING_WIFI,
    STATISTICS,
    MEASUREMENT_CHART,
    LOG_HISTORY,
};

struct System_Status {
    bool connecting_network = false;
    bool network_connected = false;
    bool bad_auth = false;
    bool water_overflow = false;
    bool refill_water = false;
};

struct Preset_Options {
    const char* name;
    uint8_t rh_val;
};

class UI {
public:
    UI(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, QueueHandle_t scan_results_queue, QueueHandle_t credentials_to_network,
        EventGroupHandle_t event_group,
        TickType_t period, std::shared_ptr<EEPROM> eeprom,
        uint32_t stack_size = 4096, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    System_Status sys_status;
    void task_impl();
    void init_task_state();
    const char *name = "UI";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    QueueHandle_t credentials_to_network;
    QueueHandle_t scan_results_queue;
    EventGroupHandle_t event_group;
    TickType_t period;

    std::shared_ptr<EEPROM> eeprom;

    Message sensor_data;

    int32_t min_set_rh = 30;
    int32_t max_set_rh = 70;

    std::shared_ptr<PicoSPIBus> spi_0;
    std::shared_ptr<PicoSPIBus> spi_1;
    std::shared_ptr<PicoSPIDevice> display_device;
    std::shared_ptr<PicoSPIDevice> touch_device;
    std::shared_ptr<Display> drv;
    std::shared_ptr<LVGLPort> lvgl_port;
    std::shared_ptr<XPT2046_Touch> touch;
    std::shared_ptr<LVGLTouch> lvgl_touch;

    void init_UI(void);
    void load_or_init_calibration();

    Screens current_screen = MAIN;
    Screens next_screen = MAIN;

    // flags for events
    bool menu_selected = false;
    bool rh_val_saved = false;

    // data variables
    uint8_t menu_selection = 0;
    uint8_t set_rh_value = 50;

    // functions for main screen
    void build_main_screen(System_Status status);
    void update_main_screen(const Message& received);
    void update_wifi_status(System_Status status);
    void update_water_status(System_Status status);
    static void dd_menu_callback(lv_event_t* e);

    // functions for setting rh screens
    void load_rh_set_screen(uint8_t target_rh);
    static void slider_event_cb(lv_event_t * e);
    static void save_btn_event_cb(lv_event_t * e);
    static void preset_btn_event_cb(lv_event_t * e);
    static constexpr Preset_Options PRESET_OPTIONS[] = {
        {"Acoustic gitar", 45},
        {"Electric guitar", 40},
        {"Violin", 50},
        {"Cigars", 65}
    };
    void load_preset_screen();
    static void preset_selection_cb(lv_event_t * e);
    static void save_preset_btn_callback(lv_event_t * e);

    // functions for network setting screens
    void load_network_screen(bool network_connected);
    static void search_networks_btn_cb(lv_event_t * e);
    void load_available_networks(Scan_result_msg &msg);
    static void network_list_cb(lv_event_t *e);
    void load_connecting_wifi_screen();
    Scan_result scan_results[MAX_SCAN_RESULTS];
    uint8_t result_count = 0;
    Network_credentials credentials;
    void load_password_screen();
    static void keyboard_cb(lv_event_t *e);

    // functions for displaying statistics
    void load_statistics_screen();
    static void chart_button_cb(lv_event_t *e);
    static void log_button_cb(lv_event_t *e);
    void load_measurement_chart_screen();
    void load_log_history_screen();
    static void delete_log_button_cb(lv_event_t *e);

    // generic functions for creating buttons, going back
    void create_button(lv_event_cb_t event_cb, lv_align_t align, int32_t x_ofs, int32_t y_ofs, const char *text);
    void navigate_to(Screens screen);
    void navigate_back();
    static void back_btn_cb(lv_event_t * e);

    Screens screen_history[5];
    int screen_depth = 0;

    bool networks_loaded = false;

    // lvgl UI elements
    lv_obj_t *temp_label = nullptr;
    lv_obj_t *rh_label = nullptr;
    lv_obj_t *dropdown = nullptr;
    lv_obj_t *tank_icon = nullptr;
    lv_obj_t *network_icon = nullptr;
    lv_obj_t *tank_status_label = nullptr;

    lv_style_t style_radio;
    lv_style_t style_radio_chk;

    lv_obj_t * slider_label = nullptr;

    lv_obj_t *network_list = nullptr;
    lv_obj_t *network_status_label = nullptr;
    lv_obj_t *current_network_name_label = nullptr;
    lv_obj_t *password_textarea = nullptr;

    lv_obj_t *log_list = nullptr;
};

#endif //UI_H
