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

#ifdef WIN32
#include <winsock2.h>
#endif
#include <stdio.h>
#include <string.h>
#include "unp.h"
#include "pcp_utils.h"
#include "pcp_socket.h"

#ifdef WIN32
// function calling WSAStartup (used in pcp-server and pcp_app)
int pcp_win_sock_startup() {
    int err;
    WORD wVersionRequested;
    WSADATA wsaData;
    OSVERSIONINFOEX osvi;
    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        perror("WSAStartup failed with error");
        return 1;
    }
    //find windows version
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if(!GetVersionEx((LPOSVERSIONINFO)(&osvi))){
        printf("pcp_app: GetVersionEx failed");
        return 1;
    }

    return 0;
}

/* function calling WSACleanup
*  returns 0 on success and 1 on failure
*/
int pcp_win_sock_cleanup() {
    if (WSACleanup() == PCP_SOCKET_ERROR) {
        printf("WSACleanup failed.\n");
        return 1;
    }
    return 0;
}
#endif

void pcp_fill_in6_addr(struct in6_addr *dst_ip6, uint16_t *dst_port,
        struct sockaddr* src)
{
    if (src->sa_family == AF_INET) {
        struct sockaddr_in* src_ip4 = (struct sockaddr_in*) src;
        if (dst_ip6) {
            if (src_ip4->sin_addr.s_addr != INADDR_ANY) {
                S6_ADDR32(dst_ip6)[0] = 0;
                S6_ADDR32(dst_ip6)[1] = 0;
                S6_ADDR32(dst_ip6)[2] = htonl(0xFFFF);
                S6_ADDR32(dst_ip6)[3] = src_ip4->sin_addr.s_addr;
            } else {
                unsigned i;
                for (i=0; i<4; ++i)
                    S6_ADDR32(dst_ip6)[i]=0;
            }
        }
        if (dst_port) {
            *dst_port = src_ip4->sin_port;
        }
    } else if (src->sa_family == AF_INET6) {
        struct sockaddr_in6* src_ip6 = (struct sockaddr_in6*) src;
        if (dst_ip6) {
            memcpy(dst_ip6,
                    src_ip6->sin6_addr.s6_addr,
                    sizeof(*dst_ip6));
        }
        if (dst_port) {
            *dst_port = src_ip6->sin6_port;
        }
    }
}

void
pcp_fill_sockaddr(struct sockaddr* dst, struct in6_addr* sip, in_port_t sport)
{
    if (IN6_IS_ADDR_V4MAPPED(sip)) {
        struct sockaddr_in *s = (struct sockaddr_in *)dst;
        s->sin_family = AF_INET;
        s->sin_addr.s_addr = S6_ADDR32(sip)[3];
        s->sin_port = sport;
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)dst;
        s->sin6_family = AF_INET6;
        s->sin6_addr = *sip;
        s->sin6_port = sport;
    }
}

