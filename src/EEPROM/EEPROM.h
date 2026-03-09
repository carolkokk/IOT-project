#ifndef EEPROM_H
#define EEPROM_H

#include "pico/stdlib.h"
#include "PicoI2C.h"
#include "Structs.h"
#include <cstring>
#include <string>
#include <memory>
#include <vector>

#define EEPROM_ADDRESS 0x50
#define STATUS_BUFF_SIZE 8 // for status updates
#define STR_BUFFER_SIZE 64 //for log messages

 // 5 addresses saved for status updates like co2_set val or reboot detect
//addresses for specific status updates
#define REBOOT_ADDR 0x00
#define REBOOT_FLAG "REBOOT"
#define RUN_FLAG "RUN"

#define RH_SET_ADDR 0x08

#define LOG_COUNT 10 // max log messages until log entries are deleted

//address for saving wifi credentials
#define WIFI_SSID_ADDR 0x40
#define WIFI_PASS_ADDR 0x80

#define MIN_LOG_ADDR (WIFI_PASS_ADDR + STR_BUFFER_SIZE)
#define MAX_LOG_ADDRESS (MIN_LOG_ADDR + (LOG_COUNT - 1) * STR_BUFFER_SIZE)
#define LOG_ADDR_STORAGE (MAX_LOG_ADDRESS + STR_BUFFER_SIZE)

// saving sensor data for plotting
#define SAMPLE_SIZE 10 // 2*float + crc
#define SAMPLE_COUNT 50
#define SAMPLE_IDX_ADDR (LOG_ADDR_STORAGE + 2)
#define SAMPLE_DATA_ADDR (SAMPLE_IDX_ADDR + 2)
#define SAMPLE_END_ADDR (SAMPLE_DATA_ADDR + SAMPLE_COUNT * SAMPLE_SIZE)
#define SAMPLE_COUNT_ADDR (SAMPLE_END_ADDR)

class EEPROM {
public:
    EEPROM(std::shared_ptr<PicoI2C> i2cbus, uint8_t address = EEPROM_ADDRESS);

    // single status updates
    bool writeStatus(uint16_t address, const char *status, size_t max_len = STATUS_BUFF_SIZE);
    bool readStatus(uint16_t address, char *status_buffer, size_t buffer_len, size_t max_len = STATUS_BUFF_SIZE);
    bool readStatus(uint16_t address, std::string &status_buffer, size_t max_len = STATUS_BUFF_SIZE);

    // functions for logging
    bool writeLog(const char *message);
    std::vector<std::string> getAllLogs();
    void printAllLogs();
    void deleteLogs();
    bool isLogEmpty(uint16_t *next_addr);

    // functions for sample saving
    bool writeSample(float rh, float temp);
    bool readAllSamples(Measure_history *samples, uint8_t &count);
    void deleteSamples();

    // direct access functions
    bool eepromWrite(uint16_t address, const uint8_t *data, size_t data_len);
    bool eepromRead(uint16_t address, uint8_t *data, size_t data_len);

private:
    std::shared_ptr<PicoI2C> i2c;
    uint8_t addr;

    // private functions for crc check and log address update
    uint16_t crc16(const uint8_t *buffer_p, size_t buffer_len);
    bool validateCrc(const uint8_t *data_buffer, size_t message_len);
    uint16_t readLogAddress();
    bool writeLogAddress(uint16_t log_addr);
};

#endif // EEPROM_H