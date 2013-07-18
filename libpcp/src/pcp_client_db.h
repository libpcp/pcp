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
 */


#ifndef PCP_CLIENT_H_
#define PCP_CLIENT_H_

#include <stdint.h>
#include "pcp.h"
#include "pcp_event_handler.h"
#include "pcp_msg_structs.h"
#include "unp.h"
#ifdef WIN32
#include "pcp_win_defines.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    optf_3rd_party = 1 << 0,
    optf_flowp     = 1 << 1,
} opt_flags_e;

#ifndef MD_VAL_MAX_LEN
#define MD_VAL_MAX_LEN 24
#endif

#define PCP_INV_SERVER (~0u)

typedef struct {
    uint16_t md_id;
    uint16_t val_len;
    uint8_t  val_buf[MD_VAL_MAX_LEN];
} md_val_t;

struct pcp_flow {
    // flow's data
    opt_flags_e         opt_flags;
    struct flow_key_data {
        uint8_t             operation;
        struct in6_addr     src_ip;
        struct in6_addr     pcp_server_ip;
        struct pcp_nonce    nonce;
        union {
            struct mp_keydata {
                uint8_t             protocol;
                in_port_t           src_port;
                struct in6_addr     dst_ip;
                in_port_t           dst_port;
            } map_peer;
        };
    } kd;
    uint32_t key_bucket;

    time_t lifetime;
    union {
        struct {
            struct in6_addr     ext_ip;
            in_port_t           ext_port;
        } map_peer;
        struct {
            uint8_t toler_fields;
            uint8_t app_name_length;
            uint8_t learned_dscp;
        } sadscp;
    };
    char*   sadscp_app_name;

    //response data
    time_t              recv_lifetime;
    uint32_t            recv_result;

    //control data
    struct pcp_flow    *next; //next flow with same key bucket
    struct pcp_flow    *next_child; //next flow for MAP with 0.0.0.0 src ip
    uint32_t            pcp_server_indx;
    pcp_flow_state_e    state;
    uint32_t            resend_timeout;
    uint32_t            retry_count;
    uint32_t            to_send_count;
    struct timeval      timeout;

    //Userid
    pcp_userid_option_t f_userid;

    //Location
    pcp_location_option_t f_location;

    //DeviceID
    pcp_deviceid_option_t f_deviceid;


    //FLOW Priority Option
    uint8_t             flowp_option_present;
    uint8_t             flowp_dscp_up;
    uint8_t             flowp_dscp_down;

    //PREFER FAILURE Option
    uint8_t             pfailure_option_present;

    //FILTER Option
    uint8_t             filter_option_present;
    uint8_t             filter_prefix;
    uint16_t            filter_port;
    struct in6_addr     filter_ip;

    //MD Option
    uint32_t             md_val_count;
    md_val_t           *md_vals;

    //msg buffer
    uint32_t            pcp_msg_len;
    char               *pcp_msg_buffer;
};


/* structure holding PCP server specific data */
struct pcp_server {
    uint32_t                    af;
    uint32_t                    pcp_ip[4];
    uint16_t                    pcp_port;
    uint32_t                    src_ip[4];
    char                        pcp_server_paddr[INET6_ADDRSTRLEN];
    PCP_SOCKET                  pcp_server_socket;
    uint8_t                     pcp_version;
    uint8_t                     next_version;
    pcp_server_state_e          server_state;
    uint32_t                    epoch;
    time_t                      cepoch;
    struct pcp_nonce            nonce;
    uint32_t                    index;
    pcp_flow_t                  ping_flow_msg;
    pcp_flow_t                  restart_flow_msg;
    uint32_t                    ping_count;
    struct timeval              next_timeout;
    uint32_t                    natpmp_ext_addr;
    void*                       app_data;
};

typedef int(*pcp_db_flow_iterate)(pcp_flow_t f, void* data);

typedef int(*pcp_db_server_iterate)(pcp_server_t *f, void* data);

pcp_flow_t pcp_create_flow(pcp_server_t* s, struct flow_key_data *fkd);

pcp_errno pcp_free_flow(pcp_flow_t f);

pcp_flow_t pcp_get_flow(struct flow_key_data *fkd, uint32_t pcp_server_indx);

pcp_errno pcp_db_add_flow(pcp_flow_t f);

pcp_errno pcp_db_rem_flow(pcp_flow_t f);

pcp_errno pcp_db_foreach_flow(pcp_db_flow_iterate f, void* data);

void pcp_flow_clear_msg_buf(pcp_flow_t f);

void pcp_db_add_md(pcp_flow_t f, uint16_t md_id, void* val, size_t val_len);

int pcp_new_server(struct in6_addr *ip, uint16_t port);

pcp_errno pcp_db_foreach_server(pcp_db_server_iterate f, void* data);

pcp_server_t * get_pcp_server(int pcp_server_index);

pcp_server_t *
get_pcp_server_by_ip(struct in6_addr *ip);

pcp_server_t * get_pcp_server_by_fd(PCP_SOCKET fd);

void pcp_db_free_pcp_servers(void);

pcp_errno pcp_delete_flow_intern(pcp_flow_t f);


#ifdef __cplusplus
}
#endif

#endif /* PCP_CLIENT_H_ */
