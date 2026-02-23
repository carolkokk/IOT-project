#include "Control.h"
#include <cstdio>
#include "Structs.h"
#include "PWM/PWM.h"
#include "Humidifier/Humidifier.h"
#include "Dehumidifier/Dehumidifier.h"
#include <cmath>
#include "Fan/Fan.h"
#include "event_groups.h"
#include "Temp_Rh/BME680_wrapper.h"
#include "WaterSensor/WaterSensor.h"

Control::Control(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, EventGroupHandle_t event_group, TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),event_group(event_group),period(period){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr); 
}

void Control::task_wrap(void *pvParameters) {
    auto *control = static_cast<Control*>(pvParameters);
    control->task_impl();
}

void Control::task_impl() {
    //initialization of Humidifier
    Humidifier humidifier(HUMIDIFIER_PIN,HUMIDIFIER_FREQUENCY,HUMIDIFIER_DUTY);

    //initialization of Dehumidifier
    Dehumidifier dehumidifier(DEHUMIDIFIER_PIN);
    Fan fan_hum(FAN_PIN);

    //temperature and humidity sensor
    auto i2cbus0 = std::make_shared<PicoI2C>(0, 100000);
    BME680 rh_sensor(i2cbus0, 0x76);

    // --- Water sensors ---
    WaterSensor dehum_water_sensor(DEHUM_WATER_PIN, true);
    WaterSensor humidifier_water_sensor(HUM_WATER_PIN, true);

    dehum_water_sensor.Init();
    humidifier_water_sensor.Init();

    //initial target rh
    set_rh = 50;

    //test structure where Control sends a number to both UI and Network
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_numbers{};
    //send_numbers.type = TEST_NUMBER;
    //send_numbers.number = 0;
    Message received{};

    //testing temp sensor data communication to UI
    Message temp_rh{};
    temp_rh.type = TEMP_RH;

    while(true) {
        //xQueueSendToBack(to_UI, &send_numbers, portMAX_DELAY);
        //xQueueSendToBack(to_Network, &send_numbers, portMAX_DELAY);

        bool dehum_water_alarm  = !dehum_water_sensor.Read();
        bool humidifier_water_alarm     = humidifier_water_sensor.Read();
        EventBits_t bits = xEventGroupGetBits(event_group);

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
            /*if (received.type == TEST_STRING){
                printf("received %s\n",received.string);
            }else if (received.type == TEST_NUMBER)
            {
                printf("received %u\n",received.number);
            }*/
            if (received.type == TARGET_RH) {
                printf("New target rh: %d\n", static_cast<uint8_t>(received.target_rh));
                set_rh = static_cast<uint8_t>(received.target_rh);
                printf("set_rh in control task: %d\n", set_rh);
            }
        }

        //printf("T: %.2f C\n", rh_sensor.read_temp());
        //printf("RH: %.2f %%\n", rh_sensor.read_rh());
        temp_rh.temp = std::round(rh_sensor.read_temp() * 100.0) / 100.0;
        temp_rh.rh = std::round(rh_sensor.read_rh() * 100.0) / 100.0;
        xQueueSendToBack(to_UI, &temp_rh, pdMS_TO_TICKS(100));
        if (bits & NETWORK_CONNECTED){
            xQueueSendToBack(to_Network, &temp_rh, pdMS_TO_TICKS(100));
        }


        if (!humidifier_water_alarm && !dehum_water_alarm){
            // hum or dehum is on outside of the set_rh +-5% range
            uint8_t uin_rh = static_cast<uint8_t>(temp_rh.rh);
            if (uin_rh >= set_rh -5 && uin_rh <= set_rh +5) {
                dehumidifier.dehum_off();
                humidifier.humidifier_off();
                fan_hum.fan_off();
            }
            else if (uin_rh < (set_rh - 5)) {
                printf("set_rh in control task: %d\n", set_rh -5);
                printf("current rh in control task: %d\n", static_cast<uint8_t>(temp_rh.rh));
                dehumidifier.dehum_off();
                humidifier.humidifier_on();
                fan_hum.fan_on();
                printf("Humidifier on\n");
                printf("Fan on\n");
            } else if (uin_rh > set_rh + 5) {
                humidifier.humidifier_off();
                fan_hum.fan_off();
                printf("Humidifier off \n");
                printf("Fan off \n");
                dehumidifier.dehum_on();
                printf("Dehumidifier on\n");
            }
        }else{
            //turn off all the devices if alarm is triggered
            dehumidifier.dehum_off();
            humidifier.humidifier_off();
            fan_hum.fan_off();
        }

        vTaskDelayUntil(&lastWakeTime, period);
    }
}
