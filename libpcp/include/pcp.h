/*
 * Copyright (c) 2013 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#ifndef PCP_H
#define PCP_H

#ifdef WIN32
#include <winsock2.h>
#include <in6addr.h>
#include <ws2tcpip.h>
#include <time.h>
#include "stdint.h"
#else //WIN32
#include <sys/time.h>
#include <stdint.h>
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PCP_MAX_SUPPORTED_VERSION 2

#ifndef PCP_MIN_SUPPORTED_VERSION
#ifdef PCP_DISABLE_NATPMP_SUPPORT
#define PCP_MIN_SUPPORTED_VERSION 1
#else
#define PCP_MIN_SUPPORTED_VERSION 0
#endif
#endif

#ifndef PCP_SERVER_PORT
#define PCP_SERVER_PORT 5351
#endif

#ifndef PCP_MAX_PING_COUNT
#define PCP_MAX_PING_COUNT 5
#endif

#ifndef PCP_SERVER_DISCOVERY_RETRY_DELAY
#define PCP_SERVER_DISCOVERY_RETRY_DELAY 3600
#endif

#ifdef WIN32
#define PCP_SOCKET SOCKET
#else
#define PCP_SOCKET int
#endif

typedef struct pcp_flow* pcp_flow_t;
typedef struct pcp_userid_option *pcp_userid_option_p;
typedef struct pcp_deviceid_option *pcp_deviceid_option_p;
typedef struct pcp_location_option *pcp_location_option_p;

/* DEBUG levels */
typedef enum {
    PCP_DEBUG_NONE  = 0,
#define PCP_DEBUG_NONE  0
    PCP_DEBUG_ERR,
#define PCP_DEBUG_ERR   1
    PCP_DEBUG_WARN,
#define PCP_DEBUG_WARN  2
    PCP_DEBUG_INFO,
#define PCP_DEBUG_INFO  3
    PCP_DEBUG_PERR,
#define PCP_DEBUG_PERR  4
    PCP_DEBUG_DEBUG
#define PCP_DEBUG_DEBUG 5
} pcp_debug_mode_t;

typedef enum {
    PCP_ERR_SUCCESS = 0,
    PCP_ERR_MAX_SIZE = -1,
    PCP_ERR_OPT_ALREADY_PRESENT = -2,
    PCP_ERR_BAD_AFINET = -3,
    PCP_ERR_SEND_FAILED = -4,
    PCP_ERR_RECV_FAILED = -5,
    PCP_ERR_UNSUP_VERSION = -6,
    PCP_ERR_NO_MEM = -7,
    PCP_ERR_BAD_ARGS = -8,
    PCP_ERR_UNKNOWN = -9,
    PCP_ERR_SHORT_LIFETIME_ERR = -10,
    PCP_ERR_TIMEOUT = -11,
    PCP_ERR_NOT_FOUND = -12
} pcp_errno;

typedef enum {
    pcp_state_processing,
    pcp_state_succeeded,
    pcp_state_partial_result,
    pcp_state_short_lifetime_error,
    pcp_state_failed
} pcp_fstate_e;

typedef void (*external_logger)(pcp_debug_mode_t , const char*);

void pcp_set_loggerfn(external_logger ext_log);

// runtime level of logging
extern pcp_debug_mode_t pcp_log_level;

/*
 * Initialize library, optionally initiate auto-discovery of PCP servers
 */
#define ENABLE_AUTODISCOVERY  1
#define DISABLE_AUTODISCOVERY 0

void pcp_init(uint8_t autodiscovery);

//returns internal pcp server ID, -1 => error occurred
int pcp_add_server(struct sockaddr* pcp_server, uint8_t pcp_version);

/*
 * Close socket fds and clean up all settings, frees all library buffers
 *      close_flows - signal end of flows to PCP servers
 */
void pcp_terminate(int close_flows);

////////////////////////////////////////////////////////////////////////////////
//                          Flow API

