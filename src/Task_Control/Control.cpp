#include "Control.h"
#include <cstdio>
#include "Structs.h"
#include "PWM/PWM.h"
#include "Humidifier/Humidifier.h"
#include "Dehumidifier/Dehumidifier.h"

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
    //initialization of Humidifier
    Humidifier humidifier(HUMIDIFIER_PIN,HUMIDIFIER_FREQUENCY,HUMIDIFIER_DUTY);

    //initialization of Dehumidifier
    Dehumidifier dehumidifier(DEHUMIDIFIER_PIN);

    //temperature and humidity sensor
    auto i2cbus0 = std::make_shared<PicoI2C>(0, 100000);
    BME680 rh_sensor(i2cbus0, 0x76);

    int count = 0;

    //test structure where Control sends a number to both UI and Network
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_numbers{};
    send_numbers.type = TEST_NUMBER;
    send_numbers.number = 0;
    Message received{};

    //testing temp sensor data communication to UI
    Message temp_rh{};
    temp_rh.type = TEMP_RH;

    while(true) {
        //xQueueSendToBack(to_UI, &send_numbers, portMAX_DELAY);
        xQueueSendToBack(to_Network, &send_numbers, portMAX_DELAY);

        while (xQueueReceive(to_Control,&received,pdMS_TO_TICKS(10))) {
            if (received.type == TEST_STRING){
                printf("received %s\n",received.string);
            }else if (received.type == TEST_NUMBER)
            {
                printf("received %u\n",received.number);
            }
        }

        //printf("T: %.2f C\n", rh_sensor.read_temp());
        //printf("RH: %.2f %%\n", rh_sensor.read_rh());
        temp_rh.temp = rh_sensor.read_temp();
        temp_rh.rh = rh_sensor.read_rh();
        xQueueSendToBack(to_UI, &temp_rh, portMAX_DELAY);


        //now the humidifier turns on for 5s for 15 times, later on can be used with H&T temperature.
        if (count <= 15){
            humidifier.humidifier_on();
            printf("Humidifier on for 5s\n");
            //turn on the humidifier for 5s just for testing
            vTaskDelay(pdMS_TO_TICKS(5000));
            humidifier.humidifier_off();
            printf("Humidifier off \n");
            //turn on the dehumidifier for 5s just for testing
            dehumidifier.dehum_on();
            printf("Dehumidifier on for 5s\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            dehumidifier.dehum_off();
            printf("Dehumidifier off \n");
            count++;
        }

        vTaskDelayUntil(&lastWakeTime, period);
    }
}

