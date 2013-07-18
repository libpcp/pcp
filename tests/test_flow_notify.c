/*
 *------------------------------------------------------------------
 * test_flow_notify.c
 *
 * March 15th, 2013, Peter Tatrai
 *
 * Copyright (c) 2013-2013 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include "pcp_socket.h"
#include "pcp.h"
#include "unp.h"
#include "pcp_utils.h"
#include "test_macro.h"


uint8_t status;

pcp_flow_t flow = NULL;

void notify_cb_1(pcp_flow_t f, struct sockaddr* src_addr, struct sockaddr* ext_addr,
        pcp_fstate_e s)
{
    TEST(1==(status = ((f==flow) && (src_addr->sa_family == AF_INET))));

    TEST(1==(status=status && ((struct sockaddr_in*)src_addr)->sin_addr.s_addr == 0x0100007f));

    TEST(1==( status=status && ((struct sockaddr_in*)src_addr)->sin_port == htons(1235)));

    TEST(1==( status=status && (s == pcp_state_succeeded)));

    TEST(1==(status = status && ext_addr->sa_family == AF_INET));

    TEST((status = status && ((struct sockaddr_in*)ext_addr)->sin_addr.s_addr == 0x281e140a)==1);
}

void notify_cb_2(pcp_flow_t f, struct sockaddr* src_addr, struct sockaddr* ext_addr,
        pcp_fstate_e s)
{
    status = 0;
    TEST((f==flow) && (src_addr->sa_family == AF_INET6));

    TEST(((struct sockaddr_in6*)src_addr)->sin6_addr.s6_addr[15] == 0x01);

    TEST(((struct sockaddr_in6*)src_addr)->sin6_port == htons(1235));

    TEST(s == pcp_state_failed);

    TEST(ext_addr->sa_family == AF_INET6);

    status =1;
}

int select_loop()
{
    fd_set read_fds;
    int fdmax = 0;
    struct timeval tout_end;
    struct timeval tout_select;
    int timeout = 3000; //ms
    gettimeofday(&tout_end, NULL);
    tout_end.tv_usec += (timeout * 1000) % 1000000;
    tout_end.tv_sec += tout_end.tv_usec / 1000000;
    tout_end.tv_usec += tout_end.tv_usec % 1000000;
    tout_end.tv_sec += timeout / 1000;

    status = -1;
    for (;;) {
        // check expiration of wait timeout
        struct timeval ctv;
        gettimeofday(&ctv, NULL);
        TEST(!((timeval_subtract(&tout_select, &tout_end, &ctv))
                || ((tout_select.tv_sec == 0) && (tout_select.tv_usec == 0))
                || (tout_select.tv_sec < 0)));

        //process all events and get timeout value for next select
        pcp_handle_select(fdmax, &read_fds, &tout_select);

        if (status==1) {
            return 0;
        }

        FD_ZERO(&read_fds);
        pcp_set_read_fdset(&fdmax, &read_fds);

        select(fdmax, &read_fds, NULL, NULL, &tout_select);
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_storage destination;
    struct sockaddr_storage source;
    struct sockaddr_storage ext;
    int ret;
    uint8_t protocol = 6;
    uint32_t lifetime = 100;

    PD_SOCKET_STARTUP();

    pcp_log_level = argc>1?PCP_DEBUG_DEBUG:1;

    pcp_add_server(Sock_pton("127.0.0.1:5351"), 2);

    sock_pton("0.0.0.0:1234", (struct sockaddr*) &destination);
    sock_pton("127.0.0.1:1235", (struct sockaddr*) &source);
    sock_pton("10.20.30.40", (struct sockaddr*) &ext);

    printf("######################################\n");
    printf("####  * Begin test notify cb*     ####\n");
    printf("####  ***********************     ####\n");
    printf("######################################\n");

    pcp_set_flow_change_cb(notify_cb_1);

    flow = NULL;

    flow = pcp_new_flow((struct sockaddr*)&source,
                        (struct sockaddr*)&destination,
                        (struct sockaddr*)&ext,
                        protocol, lifetime);

    ret = select_loop();

    pcp_terminate(0);
    sleep(1);

    pcp_add_server(Sock_pton("[::1]:5351"), 2);

    pcp_set_flow_change_cb(notify_cb_2);

    flow = NULL;

    sock_pton("[::1]:1234", (struct sockaddr*) &destination);
    sock_pton("[::1]:1235", (struct sockaddr*) &source);
    sock_pton("[::1]", (struct sockaddr*) &ext);

    flow = pcp_new_flow((struct sockaddr*)&source,
                        (struct sockaddr*)&destination,
                        (struct sockaddr*)&ext,
                        protocol, lifetime);

    ret = ret || select_loop();
    pcp_terminate(0);

    PD_SOCKET_CLEANUP();

    return ret;
}
