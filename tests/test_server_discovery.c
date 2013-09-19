/*
 *------------------------------------------------------------------
 * test_server_discovery.c
 *
 * Feb 4, 2013, ptatrai
 *
 * Copyright (c) 2013-2013 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "pcp_socket.h"
#include "pcp.h"
#include "pcp_server_discovery.h"
#include "gateway.h"
#include "pcp_logger.h"
#include "unp.h"
#include "test_macro.h"

int print_status(pcp_server_t* s, void* data)
{
    pcp_logger(PCP_DEBUG_INFO, "Server index %d, FD %d, addr %s, status: %d",
        s->index, s->pcp_server_socket, s->pcp_server_paddr, s->server_state);
    return 0;
}

int main(int argc, char* argv[])
{
    pcp_flow_t* f;
    pcp_fstate_e ret;
    pcp_server_t *s;
    int sindx, sindx1, sindx2;
    struct sockaddr_in6 src6;
    struct sockaddr_in src4, dst4;
    struct sockaddr *src = NULL;
    struct sockaddr *dst = NULL;

    PD_SOCKET_STARTUP();

    pcp_log_level = PCP_DEBUG_INFO;
    psd_create_pcp_server_socket(0);

    pcp_init(ENABLE_AUTODISCOVERY);
    pcp_init(ENABLE_AUTODISCOVERY);
    pcp_init(DISABLE_AUTODISCOVERY);

    TEST((sindx1=pcp_add_server(Sock_pton("100.2.1.1:5351"), 1))>=0);
    TEST((sindx=pcp_add_server(Sock_pton("100.2.1.1:5351"),PCP_MAX_SUPPORTED_VERSION)) == sindx1);
    TEST(pcp_add_server(Sock_pton("100.2.1.1"),PCP_MAX_SUPPORTED_VERSION+1)<0);
    TEST((sindx2=pcp_add_server(Sock_pton("[::1]:5350"),PCP_MAX_SUPPORTED_VERSION))>=0);
    s= get_pcp_server(sindx);
    TEST(s!=NULL);

    memset(&src4, 0, sizeof(src4));
    memset(&dst4, 0, sizeof(dst4));
    src4.sin_family = dst4.sin_family = AF_INET;
    src4.sin_addr.s_addr = s->src_ip[3];
    dst4.sin_addr.s_addr = 0x08080808;
    src4.sin_port = htons(1234);
    dst4.sin_port = 0x0909;
    src = (struct sockaddr*) &src4;
    dst = (struct sockaddr*) &dst4;
    f = pcp_new_flow(src, dst, NULL, IPPROTO_TCP, 600);
    s= get_pcp_server(sindx2);
    s->af=AF_UNSPEC;
    memset(&src6, 0, sizeof(src6));
    src6.sin6_family = AF_INET6;
    memcpy(&src6.sin6_addr, s->src_ip, sizeof(src6.sin6_addr));
    src6.sin6_port = htons(1234);
    src = (struct sockaddr*) &src6;
    f = pcp_new_flow(src, dst, NULL, IPPROTO_TCP, 600);
    ret = pcp_wait(f, 13000, 1);
    pcp_logger(PCP_DEBUG_INFO, "1st wait finished with result: %d\n", ret);

    ret = pcp_wait(f, 20000, 1);
    pcp_logger(PCP_DEBUG_INFO, "2nd wait finished with result: %d\n", ret);

    printf("\nFinal statuses of PCP servers:\n");
    pcp_db_foreach_server(print_status, NULL);

    PD_SOCKET_CLEANUP();

    return 0;
}
