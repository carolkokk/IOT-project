#include "UI.h"
#include <cstdio>
#include "Structs.h"
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

#include "ili9341nobuf.h"
#include "PicoSPIBus.h"
#include "PicoSPIDevice.h"
#include "rgb_palette.h"
#include "fonts/FreeMono12pt7b.h"

UI::UI(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::task_wrap(void *pvParameters) {
    auto *ui = static_cast<UI*>(pvParameters);
    ui->task_impl();
}

void UI::task_impl() {

    // init of the display
    auto spi = std::make_shared<PicoSPIBus>(0, 6, 7, 8);
    auto dev = std::make_shared<PicoSPIDevice>(spi, 9);
    ili9341nobuf display(dev, 10, 11);

    display.fill(0x0000);
    display.show();
    display.setfont(&FreeMono12pt7b);

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

                std::ostringstream temp_stream, rh_stream;
                temp_stream << std::fixed << std::setprecision(2) << received.temp;
                rh_stream << std::fixed << std::setprecision(2) << received.rh;

                std::string temp = temp_stream.str();
                std::string rh = rh_stream.str();

                display.fill(0x0000);
                // show temp
                display.text("T: ", 70, 100, 0xFFFF);
                display.text(temp, 120, 100, 0x001F);
                // show humidity
                display.text("RH: ", 70, 130, 0xFFFF);
                display.text(rh, 120, 130, 0x001F);
                display.show();
            }
        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
