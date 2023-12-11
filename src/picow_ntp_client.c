/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "date_utils.h"

#include "picow_ntp_client.h"
#include "debug.h"

// Status
#define ST_COMPLETE 0
#define ST_IN_PROGRESS 1
#define ST_ERROR 2

// Progress
// 1-7 Is the normal sequence.  The user should see an increasing count in this case.
#define P_ALLOCATE_STATE 1
#define P_INITIALIZE_WIFI 2
#define P_INIT_RECIEVE 3
#define P_WIFI_LOGIN 4
#define P_DNS_REQUESTED 5
#define P_DNS_RECEIVED 6
#define P_TIME_RECEIVED 7
#define P_UNABLE_TO_SET_RETRY_ALARM 8

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool enable_retry;
    struct udp_pcb *ntp_pcb;
    absolute_time_t time_to_start_retry;
    time_t *utc;
    int status;  //ST_*
    absolute_time_t ntp_send_time_us; // Time ntp packet was sent in us.
    void(*progress)(int p);
} NTP_T;

static NTP_T state;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_RETRY_TIME (30 * 1000)

// Define FORCE_ERRORS to force loss of DNS and NTP packtes to test retry.
// An error will be forced once of of FORCE_ERRORS times.
//#define FORCE_ERRORS 2
#define FE_TEST_CASE FE_NTP

#define FE_NTP 0
#define FE_DNS_REQUEST 1
#define FE_DNS_RECEIVE 2

#ifdef FORCE_ERRORS
int force_error_counter;

static char *force_error_name[] = {"NTP", "DNS_REQUEST", "DNS_RECEIVE"};

static bool is_force_error(int kind) {
    if (kind == FE_TEST_CASE) {
        bool err = ++force_error_counter%FORCE_ERRORS == 0;
        if (err) {
            printf("*********************************************\n");
            printf("Forcing error: %s\n", force_error_name[kind]);
            printf("current time: "); print_absolute_time(get_absolute_time());
            printf("retry time: "); print_absolute_time(state.time_to_start_retry);
            printf("enable_retry %d\n", state.enable_retry);
            printf("*********************************************\n");
            return err;
        }
    }
    return false;
}
#else
static bool is_force_error(int kind) {
    return false;
}
#endif

// Called with results of operation
static void ntp_result(int status, time_t *result) {
    state.status = status;
    if (status == ST_COMPLETE && result) {
        dprintf1("ntp_result ok: %lld\n", *result);
        state.progress(P_TIME_RECEIVED);
        state.time_to_start_retry = make_timeout_time_ms(0);
        *state.utc = *result;
        state.enable_retry = false;
    } else {
        dprintf1("ntp_result: setup for retry\n");
        state.time_to_start_retry = make_timeout_time_ms(NTP_RETRY_TIME);
    }
}

// Make an NTP request
static void ntp_request() {
    if (is_force_error(FE_NTP))
         return;

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    err_t err = udp_sendto(state.ntp_pcb, p, &state.ntp_server_address, NTP_PORT);
    if (err != ERR_OK)
        printf("ntp_request: udp sent err=%d\n", err);
    state.ntp_send_time_us = get_absolute_time();
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    if (!is_force_error(FE_DNS_RECEIVE) && ipaddr) {
        state.progress(P_DNS_RECEIVED);
        state.ntp_server_address = *ipaddr;
        dprintf1("ntp_dns_found address %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        dprintf1("ntp_dns_found: dns request timed out\n");
        ntp_result(ST_ERROR, NULL);
        state.progress(-P_DNS_RECEIVED);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state.ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {            
        dprintf1("ntp_recv: Time received\n");
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        int64_t round_trip_delay_us = absolute_time_diff_us(state.ntp_send_time_us, get_absolute_time());
        int32_t one_way_delay_seconds = round_trip_delay_us / 1000000L / 2;
        epoch += one_way_delay_seconds;
        dprintf1("one way trip delay: ");
#if DEBUG_LEVEL >= 1
         print_time_us(round_trip_delay_us/2);
#endif
        ntp_result(ST_COMPLETE, &epoch);
    } else {
        dprintf1("ntp_recv: invalid ntp response\n");
        ntp_result(ST_ERROR, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
static void ntp_init_recv() {
    state.ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state.ntp_pcb) {
        state.progress(-P_INIT_RECIEVE);
        return;
    }
    udp_recv(state.ntp_pcb, ntp_recv, NULL);
    state.progress(P_INIT_RECIEVE);
}

bool ntp_start(void(*progress)(int p)) {
    dprintf1("ntp_start\n");
    memset(&state, 0, sizeof(NTP_T));
    state.progress = progress;
    state.status = ST_IN_PROGRESS;
    progress(P_ALLOCATE_STATE);

    if (cyw43_arch_init()) {
        dprintf1("ntp_start: failed to initialize wifi\n");
        state.status = ST_ERROR;
        state.progress(-P_INITIALIZE_WIFI);
        return false;
    }
    state.progress(P_INITIALIZE_WIFI);
    
    ntp_init_recv();

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        dprintf1("ntp_start: failed to connect\n");
        state.progress(-P_WIFI_LOGIN);
        return false;
    }
    state.progress(P_WIFI_LOGIN);
    return true;
}

void ntp_end() {
    cyw43_arch_deinit();
};

/**
 * Ask NTP for time.
 * utc - Time structure passed by caller, to be filled in by this function.
 * progress - Callback to report progress.
 * return true if success. 
 */
bool ntp_ask_for_time(time_t* utc) {
    state.utc = utc;
    state.status = ST_IN_PROGRESS;
    state.time_to_start_retry = get_absolute_time();
    state.enable_retry = true;
    dprintf1("ntp_ask_for_time\n");
    
    while(state.status == ST_IN_PROGRESS) {
        dprintf3("ntp_start: enable retry=%d  time to wait %lld\n",  state.enable_retry, absolute_time_diff_us(get_absolute_time(), state.time_to_start_retry));
        if (absolute_time_diff_us(get_absolute_time(), state.time_to_start_retry) <= 0 && state.enable_retry) {
            dprintf1("ntp_start: Retry time reached.\n"); 
            state.time_to_start_retry = make_timeout_time_ms(NTP_RETRY_TIME);
            
            // cyw43_arch_l wip_begin/end should be used around calls into lwIP to ensure correct locking.
            // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
            // these calls are a no-op and can be omitted, but it is a good practice to use them in
            // case you switch the cyw43_arch type later.
            err_t err;
            if (is_force_error(FE_DNS_REQUEST)) {
                err = ERR_VAL;
            } else {
                cyw43_arch_lwip_begin();
                err = dns_gethostbyname(NTP_SERVER, &state.ntp_server_address, ntp_dns_found, NULL);
                cyw43_arch_lwip_end();
            }
            dprintf1("ntp_start: sent dns request err=%d\n", err);
            state.progress(P_DNS_REQUESTED);

            if (err == ERR_OK) {
                ntp_request(state); // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                dprintf1("dns request failed\n");
                ntp_result(ST_ERROR, NULL);
                state.progress(-P_DNS_RECEIVED);
                break;
            }
        }
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer interrupt) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // printf("- about to wait now= "); print_absolute_time(get_absolute_time());
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000L));
        // printf("- done waiting\n");
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    return state.status == ST_COMPLETE;
}

