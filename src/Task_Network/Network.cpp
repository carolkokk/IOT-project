#include "Network.h"
#include <cstdio>
#include "Structs.h"
#include <cstring>

Network::Network(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void Network::task_wrap(void *pvParameters) {
    auto *network = static_cast<Network*>(pvParameters);
    network->task_impl();
}

void Network::task_impl() {
    //test structure where Network sends a message to both UI and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_msg{};
    Message received{};
    send_msg.type = TEST_STRING;
    strncpy(send_msg.string, "Test string from Network task.", sizeof(send_msg.string)-1);
    send_msg.string[sizeof(send_msg.string)-1] = '\0';

    while(true) {
        xQueueSendToBack(to_Control, &send_msg, portMAX_DELAY);
        xQueueSendToBack(to_UI, &send_msg, portMAX_DELAY);

        while (xQueueReceive(to_Network,&received,pdMS_TO_TICKS(10))) {
            if (received.type == TEST_STRING){
                printf("received %s\n",received.string);
            }else if (received.type == TEST_NUMBER){
                printf("received %u\n",received.number);
            }

        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