/*
 * Creates new PCP message from parameters parsed to this function:
 *  src_addr    source IP/port
 *  dst_addr    destination IP/port - optional
 *  ext_addr    sugg. ext. IP/port  - optional
 *  protocol    protocol associated with flow
 *  lifetime    time in seconds how long should mapping last
 *
 *  return
 *  pcp_flow_t   value used in other functions to reference this flow.
 */
pcp_flow_t pcp_new_flow(
        struct sockaddr* src_addr,
        struct sockaddr* dst_addr,
        struct sockaddr* ext_addr,
        uint8_t protocol, uint32_t lifetime);

void pcp_flow_set_lifetime(pcp_flow_t f, uint32_t lifetime);

/*
 * Set 3rd party option to the existing message flow info
 */
void pcp_set_3rd_party_opt (pcp_flow_t f, struct sockaddr* thirdp_addr);

/*
 * Set flow priority option to the existing flow
 */
void pcp_flow_set_flowp(pcp_flow_t f, uint8_t dscp_up, uint8_t dscp_down);

/*
 * Append metadata option to the existing flow f
 * if exists md with given id then replace with new value
 * if value is NULL then remove metadata with this id
 */
void
pcp_flow_add_md (pcp_flow_t f, uint32_t md_id, void *value, size_t val_len);


int pcp_flow_set_userid(pcp_flow_t f, pcp_userid_option_p userid);
int pcp_flow_set_deviceid(pcp_flow_t f, pcp_deviceid_option_p dev);
int pcp_flow_set_location(pcp_flow_t f, pcp_location_option_p loc);


/*
 * Append filter option.
 */
void pcp_flow_set_filter_opt(pcp_flow_t f, struct sockaddr *filter_ip,
        uint8_t filter_prefix);

/*
 * Append prefer failure option.
 */
void pcp_flow_set_prefer_failure_opt (pcp_flow_t f);

// create new PCP message with SADSCP opcode. It's used to learn
// correct DSCP values to get desired flow treatment by router.
pcp_flow_t pcp_learn_dscp(uint8_t delay_tol, uint8_t loss_tol,
uint8_t jitter_tol, char* app_name);

/*
 * Remove flow from PCP server.
 */
void pcp_close_flow(pcp_flow_t f);

/*
 * Frees memory pointed to by f; Invalidates f.
 */
void pcp_delete_flow(pcp_flow_t f);

typedef struct pcp_flow_info {
    pcp_fstate_e     result;
    struct in6_addr  pcp_server_ip;
    struct in6_addr  ext_ip;
    uint16_t         ext_port;   //network byte order
    time_t           recv_lifetime_end;
    time_t           lifetime_renew_s;
    uint8_t          pcp_result_code;
    struct in6_addr  int_ip;
    uint16_t         int_port;   //network byte order
    struct in6_addr  dst_ip;
    uint16_t         dst_port;   //network byte order
    uint8_t          protocol;
    uint8_t          learned_dscp;
} pcp_flow_info_t;

// Allocates info_buf by malloc, has to be freed by client when longer needed.
pcp_flow_info_t *pcp_flow_get_info(pcp_flow_t f, pcp_flow_info_t **info_buf,
        size_t *info_count);

//callback function type - called when flow state has changed
typedef void (*pcp_flow_change_notify)
        (pcp_flow_t f, struct sockaddr* src_addr, struct sockaddr* ext_addr,
                pcp_fstate_e);

//set flow state change notify callback function
void pcp_set_flow_change_cb(pcp_flow_change_notify cb_fun);

// evaluate flow state
// params:
//   flow (in)    - handle of the flow
//   fstate (out) - state of the flow
//   return value - count of interfaces in exit_state (nonzero value means
//                  there is some result from PCP server)

int pcp_eval_flow_state(pcp_flow_t flow, pcp_fstate_e *fstate);

////////////////////////////////////////////////////////////////////////////////
//                      Event handling functions

/*****************************************************************************/
/* For use in select loop                                                    */

