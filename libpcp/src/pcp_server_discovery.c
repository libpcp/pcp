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


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include "pcp_win_defines.h"
#else
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif //WIN32
#include "pcp.h"
#include "pcp_utils.h"
#include "pcp_server_discovery.h"
#include "pcp_event_handler.h"
#include "gateway.h"
#include "pcp_msg.h"
#include "pcp_logger.h"

pcp_errno psd_create_pcp_server_socket(int pcp_server_index)
{
    struct sockaddr_storage src;
    struct sockaddr_storage pcp_server_saddr;
    socklen_t src_len = sizeof(src);
    pcp_server_t* s = get_pcp_server(pcp_server_index);

    PCP_LOGGER_BEGIN(PCP_DEBUG_DEBUG);

    if (!s) {
        PCP_LOGGER_END(PCP_DEBUG_DEBUG);
        return PCP_ERR_BAD_ARGS;
    }
    memset(&pcp_server_saddr,0,sizeof(pcp_server_saddr));
    memset(&src,0,sizeof(src));

    pcp_server_saddr.ss_family = s->af;
    if (s->af == AF_INET) {
        ((struct sockaddr_in*)
        &pcp_server_saddr)->sin_addr.s_addr = s->pcp_ip[3];
        ((struct sockaddr_in*)
        &pcp_server_saddr)->sin_port = s->pcp_port;
    } else {
        IPV6_ADDR_COPY(&((struct sockaddr_in6*)&pcp_server_saddr)->sin6_addr,
                (struct in6_addr *)&s->pcp_ip);
        ((struct sockaddr_in6*)
        &pcp_server_saddr)->sin6_port = s->pcp_port;
    }

    inet_ntop(s->af, s->af==AF_INET ?
            (void*)&((struct sockaddr_in*) &pcp_server_saddr)->sin_addr :
            (void*)&((struct sockaddr_in6*) &pcp_server_saddr)->sin6_addr,
            s->pcp_server_paddr, sizeof(s->pcp_server_paddr));

    s->pcp_server_socket = socket(s->af, SOCK_DGRAM, 0);

    if (s->pcp_server_socket == PCP_INVALID_SOCKET) {         //LCOV_EXCL_START
        char buff[128];

        pcp_strerror(errno, buff, sizeof(buff));

        PCP_LOGGER(PCP_DEBUG_WARN,
                "Error (%s) occurred while connecting socket to "
                        "PCP server %s", buff, s->pcp_server_paddr);

        PCP_LOGGER_END(PCP_DEBUG_DEBUG);
        return PCP_ERR_UNKNOWN;
    }//LCOV_EXCL_STOP

    if (connect(s->pcp_server_socket,
        (struct sockaddr *) &pcp_server_saddr,
        SA_LEN((struct sockaddr *) &pcp_server_saddr)) == PCP_SOCKET_ERROR) {
        //LCOV_EXCL_START
        char buff[128];

        pcp_strerror(errno, buff, sizeof(buff));

        PCP_LOGGER(PCP_DEBUG_WARN,
                "Error (%s) occurred while connecting socket to "
                        "PCP server %s", buff, s->pcp_server_paddr);

        PCP_LOGGER_END(PCP_DEBUG_DEBUG);
        return PCP_ERR_UNKNOWN;
    }//LCOV_EXCL_STOP

    PCP_LOGGER(PCP_DEBUG_DEBUG, "Created socket %d to pcp server %s.",
            s->pcp_server_socket, s->pcp_server_paddr);

    if (getsockname(s->pcp_server_socket,
            (struct sockaddr *) &src, &src_len) != PCP_SOCKET_ERROR) {
        if (s->af == AF_INET) {
            s->src_ip[0] = 0;
            s->src_ip[1] = 0;
            s->src_ip[2] = htonl(0xFFFF);
            s->src_ip[3] = ((struct sockaddr_in*)&src)->sin_addr.s_addr;
        } else if (s->af == AF_INET6) {
            s->src_ip[0]=S6_ADDR32(&((struct sockaddr_in6*)&src)->sin6_addr)[0];
            s->src_ip[1]=S6_ADDR32(&((struct sockaddr_in6*)&src)->sin6_addr)[1];
            s->src_ip[2]=S6_ADDR32(&((struct sockaddr_in6*)&src)->sin6_addr)[2];
            s->src_ip[3]=S6_ADDR32(&((struct sockaddr_in6*)&src)->sin6_addr)[3];
        }
    }

    s->server_state = pss_ping;
    s->next_timeout.tv_sec = 0;
    s->next_timeout.tv_usec = 0;

    pcp_fd_change_notify(s, 1);

    PCP_LOGGER_END(PCP_DEBUG_DEBUG);
    return PCP_ERR_SUCCESS;
}

