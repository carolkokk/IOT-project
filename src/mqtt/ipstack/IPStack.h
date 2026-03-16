//
// Created by Keijo Länsikunnas on 12.2.2024.
//

#ifndef UART_IRQ_IPSTACK_H
#define UART_IRQ_IPSTACK_H

#include <cstdint>
#include <memory>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "../tls/TlsClient.h"


class IPStack {
public:
    IPStack(TlsClient& tls, const uint8_t *cert, int timeout,EventGroupHandle_t event_group);
    bool init();
    bool connect_WiFi(const char* ssid, const char* password, int max_retries);
    bool WiFi_connected();
    void disconnect_WiFi();

    //functions for paho-mqtt
    int connect(const char *hostname, int port);
    int read(unsigned char *buffer, int len, int timeout);
    int write(unsigned char *buffer, int len, int timeout);
    int disconnect();
    // lwip callback functions
    //static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
    //static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) ;
    //static void tcp_client_err(void *arg, err_t err);
    //static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    //static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

    //static const int BUF_SIZE{2048};
    //static const int POLL_TIME_S{5};
private:
    bool wifi_connected;
    TlsClient& tls_client;
    const uint8_t *cert;
    int timeout;
    EventGroupHandle_t event_group;
    //const void *send_request;
    /*struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    uint16_t count;
    uint32_t dropped;
    uint16_t wr; // write index
    uint16_t rd; // read index
    bool connected;*/
};


#endif //UART_IRQ_IPSTACK_H
