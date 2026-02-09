#ifndef STRUCTS_H
#define STRUCTS_H
#include <cstdint>
//#include "configpass.h"

//initialization of Humidifier, GPIO 16 set as PWM, frequency 109 khz (LC resonance with piezo frequency), duty 50%.
#define HUMIDIFIER_PIN 16
#define HUMIDIFIER_FREQUENCY 109000
#define HUMIDIFIER_DUTY 0.5

//initialization of Dehumidifier, GPIO 17
#define DEHUMIDIFIER_PIN 17


//simple examples for testing
enum MessageType{
    TEST_NUMBER,
    TEST_STRING,
    TEMP_RH,
    TARGET_RH,
};

//combine message type and data
struct Message{
    MessageType type;

    uint8_t number;
    char string[64];
    double temp = 0.0;
    double rh = 0.0;
    uint8_t target_rh;
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
