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

#ifndef __GETGATEWAY_H__
#define __GETGATEWAY_H__

#ifdef WIN32
#if !defined(_MSC_VER)
#include <stdint.h>
#else
typedef unsigned __int32  uint32_t;
typedef unsigned __int16  uint16_t;
#endif
#define in_addr_t uint32_t
#include "pcp_win_defines.h"
#endif
#include "pcp.h"

struct sockaddr;
struct in6_addr;

/* This structure is used for route operations using routing sockets */

/* I know I could have used sockaddr. But type casting eveywhere is nasty and error prone.
   I do not believe a few bytes will make much impact and code will become much cleaner */
typedef struct route_op {
    struct sockaddr_in dst4;
    struct sockaddr_in mask4;
    struct sockaddr_in gw4;
    struct sockaddr_in6 dst6;
    struct sockaddr_in6 mask6;
    struct sockaddr_in6 gw6;
    char ifname[128];
    uint16_t family;
} route_op_t;

typedef route_op_t route_in_t;
typedef route_op_t route_out_t;

/* int get_src_gw_for_route_to(const struct sockaddr * dst,
                     void * src, size_t * src_len, void * gw, size_t *gw_len) :
 * return value :
 *    0 : success
 *   -1 : failure    */

//DLLSPEC int
extern int
get_src_gw_for_route_to(const struct sockaddr * dst,
        struct in6_addr *src, struct in6_addr *gw);

extern int getgateways(struct in6_addr ** gws);

int route_get(in_addr_t * dst, in_addr_t * mask, in_addr_t * gateway, char *ifname, route_in_t *routein, route_out_t *routeout);
int route_add(in_addr_t dst, in_addr_t mask, in_addr_t gateway, const char *ifname, route_in_t *routein, route_out_t *routeout);
int route_change(in_addr_t dst, in_addr_t mask, in_addr_t gateway, const char *ifname, route_in_t *routein, route_out_t *routeout);
int route_delete(in_addr_t dst, in_addr_t mask, route_in_t *routein, route_out_t *routeout);
int route_get_sa(struct sockaddr *dst, in_addr_t * mask, struct sockaddr * gateway, char *ifname, route_in_t *routein, route_out_t *routeout );

int
get_if_addr_from_name(char *ifname, struct sockaddr *ifsock, int family);

#endif
