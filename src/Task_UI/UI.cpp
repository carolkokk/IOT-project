#include "UI.h"
#include <cstdio>
#include "Structs.h"
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

#include "PicoSPIBus.h"
#include "PicoSPIDevice.h"

extern LVGLPort *g_lvgl_port;

// calibration values for decting touch
#define TOUCH_X_MIN 340
#define TOUCH_X_MAX 3840
#define TOUCH_Y_MIN 275
#define TOUCH_Y_MAX 3890

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    // spi device initialization
    // for display, 30MH is used, so a different bus
    spi_0 = std::make_shared<PicoSPIBus>(0, 6, 7, 4, PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 30000000});
    display_device = std::make_shared<PicoSPIDevice>(spi_0, 9);

    // for the touch detection, 1MH is used
    spi_1 = std::make_shared<PicoSPIBus>(1, 14, 15, 12,
                                        PicoSPIBus::SPI_config {8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST, 1000000});
    touch_device = std::make_shared<PicoSPIDevice>(spi_1, 13);

    //specific device initialization with dedicated pins
    display = std::make_shared<ili9341>(display_device, 10, 11, 2, 240, 320, 3);

    // irq is enabled and rotation is set for touch
    touch = std::make_shared<XPT2046_Touch>(touch_device.get(), 255);
    touch->begin();
    touch->setRotation(3);

    // creating and initializing lvgl port
    lvgl_port = std::make_shared<LVGLPort>(display);
    lvgl_port->init();
    // setting global ptr for timer callb
    g_lvgl_port = lvgl_port.get();

    //touch integration for lvgl
    lvgl_touch = std::make_shared<LVGLTouch>(touch.get(), 320, 240);
    // setting touch with calibrated values
    lvgl_touch->init();
    lvgl_touch->setCalibration(TOUCH_X_MIN, TOUCH_X_MAX, TOUCH_Y_MIN, TOUCH_Y_MAX);

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::task_wrap(void *pvParameters) {
    auto *ui = static_cast<UI*>(pvParameters);
    ui->task_impl();
}

void UI::task_impl() {
    // lvgl elements:

    // black background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);

    // humidity label
    rh_label = lv_label_create(lv_screen_active());
    lv_label_set_text(rh_label, "RH:   --");
    lv_obj_set_pos(rh_label, 10, 20);
    lv_obj_set_style_text_color(rh_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(rh_label, &lv_font_montserrat_24, 0);

    // temperature label
    temp_label = lv_label_create(lv_screen_active());
    lv_label_set_text(temp_label, "T:   --");
    lv_obj_set_pos(temp_label, 33, 60);
    lv_obj_set_style_text_color(temp_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);

    // test button to see if all works
    lv_obj_t* btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 100);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Touch Me!");
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        printf("BUTTON CLICKED!\n");
    }, LV_EVENT_CLICKED, nullptr);

    // dropdown menu test
    static const char* options = "Set Rh\n"
                                 "Set Network\n"
                                 "View Stats";

    lv_obj_t* dd = lv_dropdown_create(lv_screen_active());
    lv_dropdown_set_options_static(dd, options);
    lv_obj_align(dd, LV_ALIGN_TOP_RIGHT, -10, 40);
    lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
    lv_dropdown_set_text(dd, "Menu");
    lv_dropdown_set_symbol(dd, LV_SYMBOL_SETTINGS);

    //test structure where UI sends a message to both Network and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_msg{};
    Message received{};
    send_msg.type = TEST_STRING;
    strncpy(send_msg.string, "Test string from UI task.", sizeof(send_msg.string)-1);
    send_msg.string[sizeof(send_msg.string)-1] = '\0';

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
                printf("UI received TEMP: %.2f\n", received.temp);
                printf("UI received RH: %.2f\n", received.rh);

                // update labels with new data
                char buf[64];
                snprintf(buf, sizeof(buf), "RH:   %.2f %%", received.rh);
                lv_label_set_text(rh_label, buf);

                snprintf(buf, sizeof(buf), "T:   %.2f C", received.temp);
                lv_label_set_text(temp_label, buf);

            }
        }
        lv_timer_handler();

        vTaskDelayUntil(&lastWakeTime, period);
    }
}