void psd_add_gws()
{
    struct in6_addr *gws = NULL, *gw;

    int rcount = getgateways(&gws);
    gw = gws;

    for (; rcount > 0; rcount--) {
        int af;
        int pcps_indx;

        af = IN6_IS_ADDR_V4MAPPED(gw) ? AF_INET: AF_INET6;

        if ((af==AF_INET) && (S6_ADDR32(gw)[3]==INADDR_ANY))
            continue;

        if ((af==AF_INET6) && (IN6_IS_ADDR_UNSPECIFIED(gw)))
            continue;

        if (get_pcp_server_by_ip(gw)) {
            continue;
        }
        pcps_indx = pcp_new_server(gw, ntohs(PCP_SERVER_PORT));
        if (pcps_indx >= 0) {
            pcp_server_t *s;
            psd_create_pcp_server_socket(pcps_indx);

            if (pcp_log_level>=PCP_DEBUG_INFO) {
                s = get_pcp_server(pcps_indx);
                PCP_LOGGER(PCP_DEBUG_INFO, "Found gateway %s. "
                        "Added as possible PCP server.",
                        s?s->pcp_server_paddr:"NULL pointer!!!");
            }
        }
        gw++;
    }
    free(gws);
}

pcp_errno psd_add_pcp_server(struct sockaddr* sa, uint8_t version)
{
    struct in6_addr pcp_ip = IN6ADDR_ANY_INIT;
    uint16_t pcp_port;
    pcp_server_t *pcps = NULL;

    PCP_LOGGER_BEGIN(PCP_DEBUG_DEBUG);

    if (sa->sa_family==AF_INET) {
        S6_ADDR32(&pcp_ip)[0] = 0;
        S6_ADDR32(&pcp_ip)[1] = 0;
        S6_ADDR32(&pcp_ip)[2] = htonl(0xFFFF);
        S6_ADDR32(&pcp_ip)[3] = ((struct sockaddr_in*)sa)->sin_addr.s_addr;
        pcp_port=((struct sockaddr_in*)sa)->sin_port;
    } else {
        IPV6_ADDR_COPY(&pcp_ip, &((struct sockaddr_in6*)sa)->sin6_addr);
        pcp_port=((struct sockaddr_in6*)sa)->sin6_port;
    }

    if (!pcp_port) {
        pcp_port=ntohs(PCP_SERVER_PORT);
    }

    pcps = get_pcp_server_by_ip((struct in6_addr*)&pcp_ip);
    if (!pcps) {
        int pcps_indx = pcp_new_server(&pcp_ip, pcp_port);
        if (pcps_indx >= 0) {
            pcps = get_pcp_server(pcps_indx);
        }

        if (pcps == NULL) {  //LCOV_EXCL_START
            PCP_LOGGER(PCP_DEBUG_ERR, "%s","Can't add PCP server.\n");
            PCP_LOGGER_END(PCP_DEBUG_DEBUG);
            return PCP_ERR_UNKNOWN;
        }  //LCOV_EXCL_STOP

    } else {
        pcps->pcp_port = pcp_port;

        if (pcps->pcp_server_socket != PCP_INVALID_SOCKET) {
            CLOSE(pcps->pcp_server_socket);
            pcps->pcp_server_socket = PCP_INVALID_SOCKET;
        }
    }
    pcps->pcp_version = version;
    pcps->server_state = pss_allocated;

    psd_create_pcp_server_socket(pcps->index);

    PCP_LOGGER(PCP_DEBUG_INFO, "Added PCP server %s",
            pcps->pcp_server_paddr);

    PCP_LOGGER_END(PCP_DEBUG_DEBUG);
    return pcps->index;
}
