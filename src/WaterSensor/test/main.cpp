//
// Created by An Qi on 27.1.2026
// WaterSensor standalone test (Event Bit version)
//

#include <cstdio>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "../WaterSensor.h"

// Event bit definitions
#define EVT_NO_WATER        (1 << 0)
#define EVT_WATER_PRESENT  (1 << 1)

// Global event group handle
static EventGroupHandle_t water_event_group;

// --------------------------------------------------
// Water sensor test task
// --------------------------------------------------
void water_sensor_task(void *param) {

    // Initialize stdio (USB serial)
    stdio_init_all();
    vTaskDelay(pdMS_TO_TICKS(2000));   // Wait for serial monitor

    printf("=== WaterSensor standalone test (Event Bit) ===\n");

    // GPIO28: no-water alarm sensor (active low)
    WaterSensor no_water_sensor(28, true);

    // GPIO27: water-present sensor (active low)
    WaterSensor water_sensor(27, true);

    no_water_sensor.Init();
    water_sensor.Init();

    bool last_no_water_alarm = false;
    bool last_water_alarm    = false;

    while (true) {

        // Read physical sensor states
        bool no_water_detected = no_water_sensor.Read();
        bool water_detected    = water_sensor.Read();

        // Convert to alarm semantics
        bool no_water_alarm = !no_water_detected;
        bool water_alarm    = water_detected;

        // --- Update event bits ---

        if (no_water_alarm != last_no_water_alarm) {
            last_no_water_alarm = no_water_alarm;

            if (no_water_alarm) {
                xEventGroupSetBits(water_event_group, EVT_NO_WATER);
            } else {
                xEventGroupClearBits(water_event_group, EVT_NO_WATER);
            }
        }

        if (water_alarm != last_water_alarm) {
            last_water_alarm = water_alarm;

            if (water_alarm) {
                xEventGroupSetBits(water_event_group, EVT_WATER_PRESENT);
            } else {
                xEventGroupClearBits(water_event_group, EVT_WATER_PRESENT);
            }
        }

        // --- Read back event bits (for test output) ---
        EventBits_t bits = xEventGroupGetBits(water_event_group);

        printf("[EVENT BITS] NO_WATER=%d, WATER_PRESENT=%d\n",
               (bits & EVT_NO_WATER) ? 1 : 0,
               (bits & EVT_WATER_PRESENT) ? 1 : 0);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// --------------------------------------------------
// main
// --------------------------------------------------
int main() {

    // Create event group
    water_event_group = xEventGroupCreate();
    configASSERT(water_event_group != nullptr);

    // Create test task
    xTaskCreate(
        water_sensor_task,
        "WaterSensorTest",
        1024,
        nullptr,
        tskIDLE_PRIORITY + 1,
        nullptr
    );

    // Start scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (true) {
    }
}
