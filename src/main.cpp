#include "FreeRTOS.h"
#include <string>
#include <iostream>
#include "task.h"
#include "queue.h"
#include "Structs.h"
#include "timers.h"
#include "EEPROM/EEPROM.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "Task_Control/Control.h"
#include "Task_Network/Network.h"
#include "Task_UI/UI.h"
#include "event_groups.h"

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

int main(){
    QueueHandle_t to_control;
    QueueHandle_t to_UI;
    QueueHandle_t to_network;
    QueueHandle_t scan_results_queue;
    QueueHandle_t credentials_to_network;

    timer_hw->dbgpause = 1;
    stdio_init_all();

    EventGroupHandle_t event_group = xEventGroupCreate();

    to_control = xQueueCreate(10, sizeof(Message));
    to_UI = xQueueCreate(10, sizeof(Message));
    to_network = xQueueCreate(10, sizeof(Message));
    scan_results_queue = xQueueCreate(1, sizeof(Scan_result_msg));
    credentials_to_network = xQueueCreate(1, sizeof(Network_credentials));

    const TickType_t control_period = pdMS_TO_TICKS(4000);
    const TickType_t UI_period = pdMS_TO_TICKS(10);
    const TickType_t network_period = pdMS_TO_TICKS(100);

    // both control and UI tasks use eeprom
    auto i2cbus0 = std::make_shared<PicoI2C>(0, 100000);
    auto i2cbus1 = std::make_shared<PicoI2C>(1, 100000);
    auto eeprom = std::make_shared<EEPROM>(i2cbus1);

    Control control_task(to_UI, to_network, to_control, event_group, control_period, i2cbus0, eeprom);
    UI ui_task(to_UI, to_network, to_control, scan_results_queue, credentials_to_network, event_group, UI_period, eeprom);
    Network network_task(to_UI, to_network, to_control, scan_results_queue, credentials_to_network, event_group, network_period);

    vTaskStartScheduler();
    return 0;
}
