/*
 *------------------------------------------------------------------
 * test_server_reping.c
 *
 * May, 2013, ptatrai
 *
 * Copyright (c) 2013-2013 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "pcp_socket.h"
#include "pcp.h"
#include "pcp_client_db.h"
#include "unp.h"
#include "test_macro.h"


int main(int argc, char *argv[]) {
    pcp_server_t *s = NULL;
    struct sockaddr_storage source_ip4;
    uint8_t protocol = 6;
    uint32_t lifetime = 10;
    pcp_flow_t flow = NULL;

    PD_SOCKET_STARTUP();
    pcp_log_level = 5;

    printf("\n");
    printf("#############################################\n");
    printf("####   *****************************     ####\n");
    printf("####   * Begin server timeout test *     ####\n");
    printf("####   *****************************     ####\n");
    printf("#############################################\n");

    sock_pton(":1111", (struct sockaddr*) &source_ip4);

    s=get_pcp_server(pcp_add_server(Sock_pton("127.0.0.1:5351"), 2));

    flow = pcp_new_flow((struct sockaddr*)&source_ip4,
                        NULL,
                        NULL,
                        protocol, lifetime);

    pcp_handle_select(0,NULL,NULL); //send packet
    TEST(s->server_state == pss_wait_ping_resp);
    s->server_state = pss_not_working;
    sleep(1);
    TEST( pcp_wait(flow, 7000, 0) == pcp_state_succeeded);

    TEST(s->server_state == pss_wait_io);
    pcp_terminate(1);

    s=get_pcp_server(pcp_add_server(Sock_pton("127.0.0.1:5351"), 2));

    flow = pcp_new_flow((struct sockaddr*)&source_ip4,
                        NULL,
                        NULL,
                        protocol, lifetime);

    pcp_handle_select(0,NULL,NULL); //send packet
    TEST(s->server_state == pss_wait_ping_resp);

    pcp_db_rem_flow(flow);
    flow->kd.nonce.n[0]++;
    pcp_db_add_flow(flow);

    sleep(1);
    TEST( pcp_wait(flow, 7000, 0) == pcp_state_succeeded);

    printf("Server state %d\n", s->server_state);
    TEST(s->server_state == pss_wait_io);
    pcp_terminate(1);

    s=get_pcp_server(pcp_add_server(Sock_pton("127.0.0.1:5351"), 2));

    flow = pcp_new_flow((struct sockaddr*)&source_ip4,
                        NULL,
                        NULL,
                        protocol, lifetime);

    TEST(pcp_wait(flow, 20000, 0) == pcp_state_failed);

    TEST(s->next_timeout.tv_sec>=time(NULL)+PCP_SERVER_DISCOVERY_RETRY_DELAY-2);
    s->next_timeout.tv_sec = (long)time(NULL)+1;
    sleep(2);
    pcp_handle_select(0,NULL,NULL);
    TEST(pcp_wait(flow, 20000, 0) == pcp_state_failed);
    TEST(s->server_state == pss_not_working);

    pcp_terminate(1);

    s=get_pcp_server(pcp_add_server(Sock_pton("1.1.1.1:5351"), 2));
    flow = pcp_new_flow((struct sockaddr*)&source_ip4,
                        NULL,
                        NULL,
                        protocol, lifetime);
    TEST(pcp_wait(flow, 40000, 0) == pcp_state_failed);
    TEST(s->server_state==pss_not_working);

    s->server_state=pss_ping;
    s->pcp_version++;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_not_working);

    s->server_state=pss_ping;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_not_working);

    s->server_state=pss_ping;
    pcp_flow_updated(flow);
    s->ping_flow_msg = NULL;
    s->pcp_server_socket = PCP_INVALID_SOCKET;
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_not_working);

    s->server_state=pss_wait_io;
    flow->state = pfs_wait_for_lifetime_renew;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(flow->state==pfs_failed);

    s->server_state=pss_wait_io;
    flow->state = pfs_wait_resp;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(flow->state==pfs_failed);

    s->server_state = pss_wait_ping_resp;
    s->ping_count = 0;
    s->ping_flow_msg = NULL;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_not_working);

    s->server_state = pss_version_negotiation;
    s->next_version = s->pcp_version;
    s->ping_count = 0;
    s->ping_flow_msg = NULL;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_wait_ping_resp);
    TEST(s->pcp_version == PCP_MAX_SUPPORTED_VERSION);

    s->server_state = pss_version_negotiation;
    s->ping_count = 0;
    s->ping_flow_msg = NULL;
    s->pcp_version = 0;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_not_working);

    s->pcp_version = PCP_MAX_SUPPORTED_VERSION;
    s->server_state = pss_wait_ping_resp;
    s->ping_count = 0;
    s->ping_flow_msg = NULL;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_wait_ping_resp);

    s->server_state=pss_wait_ping_resp;
    s->ping_count = PCP_MAX_PING_COUNT-1;
    pcp_flow_updated(flow);
    sleep(1);
    pcp_handle_select(0,NULL,NULL);
    TEST(s->server_state==pss_not_working);

    PD_SOCKET_CLEANUP();

    return 0;
}
