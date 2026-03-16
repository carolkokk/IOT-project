//
// Created by carol on 22/01/2026.
//

#ifndef TLSCLIENT_H
#define TLSCLIENT_H

#include <mbedtls/debug.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tls_common.h"
#include "Structs.h"
#include "stream_buffer.h"

class TlsClient
{
public:
    TlsClient();

    void tls_received(const uint8_t *data, size_t len);
    static void tls_recv_cb(void *user, const uint8_t *data, size_t len);

    int read_bytes(uint8_t* out, size_t maxlen, int timeout);

    bool tls_connect(const char* hostname, const uint8_t *cert, size_t cert_len, int timeout,uint16_t port);
    bool tls_check_connected();
    int tls_close();
    void tls_state_free();
    int write_bytes(const void *data, size_t len);

private:
    TLS_CLIENT_T* state;
    bool tls_connected;
    StreamBufferHandle_t stream;
};

#endif //TLSCLIENT_H
