#include "Control.h"
#include <cstdio>
#include "Structs.h"
#include "PWM/PWM.h"
#include "EEPROM/EEPROM.h"
#include "Humidifier/Humidifier.h"
#include "Dehumidifier/Dehumidifier.h"
#include <cmath>
#include "Fan/Fan.h"
#include "event_groups.h"
#include "Temp_Rh/BME680_wrapper.h"
#include "WaterSensor/WaterSensor.h"

Control::Control(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, EventGroupHandle_t event_group, TickType_t period,
    std::shared_ptr<PicoI2C> i2cbus0, std::shared_ptr<EEPROM> eeprom,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),event_group(event_group),period(period),
    i2cbus0(std::move(i2cbus0)), eeprom(std::move(eeprom))
{

    xTaskCreate(task_wrap, name, stack_size, this, priority, &task_handle);
    //create timer for retrieving data from sensors
    timer_handle = xTimerCreate("Control_timer",pdMS_TO_TICKS(15000),pdTRUE,this,timer_callback);
    xTimerStart(timer_handle,0);
}

void Control::task_wrap(void *pvParameters) {
    auto *control = static_cast<Control*>(pvParameters);
    control->task_impl();
}

void Control::timer_callback(TimerHandle_t xTimer){
    auto *control = static_cast<Control*>(pvTimerGetTimerID(xTimer));
    if (control && control->task_handle){
        xTaskNotifyGive(control->task_handle);
    }
}

void Control::task_impl() {
    //initialization of Humidifier
    Humidifier humidifier(HUMIDIFIER_PIN,HUMIDIFIER_FREQUENCY,HUMIDIFIER_DUTY);

    //initialization of Dehumidifier
    Dehumidifier dehumidifier(DEHUMIDIFIER_PIN);
    Fan fan_hum(FAN_PIN);

    //temperature and humidity sensor
    BME680 rh_sensor(i2cbus0, 0x76);
    // only write every 6th value to eeprom
    uint8_t eeprom_val_write_counter = 0;

    // --- Water sensors ---
    WaterSensor dehum_water_sensor(DEHUM_WATER_PIN, true);
    WaterSensor humidifier_water_sensor(HUM_WATER_PIN, true);

    dehum_water_sensor.Init();
    humidifier_water_sensor.Init();

    //initial target rh
    eeprom->eepromRead(RH_SET_ADDR, &set_rh, sizeof(set_rh));
    uint8_t in_range_rh = set_rh;

    //test structure where Control sends a number to both UI and Network
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_numbers{};
    //send_numbers.type = TEST_NUMBER;
    //send_numbers.number = 0;
    Message received{};

    //testing temp sensor data communication to UI
    Message temp_rh{};
    temp_rh.type = TEMP_RH;

    //give it for the first time for the ui to retrieve sensor data before timer is triggered.
    xTaskNotifyGive(task_handle);

    while(true) {
        bool dehum_water_alarm  = !dehum_water_sensor.Read();
        bool humidifier_water_alarm     = humidifier_water_sensor.Read();
        EventBits_t bits = xEventGroupGetBits(event_group);
        double temp, rh;

        if (ulTaskNotifyTake(pdTRUE,0)) {
            if (rh_sensor.read_data(temp, rh)) {
                temp_rh.temp = std::round(temp * 100.0) / 100.0;
                temp_rh.rh = std::round(rh * 100.0) / 100.0;
                printf("timer triggered\n");
                double current_rh = temp_rh.rh;
                // for testing every measure value is saved, in real life probably would save evert 6th or 10th value
                //eeprom->writeSample(static_cast<float>(temp_rh.rh), static_cast<float>(temp_rh.temp));
                xQueueSendToBack(to_UI, &temp_rh, pdMS_TO_TICKS(100));
                if (bits & NETWORK_CONNECTED){
                    xQueueSendToBack(to_Network, &temp_rh, pdMS_TO_TICKS(100));
                }
                //if (!humidifier_water_alarm && !dehum_water_alarm) {
                if (current_rh < set_rh) {
                    dehumidifier.dehum_off();
                    humidifier.humidifier_on();
                    fan_hum.fan_on();
                    vTaskDelay(humidifier_on_interval);
                    humidifier.humidifier_off();
                    //fan_hum.fan_off();
                } else if (current_rh > set_rh + 5) {
                    humidifier.humidifier_off();
                    fan_hum.fan_off();
                    dehumidifier.dehum_on();
                } else {
                    dehumidifier.dehum_off();
                    humidifier.humidifier_off();
                    fan_hum.fan_off();
                }
                ++eeprom_val_write_counter;
                if (eeprom_val_write_counter >= 6) {
                    eeprom->writeSample(static_cast<float>(temp_rh.temp), static_cast<float>(temp_rh.rh));
                    eeprom_val_write_counter = 0;
                }
            }
        }
        //}

        // dehumidifier water alarm, triggers when water is detected
        if (dehum_water_alarm) {
            printf("WARNING: Too much water!.\r\n");
            xEventGroupSetBits(event_group, EVT_WATER_PRESENT);
        }else
        {
            xEventGroupClearBits(event_group, EVT_WATER_PRESENT);
        }

        // humidifier water alarm, triggers when water is not detected
        if (humidifier_water_alarm) {
            printf("WARNING: No water detected! Tank is empty\r\n");
            xEventGroupSetBits(event_group, EVT_NO_WATER);
        }else
        {
            xEventGroupClearBits(event_group, EVT_NO_WATER);
        }

        while (xQueueReceive(to_Control,&received,pdMS_TO_TICKS(10))) {
            if (received.type == TARGET_RH) {
                printf("New target rh: %d\n", static_cast<uint8_t>(received.target_rh));
                set_rh = static_cast<uint8_t>(received.target_rh);
                in_range_rh = set_rh;
                printf("set_rh in control task: %d\n", set_rh);
            }
        }

        vTaskDelayUntil(&lastWakeTime, period);
    }
}