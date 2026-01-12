#include "UI.h"
#include <cstdio>
#include "Structs.h"
#include <string>
#include <cstring>

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
            }
        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
