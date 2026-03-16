#include "UI.h"

#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

#include "PicoSPIBus.h"
#include "PicoSPIDevice.h"
#include "display/ili9341.h"

//colors
static constexpr uint32_t COLOR_BG           = 0x4a5756;
static constexpr uint32_t COLOR_PANEL        = 0xbfa782;
static constexpr uint32_t COLOR_PANEL_BORDER = 0xb8945f;
static constexpr uint32_t COLOR_BTN          = 0x2162cc;
static constexpr uint32_t COLOR_BTN_TEXT     = 0x83cdf2;
static constexpr uint32_t COLOR_BTN_BORDER   = 0x2b64ad;
static constexpr uint32_t COLOR_WIFI_ON      = 0x18cc57;
static constexpr uint32_t COLOR_WIFI_OFF     = 0x0040ff;
static constexpr uint32_t COLOR_TANK         = 0xe0ae67;

//calibration
static constexpr uint16_t TOUCH_X_MIN_DEFAULT = 272;
static constexpr uint16_t TOUCH_X_MAX_DEFAULT = 3839;
static constexpr uint16_t TOUCH_Y_MIN_DEFAULT = 190;
static constexpr uint16_t TOUCH_Y_MAX_DEFAULT = 3839;

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, QueueHandle_t scan_results_queue, QueueHandle_t credentials_to_network,
    EventGroupHandle_t event_group, TickType_t period, std::shared_ptr<EEPROM> eeprom,
    uint32_t stack_size, UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network), to_Control(to_Control), scan_results_queue(scan_results_queue), credentials_to_network(credentials_to_network),
    event_group(event_group), period(period), eeprom(std::move(eeprom)) {

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::task_wrap(void *pvParameters) {
    auto *ui = static_cast<UI*>(pvParameters);
    ui->task_impl();
}

void UI::task_impl() {
    init_UI();
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(COLOR_BG), 0);
    init_task_state();

    TickType_t lastWakeTime = xTaskGetTickCount();
    Message msg{};
    Message received{};
    bool prev_network_connected = false;
    bool prev_refill = false;
    bool prev_overflow = false;

    while(true) {
        EventBits_t bits = xEventGroupGetBits(event_group);

        sys_status.network_connected = bits & NETWORK_CONNECTED;
        sys_status.connecting_network = bits & CONNECTING_NETWORK;
        sys_status.bad_auth = bits & BAD_AUTH;

        if (sys_status.network_connected != prev_network_connected) {
            prev_network_connected = sys_status.network_connected;
            if (current_screen == MAIN) {
                update_wifi_status(sys_status);
            }
        }
        if (sys_status.bad_auth && current_screen == CONNECTING_WIFI) {
            xEventGroupClearBits(event_group, BAD_AUTH | CONNECTING_NETWORK);
            eeprom->writeLog("Bad wi-fi credentials");
            navigate_to(MAIN);
        }

        sys_status.refill_water = bits & EVT_NO_WATER;
        sys_status.water_overflow = bits & EVT_WATER_PRESENT;

        if (sys_status.refill_water != prev_refill || sys_status.water_overflow != prev_overflow) {
            prev_refill = sys_status.refill_water;
            prev_overflow = sys_status.water_overflow;
            if (current_screen == MAIN) {
                update_water_status(sys_status);
            }
        }

        while (xQueueReceive(to_UI, &received, pdMS_TO_TICKS(10))) {
            if (received.type == TEMP_RH) {
                sensor_data.type = TEMP_RH;
                sensor_data.rh = received.rh;
                sensor_data.temp = received.temp;
                if (current_screen == MAIN) {
                    update_main_screen(sensor_data);
                }
                printf("UI received TEMP: %.2f\n", received.temp);
                printf("UI received RH: %.2f\n", received.rh);
            }
            if (received.type == TARGET_RH) {
                sensor_data.target_rh = received.target_rh;
                eeprom->eepromWrite(RH_SET_ADDR, &sensor_data.target_rh, sizeof(sensor_data.target_rh));
                char log_buf[32];
                snprintf(log_buf, sizeof(log_buf), "Set RH: %d%%", received.target_rh);
                eeprom->writeLog(log_buf);
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

        if (menu_selected) {
            menu_selected = false;
            switch (menu_selection) {
                case 0: navigate_to(SET_RH);     break;
                case 1: navigate_to(NETWORK);    break;
                case 2: navigate_to(STATISTICS); break;
            }
        }

        if (rh_val_saved) {
            rh_val_saved = false;
            sensor_data.target_rh = set_rh_value;
            eeprom->eepromWrite(RH_SET_ADDR, &sensor_data.target_rh, sizeof(sensor_data.target_rh));

            msg.type = TARGET_RH;
            msg.target_rh = sensor_data.target_rh;
            xQueueSendToBack(to_Control, &msg, portMAX_DELAY);
            if (bits & NETWORK_CONNECTED) {
                xQueueSendToBack(to_Network, &msg, portMAX_DELAY);
            }
            char log_buf[32];
            snprintf(log_buf, sizeof(log_buf), "Set RH: %d%%", sensor_data.target_rh);
            eeprom->writeLog(log_buf);
            navigate_back();
        }

        if (current_screen == CONNECTING_WIFI) {
            if (!(bits & CONNECTING_NETWORK)) {
                navigate_to(MAIN);
            }
        }

        if (current_screen != next_screen) {
            current_screen = next_screen;

            lv_obj_clean(lv_screen_active());
            lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(COLOR_BG), 0);

            switch (current_screen) {
                case MAIN:
                    tank_status_label = nullptr;
                    build_main_screen(sys_status);
                    if (sys_status.bad_auth) {
                        update_wifi_status(sys_status);
                    }
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
                case CONNECTING_WIFI:
                    load_connecting_wifi_screen();
                    break;
                case STATISTICS:
                    load_statistics_screen();
                    break;
                case MEASUREMENT_CHART:
                    load_measurement_chart_screen();
                    break;
                case LOG_HISTORY:
                    load_log_history_screen();
                    break;
            }
        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}

void UI::init_task_state() {
    sensor_data.temp = 0.0;
    sensor_data.rh = 0.0;
    eeprom->eepromRead(RH_SET_ADDR, &sensor_data.target_rh, sizeof(sensor_data.target_rh));

    Message msg{};
    msg.type = TARGET_RH;
    msg.target_rh = sensor_data.target_rh;
    xQueueSendToBack(to_Control, &msg, portMAX_DELAY);

    if (sensor_data.target_rh < min_set_rh || sensor_data.target_rh > max_set_rh) {
        sensor_data.target_rh = 50;  // default
    }
    sensor_data.type = TEMP_RH;

    current_screen = MAIN;
    screen_depth = 0;
    build_main_screen(sys_status);
}

void UI::init_UI() {
    spi_0 = std::make_shared<PicoSPIBus>(0, 6, 7, 4, PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 30000000});
    display_device = std::make_shared<PicoSPIDevice>(spi_0, 9);

    drv = std::make_shared<ili9341>(display_device, 10, 11, 5, 240, 320, 3);

    lvgl_port = std::make_shared<LVGLPort>(drv);
    lvgl_port->init();

    spi_1 = std::make_shared<PicoSPIBus>(1, 14, 15, 12,
                                         PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 1000000});
    touch_device = std::make_shared<PicoSPIDevice>(spi_1, 13);

    touch = std::make_shared<XPT2046_Touch>(touch_device);
    touch->setRotation(3);

    lvgl_touch = std::make_shared<LVGLTouch>(touch, 320, 240);
    lvgl_touch->init();
    load_or_init_calibration();
}

void UI::load_or_init_calibration() {
    static constexpr uint8_t  CAL_MAGIC = 0xCA;
    uint8_t buf[9];
    if (eeprom->eepromRead(TOUCH_CAL_ADDR, buf, sizeof(buf)) && buf[0] == CAL_MAGIC) {
        uint16_t x_min, x_max, y_min, y_max;
        memcpy(&x_min, buf + 1, 2);
        memcpy(&x_max, buf + 3, 2);
        memcpy(&y_min, buf + 5, 2);
        memcpy(&y_max, buf + 7, 2);
        lvgl_touch->setCalibration(x_min, x_max, y_min, y_max);
    } else {
        uint8_t wbuf[9];
        wbuf[0] = CAL_MAGIC;
        uint16_t vals[4] = {TOUCH_X_MIN_DEFAULT, TOUCH_X_MAX_DEFAULT,
                            TOUCH_Y_MIN_DEFAULT, TOUCH_Y_MAX_DEFAULT};
        memcpy(wbuf + 1, vals, 8);
        eeprom->eepromWrite(TOUCH_CAL_ADDR, wbuf, sizeof(wbuf));
        lvgl_touch->setCalibration(TOUCH_X_MIN_DEFAULT, TOUCH_X_MAX_DEFAULT,
                                   TOUCH_Y_MIN_DEFAULT, TOUCH_Y_MAX_DEFAULT);
    }
}

void UI::build_main_screen(System_Status status) {
    lv_obj_t *panel = lv_obj_create(lv_screen_active());
    lv_obj_set_size(panel, 170, 90);
    lv_obj_set_pos(panel, 15, 15);
    lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(COLOR_PANEL_BORDER), 0);
    lv_obj_set_style_arc_rounded(panel, 10, 0);

    rh_label = lv_label_create(lv_screen_active());
    lv_obj_set_pos(rh_label, 25, 25);
    lv_obj_set_style_text_color(rh_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(rh_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(rh_label, "RH:   --");

    temp_label = lv_label_create(lv_screen_active());
    lv_obj_set_pos(temp_label, 48, 65);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(temp_label, "T:   --");

    static const char* options = "Set Rh\n"
                                 "Set Network\n"
                                 "View Stats";

    lv_obj_t* dd = lv_dropdown_create(lv_screen_active());
    lv_dropdown_set_options_static(dd, options);
    lv_obj_align(dd, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
    lv_obj_set_style_bg_color(dd, lv_color_hex(COLOR_BTN), 0);
    lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
    lv_dropdown_set_text(dd, "Menu");
    lv_dropdown_set_symbol(dd, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(dd, lv_color_hex(COLOR_BTN_TEXT), 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(COLOR_BTN_BORDER), 0);
    lv_obj_add_event_cb(dd, dd_menu_callback, LV_EVENT_VALUE_CHANGED, this);

    network_icon = lv_label_create(lv_screen_active());
    lv_label_set_text(network_icon, LV_SYMBOL_WIFI);
    lv_obj_set_pos(network_icon, 40, 150);
    lv_obj_set_style_text_font(network_icon, &lv_font_montserrat_24, 0);
    lv_color_t wifi_color = status.network_connected ? lv_color_hex(COLOR_WIFI_ON) : lv_color_hex(COLOR_WIFI_OFF);
    lv_obj_set_style_text_color(network_icon, wifi_color, 0);

    tank_icon = lv_label_create(lv_screen_active());
    lv_label_set_text(tank_icon, LV_SYMBOL_TINT);
    lv_obj_set_pos(tank_icon, 45, 115);
    lv_obj_set_style_text_font(tank_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(tank_icon, lv_color_hex(COLOR_TANK), 0);
}

void UI::update_main_screen(const Message& received) {
    char buf[64];
    snprintf(buf, sizeof(buf), "RH:   %.2f %%", received.rh);
    lv_label_set_text(rh_label, buf);

    snprintf(buf, sizeof(buf), "T:   %.2f C", received.temp);
    lv_label_set_text(temp_label, buf);
}

void UI::update_wifi_status(System_Status status) {
    lv_color_t wifi_color = status.network_connected ? lv_color_hex(COLOR_WIFI_ON) : lv_color_hex(COLOR_WIFI_OFF);
    lv_obj_set_style_text_color(network_icon, wifi_color, 0);
    if (status.bad_auth) {
        lv_obj_t * bad_auth_label = lv_label_create(lv_screen_active());
        lv_obj_set_style_text_color(bad_auth_label, lv_color_hex(COLOR_WIFI_OFF), 0);
        lv_label_set_text(bad_auth_label, "BAD PASS");
        lv_obj_set_pos(bad_auth_label, 75, 150);
    }
}

void UI::update_water_status(System_Status status) {
    if (tank_status_label == nullptr) {
        tank_status_label = lv_label_create(lv_screen_active());
        lv_obj_align(tank_status_label, LV_ALIGN_LEFT_MID, 65, 8);
        lv_obj_set_style_text_color(tank_status_label, lv_color_hex(COLOR_WIFI_OFF), 0);
    }

    if (status.refill_water) {
        lv_label_set_text(tank_status_label, "REFILL");
    } else if (status.water_overflow) {
        lv_label_set_text(tank_status_label, "OVERFLOW");
    } else {
        lv_label_set_text(tank_status_label, "");
    }
}

void UI::dd_menu_callback(lv_event_t* e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    lv_obj_t* dd = lv_event_get_target_obj(e);
    ui->menu_selection = lv_dropdown_get_selected(dd);
    ui->menu_selected = true;
}

void UI::load_rh_set_screen(uint8_t target_rh) {
    lv_obj_t* slider = lv_slider_create(lv_screen_active());
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_slider_set_range(slider, min_set_rh, max_set_rh);
    lv_slider_set_value(slider, target_rh, LV_ANIM_OFF);
    lv_obj_set_style_anim_duration(slider, 1000, 0);
    lv_obj_set_style_bg_color(slider, lv_color_hex(COLOR_PANEL), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(COLOR_PANEL_BORDER), LV_PART_KNOB);

    slider_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(slider_label, lv_color_white(), 0);

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", target_rh);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    create_button(save_btn_event_cb, LV_ALIGN_CENTER, 0, 40, "SAVE");
    create_button(preset_btn_event_cb, LV_ALIGN_CENTER, 0, 0, "CHOOSE PRESET");
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
        lv_snprintf(buf, sizeof(buf), "%s (%d%%)", PRESET_OPTIONS[i].name, PRESET_OPTIONS[i].rh_val);
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
    lv_label_set_text(network_status_label, status);
    lv_obj_set_style_text_color(network_status_label, lv_color_white(), 0);
    lv_obj_align(network_status_label, LV_ALIGN_TOP_MID, 0, 40);
    create_button(search_networks_btn_cb, LV_ALIGN_TOP_MID, 0, 120, "NEW CONNECTION");
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::search_networks_btn_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->navigate_to(AVAILABLE_NETWORKS);
}

void UI::load_available_networks(Scan_result_msg &msg) {
    network_list = lv_list_create(lv_screen_active());
    lv_obj_set_size(network_list, 250, 150);
    lv_obj_align(network_list, LV_ALIGN_TOP_MID, 0, 10);

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

        printf("Connecting to SSID: %s\n", ui->credentials.ssid);

        Network_credentials net_credentials{};
        net_credentials = ui->credentials;
        xEventGroupSetBits(ui->event_group, CONNECTING_NETWORK);
        xQueueSendToBack(ui->credentials_to_network, &net_credentials, pdMS_TO_TICKS(portMAX_DELAY));
        EventBits_t bits = xEventGroupGetBits(ui->event_group);
        if (bits & CONNECTING_NETWORK) {
            ui->navigate_to(CONNECTING_WIFI);
        } else {
            ui->navigate_to(MAIN);
        }
    } else if (code == LV_EVENT_CANCEL) {
        ui->navigate_back();
    }
}

void UI::load_connecting_wifi_screen() {
    lv_obj_t *spinner = lv_spinner_create(lv_screen_active());
    lv_obj_set_size(spinner, 100, 100);
    lv_obj_center(spinner);
    lv_spinner_set_anim_params(spinner, 5000, 100);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COLOR_PANEL), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COLOR_BTN), LV_PART_MAIN);
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::load_statistics_screen() {
    create_button(chart_button_cb, LV_ALIGN_CENTER, 0, -20, "T/RH HISTORY");
    create_button(log_button_cb, LV_ALIGN_CENTER, 0, 40, "SHOW LOGS");
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::chart_button_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->navigate_to(MEASUREMENT_CHART);
}

void UI::log_button_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->navigate_to(LOG_HISTORY);
}

void UI::load_measurement_chart_screen() {
    lv_obj_t *chart = lv_chart_create(lv_screen_active());
    lv_obj_set_size(chart, 280, 160);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 15, 10);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);

    lv_obj_t *scale = lv_scale_create(lv_screen_active());
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_obj_set_size(scale, 20, 160);
    lv_obj_set_style_text_color(scale, lv_color_white(), 0);
    lv_obj_align(scale, LV_ALIGN_TOP_LEFT, 15, 10);
    lv_scale_set_total_tick_count(scale, 11);
    lv_scale_set_major_tick_every(scale, 2);
    lv_scale_set_range(scale, 0, 100);

    lv_chart_series_t *rh_series   = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *temp_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED),  LV_CHART_AXIS_PRIMARY_Y);

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y,   0, 1000);
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0,  500);

    Measure_history samples[SAMPLE_COUNT];
    uint8_t count = 0;
    if (eeprom->readAllSamples(samples, count)) {
        lv_chart_set_point_count(chart, count);
        for (uint8_t i = 0; i < count; ++i) {
            lv_chart_set_next_value(chart, rh_series,   (int16_t)(samples[i].rh   * 10));
            lv_chart_set_next_value(chart, temp_series, (int16_t)(samples[i].temp * 10));
        }
    }
    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
}

