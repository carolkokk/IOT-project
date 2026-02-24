//
// Created by carol on 22/01/2026.
//

#include "TlsClient.h"


TlsClient::TlsClient(): state(nullptr), tls_connected{false}
{
    //create stream buffer for received data
    stream = xStreamBufferCreate(4096, 1);
}

void TlsClient::tls_received(const uint8_t *data, size_t len){
    size_t off = 0;
    while (off < len) {
        size_t sent = xStreamBufferSend(stream, data + off, len - off, pdMS_TO_TICKS(10));
        if (sent == 0) {
            printf("STREAM FULL: dropped %u bytes\n", (unsigned)(len - off));
            break;
        }
        off += sent;
    }
};

void TlsClient::tls_recv_cb(void *user, const uint8_t *data, size_t len){
    auto *self = static_cast<TlsClient*>(user);
    self->tls_received(data, len);
}

int TlsClient::read_bytes(uint8_t* out, size_t maxlen, int timeout){
    size_t n = xStreamBufferReceive(
        stream,
        out,
        maxlen,
        pdMS_TO_TICKS(timeout)
    );
    if (n > 0)
    {
        return static_cast<int>(n);
    }
    return 0;
}

bool TlsClient::tls_connect(const char* hostname, const uint8_t *cert, size_t cert_len, int timeout, uint16_t port)
{

    state = tls_client_init();
    tls_client_set_timeout_ms(state, timeout);
    tls_client_set_recv_callback(state, tls_recv_cb, this);

    cyw43_arch_lwip_begin();
    bool ok = tls_client_open_public(state,hostname,cert,cert_len,port);
    cyw43_arch_lwip_end();

    if (!ok) {
        printf("open failed\n");
        return false;
    } else
    {
        while (!tls_client_is_connected(state) && !tls_client_is_complete(state)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (!tls_client_is_connected(state)) {
            printf("connect failed, err=%d\n", tls_client_get_error(state));
            return false;
        } else {
            printf("TLS connected\n");
            tls_connected = true;
        }
    }
    return true;
}

bool TlsClient::tls_check_connected()
{
    tls_connected = tls_client_is_connected(state);
    return tls_connected;
}

int TlsClient::tls_close()
{
    tls_connected = false;

    if (!state) return 0;

    cyw43_arch_lwip_begin();
    err_t err = tls_client_close_public(state);
    cyw43_arch_lwip_end();
    tls_state_free();

    if (err == ERR_OK){
        return 0;
    }
    return -1;
}

void TlsClient::tls_state_free()
{
    if (!state) return;
    tls_client_free(state);
    state = nullptr;
}

int TlsClient::write_bytes(const void *data, size_t len)
{
    if (tls_check_connected())
    {
        err_t err = tls_client_write(state, data, len);
        if (err == ERR_OK)
        {
            return (int)len;
        }
        return -1;
    }
    tls_connected = false;
    return -1;
}