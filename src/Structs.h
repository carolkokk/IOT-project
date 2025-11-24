#ifndef STRUCTS_H
#define STRUCTS_H
#include <cstdint>

//simple examples for testing
enum MessageType{
    TEST_NUMBER,
    TEST_STRING
};

//combine message type and data
struct Message{
    MessageType type;

    uint8_t number;
    char string[64];
};

#endif //STRUCTS_H