void UI::load_log_history_screen() {
    log_list = lv_list_create(lv_screen_active());
    lv_obj_set_size(log_list, 250, 150);
    lv_obj_align(log_list, LV_ALIGN_TOP_MID, 0, 10);

    create_button(back_btn_cb, LV_ALIGN_BOTTOM_LEFT, 20, -10, " < ");
    create_button(delete_log_button_cb, LV_ALIGN_BOTTOM_RIGHT, -20, -10, "DELETE ALL");

    auto logs = eeprom->getAllLogs();
    if (logs.empty()) {
        lv_list_add_text(log_list, "No logs found");
        return;
    }
    for (const auto& log : logs) {
        lv_list_add_text(log_list, log.c_str());
    }
}

void UI::delete_log_button_cb(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->eeprom->deleteLogs();
    lv_obj_clean(lv_screen_active());
    ui->log_list = nullptr;
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(COLOR_BG), 0);
    ui->load_log_history_screen();
}

void UI::create_button(lv_event_cb_t event_cb, lv_align_t align, int32_t x_ofs, int32_t y_ofs, const char *text) {
    lv_obj_t* btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BTN), 0);
    lv_obj_set_style_text_color(btn, lv_color_hex(COLOR_BTN_TEXT), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(COLOR_BTN_BORDER), 0);
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, text);
    lv_obj_center(btn_label);
}

void UI::navigate_to(Screens screen) {
    configASSERT(screen_depth < 5);
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
