#ifndef CONTROL_H
#define CONTROL_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

class Control {
public:
    Control(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period, uint32_t stack_size = 1024, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    const char *name = "CONTROL";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    TickType_t period;
};

#endif //CONTROL_H
