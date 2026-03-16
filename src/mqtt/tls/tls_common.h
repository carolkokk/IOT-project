//
// Created by carol on 22/01/2026.
//
#ifndef TLS_COMMON_H
#define TLS_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct TLS_CLIENT_T_ TLS_CLIENT_T;   // opaque handle

    TLS_CLIENT_T* tls_client_init(void);
    void tls_client_free(TLS_CLIENT_T *state);

    void tls_client_set_timeout_ms(TLS_CLIENT_T* state, int timeout_ms);

    bool tls_client_open_public(TLS_CLIENT_T *state,
                         const char *hostname,
                         const uint8_t *cert, size_t cert_len,
                         uint16_t port);

    err_t tls_client_write(TLS_CLIENT_T *state, const void *data, size_t len);
    err_t tls_client_close_public(TLS_CLIENT_T *state);

    bool tls_client_is_connected(const TLS_CLIENT_T *state);
    bool tls_client_is_complete(const TLS_CLIENT_T *state);
    int  tls_client_get_error(const TLS_CLIENT_T *state);

    //callback for receiving data from the server
    typedef void (*tls_recv_callback)(void *user, const uint8_t *data, size_t len);
    void tls_client_set_recv_callback(TLS_CLIENT_T *state, tls_recv_callback callback, void *user);

#ifdef __cplusplus
}
#endif

#endif // TLS_COMMON_H
