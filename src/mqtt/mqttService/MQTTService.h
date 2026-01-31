#ifndef MQTTSERVICE_H
#define MQTTSERVICE_H

#include "../ipstack/IPStack.h"
#include "MQTTClient.h"
#include <string>
#include "../ipstack/Countdown.h"

class MQTTService {
public:
    MQTTService(IPStack& ipstack,
                const char* host,
                int port,
                const char* clientId,
                const char* subTopic,
                const char* pubTopic);

    bool connect_and_subscribe();
    void loop(int ms = 10);
    bool isConnected();
    void disconnect();
    int publish(const char* payload);
    bool get_Message(std::string& out);

private:
    void onMessage(MQTT::MessageData& md);

    static void messageArrived(MQTT::MessageData& md);
    static MQTTService* instance;

    IPStack& ipstack;
    MQTT::Client<IPStack, Countdown> mqtt;

    const char* hostname;
    int port;
    const char* clientId;
    const char* pub_topic;
    const char* sub_topic;

    bool hasNewMessage_ = false;
    std::string savedPayload_;
};

#endif //MQTTSERVICE_H

