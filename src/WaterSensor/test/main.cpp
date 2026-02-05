//
// Created by An Qi on 27.1.2026.
//
#include <cstdio>
#include "pico/stdlib.h"
#include "../WaterSensor.h"

int main() {
    // Initialize stdio (USB serial)
    stdio_init_all();
    sleep_ms(2000);   // Give time for serial monitor to connect

    printf("=== WaterSensor standalone test ===\n");

    // GPIO28: no-water alarm sensor
    // active_low = true -> LOW means water detected
    WaterSensor no_water_sensor(28, true);

    // GPIO27: water-present alarm sensor
    WaterSensor water_sensor(27, true);

    // Initialize GPIOs
    no_water_sensor.Init();
    water_sensor.Init();

    // Cache last alarm states to avoid repeated prints
    bool last_no_water_alarm = false;
    bool last_water_alarm    = false;

    while (true) {
        // Read physical sensor states
        bool no_water_detected = no_water_sensor.Read(); // true -> water present
        bool water_detected    = water_sensor.Read();    // true -> water present

        // Convert to alarm semantics
        bool no_water_alarm = !no_water_detected;  // no water -> alarm
        bool water_alarm    = water_detected;      // water present -> alarm

        // Print only on state change
        if (no_water_alarm != last_no_water_alarm) {
            last_no_water_alarm = no_water_alarm;
            printf("[NO WATER ALARM] %s\n",
                   no_water_alarm ? "ON" : "OFF");
        }

        if (water_alarm != last_water_alarm) {
            last_water_alarm = water_alarm;
            printf("[WATER PRESENT ALARM] %s\n",
                   water_alarm ? "ON" : "OFF");
        }

        sleep_ms(500);
    }
}
