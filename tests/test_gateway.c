/*
 *------------------------------------------------------------------
 * test_gateway.c
 *
 * Feb 4, 2013, ptatrai
 *
 * Copyright (c) 2013-2013 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#include "default_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "pcp.h"
#include "gateway.h"
#ifdef WIN32
#include <Netioapi.h>
#include <Winsock2.h>
#else /* WIN32 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "pcp_socket.h"
#include "test_macro.h"

void test_getgateways()
{
    struct in6_addr *gws=NULL, *gw;
    struct in_addr;
    int rCount = getgateways(&gws);
    gw=gws;

    TEST(rCount > 0)
    for(;rCount>0; rCount--) {
        char pb[128];
        printf("gw : %-20s\n", inet_ntop(AF_INET6, gw, pb, sizeof(pb)));
        gw++;
    }
    free(gws);

    TEST(getgateways(NULL)<0)
}

void test_sa_len()
{
    struct sockaddr sa;

    printf("Test sockaddr_in\n");
    sa.sa_family=AF_INET;
    TEST(SA_LEN(&sa) == sizeof(struct sockaddr_in));

    printf("Test sockaddr_in6\n");
    sa.sa_family=AF_INET6;
    TEST(SA_LEN(&sa) == sizeof(struct sockaddr_in6));

    printf("Test sockadd\n");
    sa.sa_family=AF_UNSPEC;
    TEST(SA_LEN(&sa) == sizeof(struct sockaddr));
}

int main()
{
    PD_SOCKET_STARTUP();

    printf("Testing get gateways:\n");
    test_getgateways();

    printf("Testing SA_LEN  \n");
    test_sa_len();

    PD_SOCKET_CLEANUP();

    return 0;
}
