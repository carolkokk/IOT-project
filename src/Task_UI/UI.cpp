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

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    auto spi = std::make_shared<PicoSPIBus>(0, 6, 7, 4);
    auto dev = std::make_shared<PicoSPIDevice>(spi, 9);
    display = std::make_shared<ili9341>(dev, 10, 11, 13, 240, 320, 3);

    // creating and initializing lvgl port
    lvgl_port = std::make_shared<LVGLPort>(display);
    lvgl_port->init();
    // setting global ptr for timer callb
    g_lvgl_port = lvgl_port.get();

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

    //test structure where UI sends a message to both Network and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_msg{};
    Message received{};
    send_msg.type = TEST_STRING;
    strncpy(send_msg.string, "Test string from UI task.", sizeof(send_msg.string)-1);
    send_msg.string[sizeof(send_msg.string)-1] = '\0';

    while(true) {

        xQueueSendToBack(to_Control, &send_msg, portMAX_DELAY);
        xQueueSendToBack(to_Network, &send_msg, portMAX_DELAY);

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
