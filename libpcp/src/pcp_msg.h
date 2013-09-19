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


#ifndef PCP_MSG_H_
#define PCP_MSG_H_

#ifdef WIN32
//#include <winsock2.h>
#include "stdint.h"
#else //WIN32
#include <sys/socket.h>
#include <stdint.h>
#endif


#include "pcp.h"
#include "pcp_utils.h"
#include "pcp_client_db.h"
#include "pcp_msg_structs.h"

void* build_pcp_msg(struct pcp_flow_s* flow);

typedef struct pcp_recv_msg {
    opt_flags_e         opt_flags;
    struct flow_key_data kd;
    uint32_t key_bucket;

    //response data
    struct in6_addr     assigned_ext_ip;
    in_port_t           assigned_ext_port;
    uint8_t             recv_dscp;
    uint32_t            recv_version;
    uint32_t            recv_epoch;
    uint32_t            recv_lifetime;
    uint32_t            recv_result;
    time_t              received_time;

    //control data
    uint32_t            pcp_server_indx;
    struct sockaddr_storage rcvd_from_addr;
    //msg buffer
    uint32_t            pcp_msg_len;
    char                pcp_msg_buffer[PCP_MAX_LEN];
} pcp_recv_msg_t;

int validate_pcp_msg(pcp_recv_msg_t* f);

void parse_response_hdr(pcp_recv_msg_t f);

pcp_errno parse_response(pcp_recv_msg_t* f);




#endif /* PCP_MSG_H_ */
