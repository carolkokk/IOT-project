#ifndef CONTROL_H
#define CONTROL_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "PicoI2C.h"
#include "EEPROM/EEPROM.h"
#include <memory>

#include "event_groups.h"
#include "Temp_Rh/BME680.h"

class Control {
public:
    Control(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, EventGroupHandle_t event_group, TickType_t period, uint32_t stack_size = 1024, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    const char *name = "CONTROL";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    TickType_t period;
    uint8_t set_rh;
    EventGroupHandle_t event_group;
};

#endif //CONTROL_H
