#ifndef NETWORK_H
#define NETWORK_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "../mqtt/ipstack/IPStack.h"
#include "../mqtt/tls/TlsClient.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <string>
#include "../mqtt/ipstack/Countdown.h"
#include <cstdio>
#include "Structs.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include "../mqtt/mqttService/MQTTService.h"

class Network {
public:
    Network(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, TickType_t period, uint32_t stack_size = 2048, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    bool connect_wifi(const char* ssid, const char* pwd,IPStack& ipstack);
    int disconnect_wifi(IPStack &ip_stack);
    void mqtt_pub(MQTTService& mqtt, double tem, double hum);
    const char *name = "NETWORK";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    TickType_t period;
    bool wifi_connected = false;
    bool mqtt_connected = false;
};

#endif //NETWORK_H

