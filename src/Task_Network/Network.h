#ifndef NETWORK_H
#define NETWORK_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "mqtt/ipstack/IPStack.h"
#include "mqtt/tls/TlsClient.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <string>
#include "mqtt/ipstack/Countdown.h"
#include <cstdio>
#include "Structs.h"
#include <cstring>
#include <iostream>
#include <ostream>

#include "event_groups.h"
#include "mqtt/mqttService/MQTTService.h"

class Network {
public:
    Network(QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, QueueHandle_t scan_results_queue,
            EventGroupHandle_t event_group,TickType_t period,
            uint32_t stack_size = 2048, UBaseType_t priority = tskIDLE_PRIORITY + 1);
    static void task_wrap(void *pvParameters);

private:
    void task_impl();
    bool connect_internet(const char* ssid, const char* pwd,IPStack& ipstack, MQTTService& mqtt);
    int disconnect_internet(IPStack& ipstack, MQTTService& mqtt);
    int disconnect_wifi(IPStack &ip_stack);
    void check_and_reconnect(IPStack& ipstack, MQTTService& mqtt, const char* ssid, const char* pwd, EventGroupHandle_t event_group, TickType_t& next_check, TickType_t period);
    void mqtt_pub_tem_hum(MQTTService& mqtt, double tem, double hum,uint8_t alarm);
    void mqtt_pub_set_rh(MQTTService& mqtt, uint8_t set_rh);

    //for network scanning
    void do_scan();
    static int scan_result(void *env, const cyw43_ev_scan_result_t *result);
    Scan_result scan_results[MAX_SCAN_RESULTS];
    uint8_t result_count = 0;

    const char *name = "NETWORK";
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_Control;
    QueueHandle_t scan_results_queue;
    TickType_t period;
    bool wifi_connected = false;
    bool mqtt_connected = false;
    EventGroupHandle_t event_group;
};

#endif //NETWORK_H

