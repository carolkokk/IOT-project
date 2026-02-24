#include "Network.h"
#include "Fmutex.h"

Network::Network(
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_Control, QueueHandle_t scan_results_queue, QueueHandle_t credentials_to_network,
    EventGroupHandle_t event_group,TickType_t period,
    uint32_t stack_size,
    UBaseType_t priority) :
    to_UI(to_UI), to_Network(to_Network) ,to_Control (to_Control), scan_results_queue(scan_results_queue),credentials_to_network(credentials_to_network),
    event_group(event_group),period(period){
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
    char ssid[32];
    char pwd[32];
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


    vTaskDelay(pdMS_TO_TICKS(100));

    //check connection once in 15s
    const TickType_t period = pdMS_TO_TICKS(15000);
    TickType_t next_check = xTaskGetTickCount() + period;

    // this is for network scanning
    bool scan_in_progress = false;

    while(true) {
        EventBits_t bits = xEventGroupGetBits(event_group);

        // check if scan start bit is set and scan networks
        if (bits & START_SCAN) {
            xEventGroupClearBits(event_group, START_SCAN);
            do_scan();

            bits = xEventGroupGetBits(event_group);
        }

        //send scan results (wifi ssids) to UI
        if (bits & SCAN_DONE) {
            xEventGroupClearBits(event_group, SCAN_DONE);
            Scan_result_msg msg{};
            msg.result_count = result_count;
            memcpy(msg.results, scan_results, sizeof(Scan_result) * result_count);
            xQueueSendToBack(scan_results_queue, &msg, pdMS_TO_TICKS(10));
        }

        //check if UI wants to connect to the internet with pwd.
        if (bits & CONNECTING_NETWORK){
            Network_credentials creds{};
            //first clear possible remaining connections and clear network connected bit
            xEventGroupClearBits(event_group, NETWORK_CONNECTED);
            //disconnect_internet(ipstack,mqtt);
            vTaskDelay(pdMS_TO_TICKS(100));

            while (xQueueReceive(credentials_to_network,&creds,pdMS_TO_TICKS(100))){
                    printf("Received network credentials.\n");
                    //copy ssid and pwd from ui
                    strncpy(ssid, creds.ssid, sizeof(ssid) - 1);
                    ssid[sizeof(ssid) - 1] = '\0';
                    strncpy(pwd, creds.pass, sizeof(pwd) - 1);
                    pwd[sizeof(pwd) - 1] = '\0';
                    printf("received ssid:%s password:%s\n",ssid,pwd);

                    if (connect_internet(ssid,pwd,ipstack,mqtt)){
                        xEventGroupSetBits(event_group, NETWORK_CONNECTED);
                        next_check = xTaskGetTickCount() + period;
                    }
            }
            xEventGroupClearBits(event_group, CONNECTING_NETWORK);
        }

        if (bits & NETWORK_CONNECTED){
            while (xQueueReceive(to_Network,&received,pdMS_TO_TICKS(10))) {
                if (received.type == TEMP_RH){
                    tem = received.temp;
                    hum = received.rh;
                    printf("received %.2f\n",received.temp);
                    printf("received %.2f\n",received.rh);
                    //alarm is 1 if either or both of the alarm is on. If none of the alarm is on, then alarm is 0.
                    uint8_t alarm = !!(bits & (EVT_NO_WATER | EVT_WATER_PRESENT));
                    printf("alarm %u\n",alarm);
                    mqtt_pub_tem_hum(mqtt,tem,hum,alarm);
                }
                else if (received.type == TARGET_RH){
                    printf("target rh received %u\n",received.target_rh);
                    mqtt_pub_set_rh(mqtt,received.target_rh);
                }
            }

            //check if MQTT is connected in every 15s.
            check_and_reconnect(ipstack,mqtt,ssid,pwd,event_group,next_check,period);

            //get message from the subscribed field3 for set humidity level
            if (mqtt.get_Message(payload)) {
                auto set_hum = static_cast<uint8_t> (std::stoi(payload));
                send_msg.type = TARGET_RH;
                send_msg.target_rh = set_hum;
                printf("converted%u\n",set_hum);
                xQueueSendToBack(to_Control, &send_msg, pdMS_TO_TICKS(10));
                xQueueSendToBack(to_UI, &send_msg, pdMS_TO_TICKS(10));
            }


            //yield. socket that client uses calls cyw43_arch_poll()
            mqtt.loop(10);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool Network::connect_internet(const char* ssid, const char* pwd, IPStack& ipstack,MQTTService& mqtt){
    //disconnect possible remaining connections
    ipstack.disconnect();
    disconnect_internet(ipstack,mqtt);
    wifi_connected = false;
    mqtt_connected = false;
    if (!ipstack.connect_WiFi(ssid, pwd, 5)) {
        printf("WiFi connect failed.\n");
        return false;
    }
    if (!mqtt.connect_and_subscribe()){
        printf("MQTT connection failed\n");
        ipstack.disconnect();
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
    mqtt_connected = true;
    wifi_connected = true;
    return true;
}

int Network::disconnect_internet(IPStack& ipstack, MQTTService& mqtt){
    mqtt.disconnect();
    int rc = disconnect_wifi(ipstack);
    mqtt_connected = false;
    wifi_connected = false;
    return rc;
}

int Network::disconnect_wifi(IPStack &ip_stack){
    ip_stack.disconnect_WiFi();
    int rc = ip_stack.disconnect();
    return rc;
}

void Network::check_and_reconnect(IPStack& ipstack, MQTTService& mqtt, const char* ssid, const char* pwd, EventGroupHandle_t event_group, TickType_t& next_check, TickType_t period){
    if ((int32_t)(xTaskGetTickCount() - next_check) >= 0) {
        next_check += period;
        printf("Checking internet connection\n");

        if (!ipstack.WiFi_connected() || !mqtt.isConnected()) {
            xEventGroupClearBits(event_group, NETWORK_CONNECTED);
            xEventGroupSetBits(event_group, CONNECTING_NETWORK);
            //vTaskDelay(pdMS_TO_TICKS(200));
            printf("Not connected... reconnecting\n");
            //disconnect_internet(ipstack, mqtt);
            //vTaskDelay(pdMS_TO_TICKS(100));

            if (connect_internet(ssid, pwd, ipstack, mqtt)) {
                printf("reconnection successful\n");
                xEventGroupSetBits(event_group, NETWORK_CONNECTED);
                xEventGroupClearBits(event_group, CONNECTING_NETWORK);
            }
        }
    }
}

void Network::mqtt_pub_tem_hum(MQTTService& mqtt, double tem, double hum,uint8_t alarm)
{
    //publish a message to the MQTT broker for verification fo MQTT connection
    char msg[256];
    snprintf(msg,sizeof(msg), R"(field1=%f&field2=%f&field4=%u&status=MQTTPUBLISH)",tem,hum,alarm);
    mqtt.publish(msg);
}

void Network::mqtt_pub_set_rh(MQTTService& mqtt, uint8_t set_rh)
{
    //publish a message to the MQTT broker for verification fo MQTT connection
    char msg[256];
    snprintf(msg,sizeof(msg), R"(field3=%u&status=MQTTPUBLISH)",set_rh);
    mqtt.publish(msg);
}

void Network::do_scan() {
    result_count = 0;

    cyw43_wifi_scan_options_t scan_options = {0};
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, this, scan_result);
    if (err != 0) {
        printf("Failed to start scan (%d)\n", err);
        return;
    }

    printf("Scanning networks...\n");
    //polling until scan is done
    while (cyw43_wifi_scan_active(&cyw43_state)) {
        cyw43_arch_poll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("Scanning complete. %u networks found\n", result_count);

    // sending event bit that scan is done and data can be sent to queue
    xEventGroupSetBits(event_group, SCAN_DONE);
}

int Network::scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (!result) return 0;
    auto *netw_scan = static_cast<Network*>(env);

    //filter out duplicate networks
    for (const auto &existing : netw_scan->scan_results) {
        for (int i = 0; i < netw_scan->result_count; i++) {
            if (memcmp(netw_scan->scan_results[i].bssid, result->bssid, 6) == 0) {
                return 0;
            }
        }
    }

    //array full
    if (netw_scan->result_count >= MAX_SCAN_RESULTS) return 0;
    
    Scan_result &res = netw_scan->scan_results[netw_scan->result_count++];
    strncpy(res.ssid, reinterpret_cast<const char *>(result->ssid), result->ssid_len);
    res.ssid[result->ssid_len] = '\0';
    res.auth_mode = result->auth_mode;
    memcpy(res.bssid, result->bssid, 6);

    return 0;
}