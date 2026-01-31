#include "MQTTService.h"
#include <cstdio>
#include <cstring>

MQTTService* MQTTService::instance = nullptr;

MQTTService::MQTTService(IPStack& ip, const char* host, int p, const char* id, const char* sub_topic, const char* pub_topic)
    : ipstack(ip),
    mqtt(ipstack),
    hostname(host),
    port(p),
    clientId(id),
    sub_topic(sub_topic),
    pub_topic(pub_topic){}

void MQTTService::messageArrived(MQTT::MessageData& md) {
    if (instance) instance->onMessage(md);
}

void MQTTService::onMessage(MQTT::MessageData& md) {
    MQTT::Message& msg = md.message;
    savedPayload_.assign((char*)msg.payload, (size_t)msg.payloadlen);
    hasNewMessage_ = true;
}

bool MQTTService::connect_and_subscribe() {
    instance = this;

    if (ipstack.connect(hostname, port) != 0) {
        printf("TCP connect failed\n");
        return false;
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION;
    data.clientID.cstring = (char*)clientId;
    data.cleansession = MQTT_CLEAN_SESSION;
    data.keepAliveInterval = MQTT_KEEPALIVE;
    data.username.cstring = (char *) MQTT_USERNAME;
    data.password.cstring = (char *) MQTT_PASSWORD;

    if (mqtt.connect(data) != 0) {
        printf("MQTT connect failed\n");
        return false;
    }

    if (mqtt.subscribe(sub_topic, MQTT::QOS0, messageArrived) != 0) {
        printf("MQTT subscribe failed\n");
        return false;
    }

    printf("MQTT connected & subscribed\n");
    return true;
}

void MQTTService::loop(int ms) {
    mqtt.yield(ms);
}

bool MQTTService::isConnected() {
    return mqtt.isConnected();
}

void MQTTService::disconnect() {
    mqtt.disconnect();
}

int MQTTService::publish(const char* payload) {
    MQTT::Message msg{};
    msg.qos = MQTT::QOS0;
    msg.retained = MSG_RETAINED;
    msg.dup = MSG_DUP;
    msg.payload = (void*)payload;
    msg.payloadlen = strlen(payload);
    return mqtt.publish(pub_topic, msg);
}

bool MQTTService::get_Message(std::string& out) {
    if (!hasNewMessage_) return false;
    out = savedPayload_;
    hasNewMessage_ = false;
    return true;
}
