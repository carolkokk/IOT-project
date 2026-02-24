#include "UI.h"

#include <complex>
#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

#include "PicoSPIBus.h"
#include "PicoSPIDevice.h"

extern LVGLPort *g_lvgl_port;

// for now calibration values depend on the display
#define DISPLAY28
//#define DISPLAY24

// calibration values for decting touch
#ifdef DISPLAY28
#define TOUCH_X_MIN 340
#define TOUCH_X_MAX 3860
#define TOUCH_Y_MIN 275
#define TOUCH_Y_MAX 3890
#endif

#ifdef DISPLAY24
#define TOUCH_X_MIN  285
#define TOUCH_X_MAX  3951
#define TOUCH_Y_MIN  414
#define TOUCH_Y_MAX  3840
#endif

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, QueueHandle_t scan_results_queue,
    EventGroupHandle_t event_group, TickType_t period, uint32_t stack_size, UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control), scan_results_queue(scan_results_queue),
    event_group(event_group), period(period){

    init_UI();

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::task_wrap(void *pvParameters) {
    auto *ui = static_cast<UI*>(pvParameters);
    ui->task_impl();
}

void UI::task_impl() {
    // black background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x4a5756), 0);

    //test structure where UI sends a message to both Network and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send{};
    Message received{};

    sensor_data.temp = 0.0;
    sensor_data.rh = 0.0;
    //for testing initial value is a num
    sensor_data.target_rh = 50;
    sensor_data.type = TEMP_RH;

    current_screen = MAIN;
    screen_depth = 0;
    System_Status sys_status;
    sys_status.initial_main = true;
    load_main_screen(sensor_data, sys_status);
    bool prev_network_connected = false;

    while(true) {
        EventBits_t bits = xEventGroupGetBits(event_group);

        bool now_connected = bits & NETWORK_CONNECTED;
        bool now_connecting = bits & CONNECTING_NETWORK;

        sys_status.network_connected = now_connected;
        sys_status.connecting_network = now_connecting;

        if (now_connected != prev_network_connected) {
            prev_network_connected = now_connected;
            if (current_screen == MAIN) {
                update_main_screen(sys_status);
            }
        }

        while (xQueueReceive(to_UI,&received,pdMS_TO_TICKS(10))) {
            if (received.type == TEMP_RH) {
                sensor_data.type = TEMP_RH;
                sensor_data.rh = received.rh;
                sensor_data.temp = received.temp;
                if (current_screen == MAIN) {
                    sys_status.initial_main = false;
                    load_main_screen(sensor_data, sys_status);
                }
                printf("UI received TEMP: %.2f\n", received.temp);
                printf("UI received RH: %.2f\n", received.rh);
            }
            if (received.type == TARGET_RH) {
                sensor_data.target_rh = received.target_rh;
                printf("UI RECEIVED set rh: %d", received.target_rh);
            }
        }

        Scan_result_msg scan_msg{};
        if (xQueueReceive(scan_results_queue, &scan_msg, 0) == pdTRUE) {
            if (current_screen == AVAILABLE_NETWORKS) {
                lv_obj_clean(lv_screen_active());
                load_available_networks(scan_msg);
                networks_loaded = true;
            }
        }

        lv_timer_handler();

        // check for flags
        if (menu_selected) {
            menu_selected = false;
            switch (menu_selection) {
                case 0:
                    navigate_to(SET_RH);
                    break;
                case 1:
                    navigate_to(NETWORK);
                    break;
            }
        }

        if (rh_val_saved) {
            rh_val_saved = false;
            sensor_data.target_rh = set_rh_value;
            // send new target rh value to queues
            Message msg{};
            msg.type = TARGET_RH;
            msg.target_rh = sensor_data.target_rh;
            xQueueSendToBack(to_Control, &msg, portMAX_DELAY);
            xQueueSendToBack(to_Network, &msg, portMAX_DELAY);

            //next_screen = MAIN;
            navigate_back();
        }

        if (current_screen != next_screen) {
            current_screen = next_screen;

            // cleans up current screen and sets blck background again before displayinf new screen
            lv_obj_clean(lv_screen_active());
            lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x4a5756), 0);

            switch (current_screen) {
                case MAIN:
                    load_main_screen(sensor_data, sys_status);
                    break;
                case SET_RH:
                    load_rh_set_screen(sensor_data.target_rh);
                    break;
                case PRESET_SELECT:
                    load_preset_screen();
                    break;
                case NETWORK:
                    networks_loaded = false;
                    load_network_screen(sys_status.network_connected);
                    break;
                case AVAILABLE_NETWORKS: {
                    xEventGroupSetBits(event_group, START_SCAN);
                    networks_loaded = false;
                    lv_obj_t *label = lv_label_create(lv_screen_active());
                    lv_label_set_text(label, "Searching for networks...");
                    lv_obj_set_style_text_color(label, lv_color_white(), 0);
                    lv_obj_center(label);
                    break;
                }
                case ENTER_PASS:
                    load_password_screen();
                    break;
            }
        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}

void UI::init_UI() {
    // spi device initialization
    // for display, 30MH is used, so a different bus
    spi_0 = std::make_shared<PicoSPIBus>(0, 6, 7, 4, PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 30000000});
    display_device = std::make_shared<PicoSPIDevice>(spi_0, 9);

    //specific device initialization with dedicated pins
    display = std::make_shared<ili9341>(display_device, 10, 11, 5, 240, 320, 3);

    // creating and initializing lvgl port
    lvgl_port = std::make_shared<LVGLPort>(display);
    lvgl_port->init();
    // setting global ptr for timer callb
    g_lvgl_port = lvgl_port.get();

    // for the touch detection, 1MH is used
    spi_1 = std::make_shared<PicoSPIBus>(1, 14, 15, 12,
                                        PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 1000000});
    touch_device = std::make_shared<PicoSPIDevice>(spi_1, 13);

    // irq is enabled and rotation is set for touch
    touch = std::make_shared<XPT2046_Touch>(touch_device.get());
    //touch->begin();
    touch->setRotation(0);

    //touch integration for lvgl
    lvgl_touch = std::make_shared<LVGLTouch>(touch.get(), 320, 240);
    // setting touch with calibrated values
    lvgl_touch->init();
    lvgl_touch->setCalibration(TOUCH_X_MIN, TOUCH_X_MAX, TOUCH_Y_MIN, TOUCH_Y_MAX);
}


