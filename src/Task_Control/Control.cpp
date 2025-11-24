#include "Control.h"
#include <cstdio>
#include "Structs.h"

Control::Control(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void Control::task_wrap(void *pvParameters) {
    auto *control = static_cast<Control*>(pvParameters);
    control->task_impl();
}

void Control::task_impl() {
    //test structure where Control sends a number to both UI and Network
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_numbers{};
    send_numbers.type = TEST_NUMBER;
    send_numbers.number = 0;
    Message received{};

    auto i2cbus0 = std::make_shared<PicoI2C>(0, 100000);
    BME680 temp_rh(i2cbus0, 0x76);

    while(true) {
        xQueueSendToBack(to_UI, &send_numbers, portMAX_DELAY);
        xQueueSendToBack(to_Network, &send_numbers, portMAX_DELAY);

        while (xQueueReceive(to_Control,&received,pdMS_TO_TICKS(10))) {
            if (received.type == TEST_STRING){
                printf("received %s\n",received.string);
            }else if (received.type == TEST_NUMBER)
            {
                printf("received %u\n",received.number);
            }
            printf("T: %.2f C\n", temp_rh.read_temp());
            printf("RH: %.2f %%\n", temp_rh.read_rh());
        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
