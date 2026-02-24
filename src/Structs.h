#ifndef STRUCTS_H
#define STRUCTS_H
#include <cstdint>
#include "configpass.h"

//initialization of Humidifier, GPIO 16 set as PWM, frequency 109 khz (LC resonance with piezo frequency), duty 50%.
#define HUMIDIFIER_PIN 16
#define HUMIDIFIER_FREQUENCY 109000
#define HUMIDIFIER_DUTY 0.5

//initialization of Dehumidifier, GPIO 17
#define DEHUMIDIFIER_PIN 17

//initialization of Fan, GPIO 2
#define FAN_PIN 2

//initialization of dehum_water_sensor GPIO 20
#define DEHUM_WATER_PIN 20

//initialization of hum_water_sensor GPIO 21
#define HUM_WATER_PIN 21

//event groups
#define EVT_NO_WATER        (1 << 0)
#define EVT_WATER_PRESENT  (1 << 1)
#define CONNECTING_NETWORK (1 << 2)
#define NETWORK_CONNECTED (1 << 3)
#define START_SCAN (1 << 4)
#define SCAN_DONE (1 << 5)

#define MAX_SCAN_RESULTS 20

//network scan result structure
struct Scan_result {
    char ssid[32];
    uint8_t bssid[6];
    uint32_t auth_mode;
};

//simple examples for testing
enum MessageType{
    TEMP_RH,
    TARGET_RH,
    NETWORK_CREDENTIALS,
};

struct Network_credentials {
    char ssid[32];
    char pass[32];
};

//combine message type and data
struct Message{
    MessageType type;
    //uint8_t number;
    //char string[64];
    double temp = 0.0;
    double rh = 0.0;
    uint8_t target_rh;
};

struct Scan_result_msg {
    Scan_result results[MAX_SCAN_RESULTS];
    uint8_t result_count;
};


//------------tls config--------------
#define TLS_CLIENT_SERVER        "api.thingspeak.com"
#define TLS_CLIENT_HTTP_REQUEST  "GET /talkbacks/55392/commands/57546109.json?api_key=WYYFXF0NGSZCUMW6 HTTP/1.1\r\n" \
"Host: " TLS_CLIENT_SERVER "\r\n" \
"Connection: close\r\n" \
"\r\n"
#define TLS_CLIENT_TIMEOUT_SECS  15
#define TLS_PORT 443


//MQTT configuration
#define MQTT_PUB_TOPIC "channels/3243447/publish"
#define MQTT_SUB_TOPIC "channels/3243447/subscribe/fields/field3"

//#define HOSTNAME "192.168.43.122"
#define HOSTNAME "mqtt3.thingspeak.com"
#define PORT 8883
#define MQTT_VERSION 4

#define MQTT_CLEAN_SESSION true
#define MQTT_KEEPALIVE 60
#define MSG_RETAINED false
#define MSG_DUP false


#endif //STRUCTS_H