void UI::load_main_screen(Message received, System_Status status) {
    //buffer for updating sensor data
    char buf[64];

    if (status.initial_main) {
        // a white rectangle for the background of displaying sensor info
        lv_obj_t *white_bck = lv_obj_create(lv_screen_active());
        lv_obj_set_size(white_bck, 170, 90);
        lv_obj_set_pos(white_bck, 15, 15);
        lv_obj_set_style_bg_color(white_bck, lv_color_hex(0xbfa782), 0);
        lv_obj_set_style_border_color(white_bck, lv_color_hex(0xb8945f), 0);
        lv_obj_set_style_arc_rounded(white_bck, 10, 0);

        // humidity label
        rh_label = lv_label_create(lv_screen_active());
        lv_obj_set_pos(rh_label, 25, 25);
        lv_obj_set_style_text_color(rh_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(rh_label, &lv_font_montserrat_24, 0);

        // temperature label
        temp_label = lv_label_create(lv_screen_active());
        lv_obj_set_pos(temp_label, 48, 65);
        lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);

        // dropdown menu test
        static const char* options = "Set Rh\n"
                                     "Set Network\n"
                                     "View Stats";

        lv_obj_t* dd = lv_dropdown_create(lv_screen_active());
        lv_dropdown_set_options_static(dd, options);
        lv_obj_align(dd, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
        lv_obj_set_style_bg_color(dd, lv_color_hex(0x2162cc), 0);
        lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
        lv_dropdown_set_text(dd, "Menu");
        lv_dropdown_set_symbol(dd, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_color(dd, lv_color_hex(0x83cdf2), 0);
        lv_obj_set_style_border_color(dd, lv_color_hex(0x2b64ad), 0);


        lv_label_set_text(rh_label, "RH:   --");
        lv_label_set_text(temp_label, "T:   --");

        //adding callback to react to different menu selection items
        lv_obj_add_event_cb(dd, dd_menu_callback, LV_EVENT_VALUE_CHANGED, this);

        network_icon = lv_label_create(lv_screen_active());
        lv_label_set_text(network_icon, LV_SYMBOL_WIFI);
        lv_obj_set_pos(network_icon, 40, 150);

        lv_obj_set_style_text_font(network_icon, &lv_font_montserrat_24, 0);
        lv_color_t wifi_status_color = status.network_connected ? lv_color_hex(0x18cc57) : lv_color_hex(0x0040ff);
        lv_obj_set_style_text_color(network_icon, wifi_status_color, 0);

        tank_icon = lv_label_create(lv_screen_active());
        lv_obj_set_style_text_color(tank_icon, lv_color_white(), 0);
        lv_label_set_text(tank_icon, LV_SYMBOL_TINT);
        lv_obj_set_pos(tank_icon, 45, 115);
        lv_obj_set_style_text_font(tank_icon, &lv_font_montserrat_24, 0);
        //lv_label_set_text(tank_label, "Water level OK");

        lv_obj_set_style_text_color(tank_icon, lv_color_hex(0xe0ae67), 0);

        /*network_status_label = lv_label_create(lv_screen_active());
        lv_obj_set_style_text_color(network_status_label, lv_color_white(), 0);
        lv_label_set_text(network_status_label, "Network CONN");
        lv_obj_set_pos(network_status_label, 20, 140);
        lv_obj_set_style_text_color(network_status_label, lv_color_hex(0x4ad43b), 0);*/
    } else {
        // update labels with new data
        snprintf(buf, sizeof(buf), "RH:   %.2f %%", received.rh);
        lv_label_set_text(rh_label, buf);

        snprintf(buf, sizeof(buf), "T:   %.2f C", received.temp);
        lv_label_set_text(temp_label, buf);
    }
}

void UI::update_main_screen(System_Status status) {
    // update wifi icon color
    lv_color_t wifi_status_color = status.network_connected ? lv_color_hex(0x18cc57) : lv_color_hex(0x0040ff);
    lv_obj_set_style_text_color(network_icon, wifi_status_color, 0);

    // update

}


void UI::dd_menu_callback(lv_event_t* e) {
    auto ui = (UI*)lv_event_get_user_data(e);

    lv_obj_t* dd = lv_event_get_target_obj(e);

    ui->menu_selection = lv_dropdown_get_selected(dd);
    ui->menu_selected = true;
}

void UI::load_rh_set_screen(uint8_t target_rh) {
    // creating slider for adjusting the target humidity value
    lv_obj_t* slider = lv_slider_create(lv_screen_active());
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, this);

    lv_slider_set_range(slider, 35, 65);
    lv_slider_set_value(slider, target_rh, LV_ANIM_OFF);

    lv_obj_set_style_anim_duration(slider, 1000, 0);
    slider_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(slider_label, lv_color_white(), 0);

    //showing the current set rh as slider initial value
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", target_rh);

    lv_label_set_text(slider_label, buf);

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // button to save the new set rh
    create_button(save_btn_event_cb, LV_ALIGN_CENTER, 0, 0, "SAVE");
    // button to open pre-set page
    create_button(preset_btn_event_cb, LV_ALIGN_CENTER, 0, 0, "CHOOSE PRESET");
    // button to cancel
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::slider_event_cb(lv_event_t* e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_target_obj(e);

    ui->set_rh_value = (uint8_t)lv_slider_get_value(slider);

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", ui->set_rh_value);
    lv_label_set_text(ui->slider_label, buf);
    lv_obj_align_to(ui->slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void UI::save_btn_event_cb(lv_event_t* e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->rh_val_saved = true;
}

void UI::preset_btn_event_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->navigate_to(PRESET_SELECT);
}

void UI::load_preset_screen() {
    lv_style_init(&style_radio);
    lv_style_set_radius(&style_radio, LV_RADIUS_CIRCLE);

    lv_style_init(&style_radio_chk);
    lv_style_set_bg_image_src(&style_radio_chk, NULL);

    lv_obj_t* cont = lv_obj_create(lv_screen_active());
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 20);

    char buf[64];
    for (size_t i = 0; i < sizeof(PRESET_OPTIONS) / sizeof(PRESET_OPTIONS[0]); ++i) {
        lv_obj_t* obj = lv_checkbox_create(cont);
        lv_snprintf(buf, sizeof(buf), "%s (%d%%)",
                    PRESET_OPTIONS[i].name, PRESET_OPTIONS[i].rh_val);

        lv_checkbox_set_text(obj, buf);
        lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_style(obj, &style_radio, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_user_data(obj, reinterpret_cast<void *>(static_cast<uintptr_t>(PRESET_OPTIONS[i].rh_val)));
        lv_obj_add_event_cb(obj, preset_selection_cb, LV_EVENT_CLICKED, this);
    }

    create_button(save_preset_btn_callback, LV_ALIGN_BOTTOM_MID, 0, -20, "SAVE");
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::preset_selection_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    lv_obj_t* obj = lv_event_get_target_obj(e);
    lv_obj_t* cont = lv_obj_get_parent(obj);

    uint32_t child_count = lv_obj_get_child_count(cont);
    for (int32_t i = 0; i < child_count; ++i) {
        lv_obj_t* child = lv_obj_get_child(cont, i);
        if (child != obj) {
            lv_obj_clear_state(child, LV_STATE_CHECKED);
        }
    }

    ui->set_rh_value = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(obj)));
    printf("Selected RH: %d%%\n", ui->set_rh_value);
}

void UI::save_preset_btn_callback(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->rh_val_saved = true;
}

void UI::load_network_screen(bool network_connected) {
    network_status_label = lv_label_create(lv_screen_active());
    const char *status = network_connected ? "Status: CONNECTED" : "Status: DISCONNECTED";
    lv_label_set_text(network_status_label,  status);
    lv_obj_set_style_text_color(network_status_label, lv_color_white(), 0);
    lv_obj_align(network_status_label, LV_ALIGN_TOP_MID, 0, 30);
    create_button(search_networks_btn_cb, LV_ALIGN_TOP_MID, 0, 120, "NEW CONNECTION");
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::search_networks_btn_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    //xEventGroupSetBits(ui->event_group, START_SCAN);
    ui->navigate_to(AVAILABLE_NETWORKS);
}

void UI::load_available_networks(Scan_result_msg &msg) {
    network_list = lv_list_create(lv_screen_active());
    lv_obj_set_size(network_list, 250, 150);
    lv_obj_align(network_list, LV_ALIGN_TOP_MID, 0, 10);

    //add buttons to list
    lv_obj_t *btn;
    for (uint8_t i = 0; i < msg.result_count; ++i) {
        btn = lv_list_add_button(network_list, nullptr, msg.results[i].ssid);
        lv_obj_add_event_cb(btn, network_list_cb, LV_EVENT_CLICKED, this);
    }
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, "< ");
}

