#include "Network.h"

Network::Network(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control),period(period){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void Network::task_wrap(void *pvParameters) {
    auto *network = static_cast<Network*>(pvParameters);
    network->task_impl();
}

void Network::task_impl() {
    //test structure where Network sends a message to both UI and control
    TickType_t lastWakeTime = xTaskGetTickCount();
    Message send_msg{};
    Message received{};
    //send_msg.type = TEST_STRING;
    //strncpy(send_msg.string, "Test string from Network task.", sizeof(send_msg.string)-1);
    //send_msg.string[sizeof(send_msg.string)-1] = '\0';
    char ssid[] = WIFI_SSID;
    char pwd[] = WIFI_PASSWORD;
    //fake tem and hum data for testing
    double tem = 20.00;
    double hum = 45.00;

    const char *pub_topic = MQTT_PUB_TOPIC;
    const char *sub_topic = MQTT_SUB_TOPIC;
    const uint8_t cert_thingspeak[] = TLS_THINGSPEAK_SERVER;
    const uint8_t cert_mqtt[] = TLS_MQTT_BROKER;
    std::string payload;
    TlsClient tls_client;
    IPStack ipstack(tls_client,cert_thingspeak,TLS_CLIENT_TIMEOUT_SECS);
    MQTTService mqtt(ipstack,HOSTNAME,PORT,MQTT_CLIENT_ID,sub_topic,pub_topic);

    if (!connect_wifi(ssid,pwd,ipstack)){
        printf("WIFI connection failed\n");
    }

    printf("Subscribing topic: %s\n", sub_topic);
    if (!mqtt.connect_and_subscribe()){
        printf("MQTT connection failed\n");
    }else{
        mqtt_connected = true;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    //publish a message to the MQTT broker for verification for MQTT connection
    //mqtt_pub(mqtt,tem,hum);

    //check connection once in 15s
    const TickType_t period = pdMS_TO_TICKS(15000);
    TickType_t next_check = xTaskGetTickCount() + period;

    while(true) {
        //xQueueSendToBack(to_Control, &send_msg, pdMS_TO_TICKS(10));
        //xQueueSendToBack(to_UI, &send_msg, pdMS_TO_TICKS(10));

        while (xQueueReceive(to_Network,&received,pdMS_TO_TICKS(10))) {
            if (received.type == TEMP_RH){
                tem = received.temp;
                hum = received.rh;
                printf("received %.2f\n",received.temp);
                printf("received %.2f\n",received.rh);
                mqtt_pub(mqtt,tem,hum);
            }else if (received.type == TEST_NUMBER){
                printf("received %u\n",received.number);
            }
        }

        //check if MQTT is connected in every 5s.
        if ((int32_t)(xTaskGetTickCount()-next_check) >= 0) {
            if (ipstack.WiFi_connected()){
                next_check += period;
                if (!mqtt.isConnected()) {
                    printf("Not connected... reconnecting\n");
                    mqtt_connected = false;

                    mqtt.disconnect();
                    ipstack.disconnect();
                    if (mqtt.connect_and_subscribe())
                    {
                        mqtt_connected = true;
                    };
                }
            }else{
                printf("wifi is not connected. Reconnecting\n");
                disconnect_wifi(ipstack);
                connect_wifi(ssid,pwd,ipstack);
            }
        }

        //get message from the subscribed field3 for set humidity level
        if (mqtt.get_Message(payload)) {
            auto set_hum = static_cast<uint8_t> (std::stoi(payload));
            send_msg.type = TARGET_RH;
            send_msg.target_rh = set_hum;
            printf("converted%u\n",set_hum);
            xQueueSendToBack(to_Control, &send_msg, pdMS_TO_TICKS(10));
        }

        //yield. socket that client uses calls cyw43_arch_poll()
        mqtt.loop(10);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool Network::connect_wifi(const char* ssid, const char* pwd, IPStack& ipstack)
{
    if (!ipstack.connect_WiFi(ssid, pwd, 5)) {
        printf("WiFi connect failed.\n");
        return false;
    }
    wifi_connected = true;
    return true;
}

int Network::disconnect_wifi(IPStack &ip_stack){
    ip_stack.disconnect_WiFi();
    int rc = ip_stack.disconnect();
    return rc;
}

void Network::mqtt_pub(MQTTService& mqtt, double tem, double hum)
{
    //publish a message to the MQTT broker for verification fo MQTT connection
    char msg[256];
    snprintf(msg,sizeof(msg), R"(field1=%f&field2=%f&status=MQTTPUBLISH)",tem,hum);
    mqtt.publish(msg);
}