// pcp_handle_select - handle socket and timeout events. It's intended to be
// used in select loop.
// params:
//   fd_max (in)       - the biggest FD to check in read_fd_set
//   read_fd_set (in)  - fd_set with signaled FDs with data available to read.
//                       mostly to be used with a set filled by select.
//   select_timeout(in/out) - nearest time-out. if it is filles be nonzero
//                            value it will return smaller one of provided and
//                            inner calculated. to be used in select function

void pcp_handle_select(int fd_max, fd_set *read_fd_set,
        struct timeval *select_timeout);

// pcp_set_read_fdset - fills in fd_max and read_fd_set with FDs of internally
//                      used sockets.
void pcp_set_read_fdset(int *fd_max, fd_set *read_fd_set);

// example of use in select loop
/*
    fd_set read_fds;
    int fdmax = 0;
    struct timeval tout_select;
    memset(&tout_select, 0, sizeof(tout_select));

    FD_ZERO(&read_fds);

    // select loop
    for (;;) {
        int ret_count;
        void* data;
        pcp_fstate_e ret_state;

        //process all events and get timeout value for next select
        pcp_handle_select(fdmax, &read_fds, &tout_select);

        FD_ZERO(&read_fds);
        pcp_set_read_fdset(&fdmax, &read_fds);

        ret_count = select(fdmax, &read_fds, NULL, NULL, &tout_select);
    }
 */

/*****************************************************************************/
/* Blocking wait for flow reaching one of exit states or time-out(ms)
 * expiration.
 *   pcp_wait
 * params:
 *   flow    (in)             - pcp flow handle
 *   timeout (in)             - maximal time in ms to wait for result
 *   exit_on_partial_res (in) - do not wait for result of all interfaces or
 *                              possible PCP servers. Instead return immediately
 *                              after first received result.
                                                                             */
pcp_fstate_e pcp_wait(pcp_flow_t flow, int timeout, int exit_on_partial_res);

// example of pcp_wait use:
/*
    pcp_flow_t f = pcp_new_flow((struct sockaddr*)&src, (struct sockaddr*)&dst,
                                NULL, IPPROTO_TCP, 60);
    pcp_flow_set_flowp(f, 12, 16);
    pcp_wait(f, 500, 0);  // send PCP msg and wait for response for 500 ms
 */

/*****************************************************************************/
/* Functions to be used in apps with event handling (e.g. with libevent)     */

// type of callback function. function will be called when new socket is created
// or the already used one is closed.
// params:
//   fd (in)          - fd being added or removed
//   added (in)       - added / removed
//   cb_arg (in)      - user defined param - set during cb handler registering
//   fd_data (in/out) - user data stored to this FD

typedef void (*pcp_fd_change_cb_t)
        (PCP_SOCKET fd, int added, void *cb_arg, void ** fd_data);

// pcp_set_fd_change_cb - register callback function to be notified about FD
// creation and deletion.

int pcp_set_fd_change_cb(pcp_fd_change_cb_t cb, void* cb_arg);

// pcp_handle_fd_event - handle event on one FD - data / time-out
// params:
//   fd (in)            - FD on which event occurred
//   timed_out (in)     - 1 timed out, 0 data available to read
//   mext_timeout (out) - func return next value of time-out on FD

int pcp_handle_fd_event(PCP_SOCKET fd, int timed_out,
                        struct timeval *next_timeout);


/*****************************************************************************/
/* pcp_pulse - non-blocking function - checks all sockets for any incoming
 * messages and for time-outs and triggers corresponding action
 * params:
 *    struct timeval * next_pulse (out) - value of nearest timeout           */

#define pcp_pulse(next_pulse) pcp_handle_select(0, NULL, next_pulse)

//example of pcp_pulse use:
/*
  pcp_flow_t f = pcp_new_flow(...);
  do {
      pcp_pulse(NULL);
      //do something else
      sleep(1);
  } while (1);
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PCP_H */
