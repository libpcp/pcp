/*
 *------------------------------------------------------------------
 * test_pcp_api.c
 *
 * May 10, 2013, ptatrai
 *
 * Copyright (c) 2013-2013 by cisco Systems, Inc.
 * All rights reserved.
 *
 *------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pcp_client_db.h"
#include "test_macro.h"
#include "unp.h"
#include "pcp_socket.h"

int main()
{
    pcp_flow_t f1, f2;

    pcp_log_level = PCP_DEBUG_NONE;

    PD_SOCKET_STARTUP();
    //test pcp_init & terminate
    pcp_init(DISABLE_AUTODISCOVERY);
    TEST(get_pcp_server(0)==NULL);
    pcp_init(ENABLE_AUTODISCOVERY);
    TEST(get_pcp_server(0)!=NULL);
    pcp_terminate(1);
    TEST(get_pcp_server(0)==NULL);

    TEST(pcp_add_server(Sock_pton("[::1]:5351"), 1)==0);
    TEST(pcp_add_server(Sock_pton("127.0.0.1:5351"), 2)==1);
    pcp_terminate(1);

#ifdef PCP_SADSCP
    //TEST learn DSCP
    {
        pcp_flow_t l1;
        TEST((l1=pcp_learn_dscp(1,1,1,NULL))==NULL); //NO PCP server to send req

        TEST(pcp_add_server(Sock_pton("127.0.0.1:5351"), 2)==0);
        TEST((l1=pcp_learn_dscp(1,1,1,NULL))!=NULL);
        TEST(l1->kd.operation==PCP_OPCODE_SADSCP);
        TEST(l1->sadscp_app_name==NULL);
        TEST(l1->sadscp.app_name_length==0);
        TEST(l1->sadscp.toler_fields==84);
        pcp_close_flow(l1);
        pcp_delete_flow(l1);

        TEST((l1=pcp_learn_dscp(2,2,2,"test"))!=NULL);
        TEST(l1->sadscp.app_name_length==4);
        TEST(strncmp(l1->sadscp_app_name,"test",l1->sadscp.app_name_length)==0);
        TEST(l1->sadscp.toler_fields==168);

        pcp_terminate(1);
    }
#endif

    TEST(pcp_add_server(Sock_pton("127.0.0.1:5351"), 2)==0);
    //TEST pcp_set_read_fdset
    {
        int fd_max=0, last_fdmax;
        fd_set fds;
        pcp_server_t *s = get_pcp_server(0);
        FD_ZERO(&fds);
        pcp_set_read_fdset(&fd_max, &fds);

        TEST(fd_max>0);
        TEST(fd_max>s->pcp_server_socket);
        TEST(FD_ISSET( s->pcp_server_socket, &fds));

        last_fdmax=fd_max;
        run_server_state_machine(NULL, pcpe_terminate);
        run_server_state_machine(s, pcpe_terminate);
        TEST(s->pcp_server_socket == PCP_INVALID_SOCKET);
        fd_max=0;
        pcp_set_read_fdset(&fd_max, &fds);
        TEST(fd_max<last_fdmax);
        pcp_wait(NULL, 0, 0);
        TEST((pcp_new_flow(Sock_pton("[::1]:1234"), Sock_pton("[::1]"), NULL,
            IPPROTO_TCP, 100))==NULL);

    }

    pcp_handle_select(0, NULL, NULL);
    pcp_flow_get_info(NULL, NULL, NULL);

    //PCP PEER/MAP tests
    TEST(pcp_new_flow(NULL, NULL, NULL, 0, 0)==NULL);

    TEST(pcp_add_server(Sock_pton("[::1]:5351"), 1)==1);
    TEST((f1=pcp_new_flow(Sock_pton("[::1]:1234"), Sock_pton("[::1]"), NULL,
            IPPROTO_TCP, 100))!=NULL);
    TEST((f2=pcp_new_flow(Sock_pton("[::1]:1234"), NULL, NULL,
            IPPROTO_TCP, 100))!=NULL);
    pcp_flow_set_prefer_failure_opt(f2);
    pcp_flow_set_prefer_failure_opt(f2);

    pcp_flow_set_lifetime(f1, 1000);
    TEST((f1->lifetime)>=99);
    TEST((f1->timeout.tv_sec>0)||(f1->timeout.tv_usec>0));

    f1->timeout.tv_sec=0;
    f1->timeout.tv_usec=0;
    pcp_flow_set_lifetime(f1, 0);
    TEST(f1->lifetime==0);
    TEST((f1->timeout.tv_sec>0)||(f1->timeout.tv_usec>0));

    pcp_set_3rd_party_opt(f1,NULL);

    printf("Tests succeeded.\n\n");

    PD_SOCKET_CLEANUP();
    return 0;
}
