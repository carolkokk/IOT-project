#ifndef UI_H
#define UI_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"


class UI {
public:
    UI(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period, uint32_t stack_size = 1024, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    const char *name = "UI";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    TickType_t period;
};

#endif //UI_H
