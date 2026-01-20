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
#define TOUCH_X_MAX 3840
#define TOUCH_Y_MIN 275
#define TOUCH_Y_MAX 3890

#define TOUCH_ENABLE 0

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    // spi device initialization
    // for display, 30MH is used, so a different bus
    spi_0 = std::make_shared<PicoSPIBus>(0, 6, 7, 8, PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 30000000});
    display_device = std::make_shared<PicoSPIDevice>(spi_0, 9);

    //specific device initialization with dedicated pins
    display = std::make_shared<ili9341>(display_device, 10, 11, UINT_MAX, 240, 320, 3);

    // creating and initializing lvgl port
    lvgl_port = std::make_shared<LVGLPort>(display);
    lvgl_port->init();
    // setting global ptr for timer callb
    g_lvgl_port = lvgl_port.get();

#ifdef TOUCH_ENABLE
    // for the touch detection, 1MH is used
    spi_1 = std::make_shared<PicoSPIBus>(1, 14, 15, 12,
                                        PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 1000000});
    touch_device = std::make_shared<PicoSPIDevice>(spi_1, 13);

    // irq is enabled and rotation is set for touch
    touch = std::make_shared<XPT2046_Touch>(touch_device.get(), 255);
    touch->begin();
    touch->setRotation(3);

    //touch integration for lvgl
    lvgl_touch = std::make_shared<LVGLTouch>(touch.get(), 320, 240);
    // setting touch with calibrated values
    lvgl_touch->init();
    lvgl_touch->setCalibration(TOUCH_X_MIN, TOUCH_X_MAX, TOUCH_Y_MIN, TOUCH_Y_MAX);

#endif

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::task_wrap(void *pvParameters) {
    auto *ui = static_cast<UI*>(pvParameters);
    ui->task_impl();
}

void UI::task_impl() {
    // lvgl elements:

    // black background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

    /*
    // test button to see if all works
    lv_obj_t* btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 100);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Touch Me!");
    lv_obj_center(btn_label);
    */
    /*
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        printf("BUTTON CLICKED!\n");
    }, LV_EVENT_CLICKED, nullptr);
    */

    //test structure where UI sends a message to both Network and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_msg{};
    Message received{};
    send_msg.type = TEST_STRING;
    strncpy(send_msg.string, "Test string from UI task.", sizeof(send_msg.string)-1);
    send_msg.string[sizeof(send_msg.string)-1] = '\0';

    load_main_screen(received, true);

    while(true) {

        //xQueueSendToBack(to_Control, &send_msg, portMAX_DELAY);
        //xQueueSendToBack(to_Network, &send_msg, portMAX_DELAY);

        while (xQueueReceive(to_UI,&received,pdMS_TO_TICKS(10))) {
            /*if (received.type == TEST_STRING){
                printf("received %s\n",received.string);
            }
            else if (received.type == TEST_NUMBER){
                printf("received %u\n",received.number);
            }*/
            if (received.type == TEMP_RH) {
                load_main_screen(received, false);
                printf("UI received TEMP: %.2f\n", received.temp);
                printf("UI received RH: %.2f\n", received.rh);
            }
        }
        lv_timer_handler();

        vTaskDelayUntil(&lastWakeTime, period);
    }
}

void UI::load_main_screen(Message received, bool initial) {
    //buffer for updating sensor data
    char buf[64];

    if (initial) {
        // humidity label
        rh_label = lv_label_create(lv_screen_active());
        lv_obj_set_pos(rh_label, 10, 20);
        lv_obj_set_style_text_color(rh_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(rh_label, &lv_font_montserrat_24, 0);

        // temperature label
        temp_label = lv_label_create(lv_screen_active());
        lv_obj_set_pos(temp_label, 33, 60);
        lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);

        // dropdown menu test
        static const char* options = "Set Rh\n"
                                     "Set Network\n"
                                     "View Stats";

        lv_obj_t* dd = lv_dropdown_create(lv_screen_active());
        lv_dropdown_set_options_static(dd, options);
        lv_obj_align(dd, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
        lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
        lv_dropdown_set_text(dd, "Menu");
        lv_dropdown_set_symbol(dd, LV_SYMBOL_SETTINGS);
        lv_label_set_text(rh_label, "RH:   --");
        lv_label_set_text(temp_label, "T:   --");

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

