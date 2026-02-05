#ifndef STRUCTS_H
#define STRUCTS_H
#include <cstdint>

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
};

//combine message type and data
struct Message{
    MessageType type;

    uint8_t number;
    char string[64];
    double temp;
    double rh;
};


#endif //STRUCTS_H