void UI::network_list_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    lv_obj_t * btn = lv_event_get_target_obj(e);
    const char *txt = lv_list_get_button_text(ui->network_list, btn);

    printf("Selected network: %s\n", txt);
    strncpy(ui->credentials.ssid, txt, sizeof(ui->credentials.ssid) - 1);
    ui->credentials.ssid[sizeof(ui->credentials.ssid) - 1] = '\0';

    ui->navigate_to(ENTER_PASS);
}

void UI::load_password_screen() {
    current_network_name_label = lv_label_create(lv_screen_active());
    lv_obj_set_pos(current_network_name_label, 30, 20);
    lv_label_set_text(current_network_name_label, credentials.ssid);
    lv_obj_set_style_text_color(current_network_name_label, lv_color_white(), 0);

    // text area
    lv_obj_t *text_area = lv_textarea_create(lv_screen_active());
    lv_obj_align(text_area, LV_ALIGN_TOP_MID, 10, 45);
    lv_obj_set_size(text_area, 280, 20);
    lv_textarea_set_placeholder_text(text_area, "Password");
    lv_textarea_set_one_line(text_area, true);
    lv_textarea_set_password_mode(text_area, true);

    lv_obj_t *keyb = lv_keyboard_create(lv_screen_active());
    lv_keyboard_set_textarea(keyb, text_area);

    lv_obj_add_event_cb(keyb, keyboard_cb, LV_EVENT_ALL, this);

    password_textarea = text_area;
}

