/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"

#include "FreeRTOS.h"
#include "task.h"
#include "tls_common.h"

typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    struct altcp_tls_config *tls_cfg;
    bool complete;
    int error;
    int timeout;
    bool connected;
    uint16_t port;
    tls_recv_callback recv_callback;
    void *user;
} TLS_CLIENT_T;

static err_t tls_client_close(void *arg) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    err_t err = ERR_OK;

    state->complete = true;
    state->connected = false;
    if (state->pcb != NULL) {
        altcp_arg(state->pcb, NULL);
        altcp_poll(state->pcb, NULL, 0);
        altcp_recv(state->pcb, NULL);
        altcp_err(state->pcb, NULL);
        err = altcp_close(state->pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            altcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    return err;
}

static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        state->connected = false;
        state->error = err;
        return tls_client_close(state);
    }

    state->connected = true;

    /*printf("connected to server, sending request\n");
    err = altcp_write(state->pcb, state->http_request, strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("error writing data, err=%d", err);
        return tls_client_close(state);
    }*/

    return ERR_OK;
}

static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    printf("timed out\n");
    state->error = PICO_ERROR_TIMEOUT;
    return tls_client_close(arg);
}

static void tls_client_err(void *arg, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    printf("tls_client_err %d\n", err);
    tls_client_close(state);
    state->error = PICO_ERROR_GENERIC;
}

static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (!p) {
        state->connected = false;
        printf("connection closed\n");
        return tls_client_close(state);
    }

    if (p->tot_len > 0) {
        /* For simplicity this examples creates a buffer on stack the size of the data pending here, 
           and copies all the data to it in one go.
           Do be aware that the amount of data can potentially be a bit large (TLS record size can be 16 KB),
           so you may want to use a smaller fixed size buffer and copy the data to it using a loop, if memory is a concern */
        //char buf[p->tot_len + 1];
        //char *buf= (char *) malloc(p->tot_len + 1);
        uint8_t *buf = (uint8_t*) malloc(p->tot_len + 1);

        pbuf_copy_partial(p, buf, p->tot_len, 0);
        //buf[p->tot_len] = 0;

        //printf("***\nnew data received from server:\n***\n\n%s\n", buf);

        //use callback instead of printing
        if (state->recv_callback) {
            state->recv_callback(state->user, buf, p->tot_len);
        }

        free(buf);

        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

//receive callback function
void tls_client_set_recv_callback(TLS_CLIENT_T *state, tls_recv_callback callback, void *user){
    if (!state){
        return;
    }
    state->recv_callback = callback;
    state->user = user;
}

static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state)
{
    err_t err;
    //u16_t port = 443;
    //u16_t port = 21883; // Joe's secure MQTT
    //u16_t port = 8883; // secure MQTT

    printf("connecting to server IP %s port %d\n", ipaddr_ntoa(ipaddr), state->port);
    err = altcp_connect(state->pcb, ipaddr, state->port, tls_client_connected);
    if (err != ERR_OK)
    {
        fprintf(stderr, "error initiating connect, err=%d\n", err);
        tls_client_close(state);
    }
}

static void tls_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
    if (ipaddr)
    {
        printf("DNS resolving complete\n");
        tls_client_connect_to_server_ip(ipaddr, (TLS_CLIENT_T *) arg);
    }
    else
    {
        printf("error resolving hostname %s\n", hostname);
        tls_client_close(arg);
    }
}


static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    ip_addr_t server_ip;
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;

    state->pcb = altcp_tls_new(state->tls_cfg, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    altcp_arg(state->pcb, state);
    //for http:
    //altcp_poll(state->pcb, tls_client_poll, state->timeout * 2);

    //for mqtt:
    altcp_poll(state->pcb, NULL, 0);

    altcp_recv(state->pcb, tls_client_recv);
    altcp_err(state->pcb, tls_client_err);

    /* Set SNI */
    mbedtls_ssl_set_hostname(altcp_tls_context(state->pcb), hostname);

    printf("resolving %s\n", hostname);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();

    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);
    if (err == ERR_OK)
    {
        /* host is in DNS cache */
        tls_client_connect_to_server_ip(&server_ip, state);
    }
    else if (err != ERR_INPROGRESS)
    {
        printf("error initiating DNS resolving, err=%d\n", err);
        tls_client_close(state->pcb);
    }

    cyw43_arch_lwip_end();

    return err == ERR_OK || err == ERR_INPROGRESS;
}

// Perform initialisation
TLS_CLIENT_T* tls_client_init(void) {
    TLS_CLIENT_T *state = calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}
static void tlsdebug(void *ctx, int level, const char *file, int line, const char *message){
    fputs(message, stdout);
}

void tls_client_free(TLS_CLIENT_T *state) {
    if (!state) {
        printf("failed to find state\n");
        return ;
    }
    tls_client_close(state);
    if (state->tls_cfg) {
        altcp_tls_free_config(state->tls_cfg);
        state->tls_cfg = NULL;
    }
    free(state);
}

void tls_client_set_timeout_ms(TLS_CLIENT_T* state, int timeout) {
    if (state) state->timeout = timeout;
}

bool tls_client_open_public(TLS_CLIENT_T *state,
                     const char *hostname,
                     const uint8_t *cert, size_t cert_len,
                     uint16_t port) {

    //make sure the old tcp & tls connection is cleared
    if (!state || !hostname || !cert || cert_len == 0) return false;

    tls_client_close(state);
    if (state->tls_cfg) {
        altcp_tls_free_config(state->tls_cfg);
        state->tls_cfg = NULL;
    }

    //create tls_cfg
    state->complete = false;
    state->connected = false;
    state->error = 0;
    state->port = port;

    state->tls_cfg = altcp_tls_create_config_client(cert, cert_len);
    //state->tls_cfg = altcp_tls_create_config_client(NULL, 0);
    if (!state->tls_cfg) {
        state->error = ERR_MEM;
        return false;
    }

    //select authmode
    mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)state->tls_cfg, MBEDTLS_SSL_VERIFY_REQUIRED);

    //mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)state->tls_cfg, MBEDTLS_SSL_VERIFY_NONE);
    //start connection
    return tls_client_open(hostname,state);
}

//sending request to server (HTTP, MQTT, etc.)
err_t tls_client_write(TLS_CLIENT_T *state, const void *data, size_t len) {
    err_t err;
    if (!state || !state->pcb || !state->connected) return ERR_CONN;
    if (!data || len == 0) return ERR_OK;
    printf("connected to server, sending request\n");
    err = altcp_write(state->pcb, data, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("error writing data, err=%d", err);
        return tls_client_close(state);
    }
    return err;
}

err_t tls_client_close_public(TLS_CLIENT_T *s) {
    return tls_client_close(s);
}

bool tls_client_is_connected(const TLS_CLIENT_T *state)
{
    return state && state->connected;
}

bool tls_client_is_complete(const TLS_CLIENT_T *state)
{
    return state && state->complete;
}

int  tls_client_get_error(const TLS_CLIENT_T *state)
{
    return state ? state->error : PICO_ERROR_INVALID_ARG;
}