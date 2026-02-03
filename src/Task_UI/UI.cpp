#include "UI.h"

#include <complex>
#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <math.h>

#include "PicoSPIBus.h"
#include "PicoSPIDevice.h"

extern LVGLPort *g_lvgl_port;

// calibration values for decting touch
#define TOUCH_X_MIN 340
#define TOUCH_X_MAX 3860
#define TOUCH_Y_MIN 275
#define TOUCH_Y_MAX 3890

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    init_UI();

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::task_wrap(void *pvParameters) {
    auto *ui = static_cast<UI*>(pvParameters);
    ui->task_impl();
}

void UI::task_impl() {
    // black background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x414445), 0);

    //test structure where UI sends a message to both Network and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send{};
    Message received{};
    send.type = TEST_STRING;
    strncpy(send.string, "Test string from UI task.", sizeof(send.string)-1);
    send.string[sizeof(send.string)-1] = '\0';

    current_screen = MAIN;
    next_screen = MAIN;

    sensor_data.temp = 0.0;
    sensor_data.rh = 0.0;
    //for testing initial value is a num
    sensor_data.target_rh = 50;
    sensor_data.type = TEMP_RH;

    load_main_screen(sensor_data, true);

    while(true) {
        while (xQueueReceive(to_UI,&received,pdMS_TO_TICKS(10))) {
            /*if (received.type == TEST_STRING){
                printf("received %s\n",received.string);
            }
            else if (received.type == TEST_NUMBER){
                printf("received %u\n",received.number);
            }*/
            if (received.type == TEMP_RH) {
                sensor_data.type = TEMP_RH;
                sensor_data.rh = received.rh;
                sensor_data.temp = received.temp;
                if (current_screen == MAIN) {
                    load_main_screen(sensor_data, false);
                }
                printf("UI received TEMP: %.2f\n", received.temp);
                printf("UI received RH: %.2f\n", received.rh);
            }
            if (received.type == TARGET_RH) {
                sensor_data.target_rh = received.target_rh;
                printf("UI RECEIVED set rh: %d", received.target_rh);
            }
        }
        lv_timer_handler();

        // check for flags
        if (menu_selected) {
            menu_selected = false;
            switch (menu_selection) {
                case 0:
                    next_screen = SET_RH;
                    break;
                case 1:
                    next_screen = SET_NETWORK;
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

            next_screen = MAIN;
        }

        if (current_screen != next_screen) {
            current_screen = next_screen;

            // cleans up current screen and sets blck background again before displayinf new screen
            lv_obj_clean(lv_screen_active());
            lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x414445), 0);

            switch (current_screen) {
                case MAIN:
                    load_main_screen(sensor_data, true);
                    break;
                case SET_RH:
                    load_rh_set_screen(sensor_data.target_rh);
                    break;
                case PRESET_SELECT:
                    load_preset_screen();
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
    touch->setRotation(3);

    //touch integration for lvgl
    lvgl_touch = std::make_shared<LVGLTouch>(touch.get(), 320, 240);
    // setting touch with calibrated values
    lvgl_touch->init();
    lvgl_touch->setCalibration(TOUCH_X_MIN, TOUCH_X_MAX, TOUCH_Y_MIN, TOUCH_Y_MAX);
}


void UI::load_main_screen(Message received, bool initial) {
    //buffer for updating sensor data
    char buf[64];

    if (initial) {
        // a white rectangle for the background of displaying sensor info
        lv_obj_t *white_bck = lv_obj_create(lv_screen_active());
        lv_obj_set_size(white_bck, 170, 90);
        lv_obj_set_pos(white_bck, 5, 10);
        lv_obj_set_style_bg_color(white_bck, lv_color_white(), 0);
        lv_obj_set_style_arc_rounded(white_bck, 10, 0);

        // humidity label
        rh_label = lv_label_create(lv_screen_active());
        lv_obj_set_pos(rh_label, 10, 20);
        lv_obj_set_style_text_color(rh_label, lv_color_black(), 0);
        lv_obj_set_style_text_font(rh_label, &lv_font_montserrat_24, 0);

        // temperature label
        temp_label = lv_label_create(lv_screen_active());
        lv_obj_set_pos(temp_label, 33, 60);
        lv_obj_set_style_text_color(temp_label, lv_color_black(), 0);
        lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);

        // dropdown menu test
        static const char* options = "Set Rh\n"
                                     "Set Network\n"
                                     "View Stats";

        lv_obj_t* dd = lv_dropdown_create(lv_screen_active());
        lv_dropdown_set_options_static(dd, options);
        lv_obj_align(dd, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
        lv_obj_set_style_bg_color(dd, lv_color_hex(0x8fa4b0), 0);
        lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
        lv_dropdown_set_text(dd, "Menu");
        lv_dropdown_set_symbol(dd, LV_SYMBOL_SETTINGS);
        lv_label_set_text(rh_label, "RH:   --");
        lv_label_set_text(temp_label, "T:   --");

        //adding callback to react to different menu selection items
       lv_obj_add_event_cb(dd, dd_menu_callback, LV_EVENT_VALUE_CHANGED, this);

        // creating led for indicating water tank
        /*
        led  = lv_led_create(lv_screen_active());
        //lv_color_t blue = lv_color_make(64, 125, 237);
        lv_led_set_color(led, lv_color_white());
        lv_obj_align(led, LV_ALIGN_LEFT_MID, 20, 10);
        lv_led_on(led);*/
        tank_label = lv_label_create(lv_screen_active());
        lv_obj_set_style_text_color(tank_label, lv_color_white(), 0);
        lv_label_set_text(tank_label, "Water level OK");
        lv_obj_set_pos(tank_label, 20, 120);
        lv_obj_set_style_text_color(tank_label, lv_color_hex(0x217feb), 0);

        network_label = lv_label_create(lv_screen_active());
        lv_obj_set_style_text_color(network_label, lv_color_white(), 0);
        lv_label_set_text(network_label, "Network CONN");
        lv_obj_set_pos(network_label, 20, 140);
        lv_obj_set_style_text_color(network_label, lv_color_hex(0x4ad43b), 0);
    } else {
        // update labels with new data
        snprintf(buf, sizeof(buf), "RH:   %.2f %%", received.rh);
        lv_label_set_text(rh_label, buf);

        snprintf(buf, sizeof(buf), "T:   %.2f C", received.temp);
        lv_label_set_text(temp_label, buf);
    }
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
    lv_obj_t* save_btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(save_btn, save_btn_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_align(save_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x8fa4b0), 0);

    lv_obj_t* btn_label = lv_label_create(save_btn);
    lv_label_set_text(btn_label, "SAVE");
    lv_obj_center(btn_label);

    // button to open pre-set page
    lv_obj_t* preset_btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(preset_btn, preset_btn_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_align(preset_btn, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t* preset_btn_label = lv_label_create(preset_btn);
    lv_label_set_text(preset_btn_label, "CHOOSE PRESET");
    lv_obj_center(preset_btn_label);

    // button to cancel
    lv_obj_t* cancel_btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(cancel_btn, cancel_slider_btn_callback, LV_EVENT_CLICKED, this);
    lv_obj_align(cancel_btn , LV_ALIGN_BOTTOM_LEFT, 0, -10);
    lv_obj_set_style_bg_color(cancel_btn , lv_color_hex(0x8fa4b0), 0);

    lv_obj_t* cancel_btn_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_btn_label, " < ");
    lv_obj_center(cancel_btn_label);
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
    ui->next_screen = PRESET_SELECT;
}

void UI::cancel_slider_btn_callback(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->next_screen = MAIN;
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

    lv_obj_t* save_preset_btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(save_preset_btn, save_preset_btn_callback, LV_EVENT_CLICKED, this);
    lv_obj_align(save_preset_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(save_preset_btn, lv_color_hex(0x8fa4b0), 0);

    lv_obj_t* btn_label = lv_label_create(save_preset_btn);
    lv_label_set_text(btn_label, "SAVE");
    lv_obj_center(btn_label);

    lv_obj_t* cancel_btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(cancel_btn, cancel_preset_btn_callback, LV_EVENT_CLICKED, this);
    lv_obj_align(cancel_btn , LV_ALIGN_BOTTOM_LEFT, 0, -10);
    lv_obj_set_style_bg_color(cancel_btn , lv_color_hex(0x8fa4b0), 0);

    lv_obj_t* cancel_btn_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_btn_label, " < ");
    lv_obj_center(cancel_btn_label);
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

void UI::cancel_preset_btn_callback(lv_event_t *e) {
    auto ui = (UI*)lv_event_get_user_data(e);
    ui->next_screen = SET_RH;
}

void UI::create_save_button(lv_event_cb_t* event_cb) {
    lv_obj_t* save_btn = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(save_btn, save_btn_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_align(save_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x8fa4b0), 0);

    lv_obj_t* btn_label = lv_label_create(save_btn);
    lv_label_set_text(btn_label, "SAVE");
    lv_obj_center(btn_label);
}