void UI::keyboard_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        const char *pass = lv_textarea_get_text(ui->password_textarea);
        strncpy(ui->credentials.pass, pass, sizeof(ui->credentials.pass) - 1);
        ui->credentials.pass[sizeof(ui->credentials.pass) - 1] = '\0';

        printf("Connecting. SSID: %s, PASS: %s\n", ui->credentials.ssid, ui->credentials.pass);

        //sending credentials to network task to connect
        Message msg{};
        msg.type = NETWORK_CREDENTIALS;
        msg.credentials = ui->credentials;
        xQueueSendToBack(ui->to_Network, &msg, portMAX_DELAY);
        xEventGroupSetBits(ui->event_group, CONNECTING_NETWORK);

        ui->navigate_to(MAIN);
    } else if (code == LV_EVENT_CANCEL) {
        ui->navigate_back();
    }
}

void UI::create_button(lv_event_cb_t event_cb, lv_align_t align, int32_t x_ofs, int32_t y_ofs, const char *text) {
    lv_obj_t* btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2162cc), 0);
    lv_obj_set_style_text_color(btn, lv_color_hex(0x83cdf2), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2b64ad), 0);
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, text);
    lv_obj_center(btn_label);
}

void UI::navigate_to(Screens screen) {
    if (screen_depth < 5) {
        screen_history[screen_depth++] = current_screen;
    }
    next_screen = screen;
}

void UI::navigate_back() {
    if (screen_depth > 0) {
        next_screen = screen_history[--screen_depth];
    } else {
        next_screen = MAIN;
    }
}

void UI::back_btn_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->navigate_back();
}