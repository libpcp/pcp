#include "pcp_event_handler.c"
#include "pcp_api.c"
#include <stdio.h>
#include <time.h>
#include "test_macro.h"

#include "pcp_socket.h"
#ifdef WIN32
#include "pcp_gettimeofday.h"
#include <Netioapi.h>
#include <Winsock2.h>
#endif /* WIN32 */

#if 0
int gettimeofday (struct timeval *__restrict __tv,
             __timezone_ptr_t __tz)
{
    static unsigned t = 1363871491;
    __tv->tv_sec = t;
    __tv->tv_usec = 12345;
}
#endif

void fill_in6_addr(struct in6_addr *dst_ip6, uint16_t *dst_port,
        struct sockaddr* src);

void pcp_get_event_loop_timeout(pcp_ctx_t *ctx, struct timeval *select_timeout)
{
    pcp_server_t *s;
    s = get_pcp_server(ctx, 0);
    s->server_state = pss_wait_io_calc_nearest_timeout;
    gettimeofday(&s->next_timeout, NULL);

    pcp_pulse(ctx, select_timeout);
}

int
main(int argc, char **argv)
{
    struct sockaddr_in sa, sa2, sa3;
    int ret = 0;
    int cmp, p;
    pcp_flow_t* f;
    struct timeval t, t2, t3;
    struct flow_key_data kd;
    pcp_server_t* s;
    pcp_ctx_t *ctx;

    PD_SOCKET_STARTUP();

    pcp_log_level = PCP_DEBUG_WARN;
    ctx = pcp_init(0);

    sa.sin_family = AF_INET;
    inet_pton (AF_INET, "100.2.1.1", &sa.sin_addr);
    sa.sin_port = htons(5351);

    sa3.sin_family = AF_INET;
    inet_pton (AF_INET, "10.2.1.1", &sa.sin_addr);
    sa.sin_port = htons(5351);

    sa2.sin_family = AF_INET;
    inet_pton (AF_INET, "1.2.1.1", &sa2.sin_addr);
    sa2.sin_port = htons(5350);

    gettimeofday(&t, NULL);
    //test 1
    t2=t;
    t2.tv_sec+=1;
    t2.tv_usec+=15000;
    t3 = t;
    t3.tv_sec += 2; t3.tv_usec += 5000;
    cmp = timeval_comp(&t2,&t3);
    if (cmp>=0) { //LCOV_EXCL_START
        printf("File: %s:%d: Test 1 failed: %ds %dus < %ds %dus\n",
                __FILE__,__LINE__, (int)t2.tv_sec, (int)t2.tv_usec, (int)t3.tv_sec, (int)t3.tv_usec);
        ret = 1;
    } else {
        printf("File: %s:%d: Test 1 OK\n",
                __FILE__,__LINE__);
    }//LCOV_EXCL_STOP

    //test 2
    t2=t;
    t2.tv_sec+=1;
    t2.tv_usec+=15000;
    t3 = t;
    t3.tv_sec += 2; t3.tv_usec += 5000;
    cmp = timeval_comp(&t3,&t2);
    if (cmp<0) { //LCOV_EXCL_START
        printf("File: %s:%d: Test 2 failed: %ds %dus > %ds %dus\n",
                __FILE__,__LINE__, (int)t2.tv_sec, (int)t2.tv_usec, (int)t3.tv_sec, (int)t3.tv_usec);
        ret = 2;
    } else {
        printf("File: %s:%d: Test 2 OK\n",
                __FILE__,__LINE__);
    }//LCOV_EXCL_STOP

    //test 3
    t2=t;
    t2.tv_sec=0;
    t2.tv_usec=-15000;
    t3 = t;
    t3.tv_sec += 1; t3.tv_usec = -5000;
    cmp = timeval_comp(&t3,&t2)<0;
    if (cmp<0) { //LCOV_EXCL_START
        printf("File: %s:%d: Test 3 failed: %ds %dus > %ds %dus\n",
                __FILE__,__LINE__, (int)t2.tv_sec, (int)t2.tv_usec, (int)t3.tv_sec, (int)t3.tv_usec);
        ret = 2;
    } else {
        printf("File: %s:%d: Test 3 OK\n",
                __FILE__,__LINE__);
    }//LCOV_EXCL_STOP

    p = psd_add_pcp_server(ctx, (struct sockaddr*)&sa, PCP_MAX_SUPPORTED_VERSION);
    s = get_pcp_server(ctx, p);

    memset(&kd, 0, sizeof(kd));
    pcp_fill_in6_addr(&kd.src_ip, &kd.map_peer.src_port, (struct sockaddr*)&sa3);
    pcp_fill_in6_addr(&kd.map_peer.dst_ip, &kd.map_peer.dst_port,
            (struct sockaddr*)&sa);
    kd.map_peer.protocol = IPPROTO_TCP;

    f = pcp_create_flow(s, &kd);
    f->lifetime = 3600;
    pcp_db_add_flow(f);
    gettimeofday(&t, NULL);

    //test 3
    t2=t;
    t2.tv_sec+=1;
    t2.tv_usec+=10000;
    f->timeout=t2;
    t3.tv_sec = 2; t3.tv_usec = 5000;
    f->state = pfs_wait_resp;
    pcp_get_event_loop_timeout(ctx, &t3);
    if ((t3.tv_sec != 1) || ((t3.tv_usec+500)/1000 != 10)) {   //LCOV_EXCL_START
        printf("File: %s:%d: Test 3 failed: t.tv_sec = %d, t.tv_usec = %d\n",
                __FILE__,__LINE__, (int)t3.tv_sec, (int)t3.tv_usec);
        ret = 3;
    } else {                                                   //LCOV_EXCL_STOP
        printf("File: %s:%d: Test 3 OK\n",
                __FILE__,__LINE__);
    }

    //test 4

    memset(&kd, 0, sizeof(kd));
    pcp_fill_in6_addr(&kd.src_ip, &kd.map_peer.src_port, (struct sockaddr*)&sa2);
    pcp_fill_in6_addr(&kd.map_peer.dst_ip, &kd.map_peer.dst_port, (struct sockaddr*)&sa);
    kd.map_peer.protocol = IPPROTO_TCP;

    f = pcp_create_flow(get_pcp_server(ctx, p), &kd);
    f->lifetime = 3600;
    pcp_db_add_flow(f);
    t2=t;
    t2.tv_sec+=1;
    t2.tv_usec+=30000;
    f->timeout=t2;
    t3 = t;
    t3.tv_sec = 2; t3.tv_usec = 5000;
    f->state = pfs_wait_resp;
    pcp_get_event_loop_timeout(ctx, &t3);
    if ((t3.tv_sec != 1) || ((t3.tv_usec+500)/1000 != 10)) { //LCOV_EXCL_START
        printf("File: %s:%d: Test 4 failed: t.tv_sec = %d, t.tv_usec = %d\n",
                __FILE__,__LINE__, (int)t3.tv_sec, (int)t3.tv_usec);
        ret = 4;
    } else {
        printf("File: %s:%d: Test 4 OK\n",
                __FILE__,__LINE__);
    }//LCOV_EXCL_STOP

    //test 5
    t3.tv_sec = 1; t3.tv_usec = 15000;
    f->state = pfs_wait_resp;
    pcp_get_event_loop_timeout(ctx, &t3);
    if ((t3.tv_sec != 1) || ((t3.tv_usec+500)/1000 != 10)) { //LCOV_EXCL_START
        printf("File: %s:%d: Test 5 failed: t.tv_sec = %d, t.tv_usec = %d\n",
                __FILE__,__LINE__, (int)t3.tv_sec, (int)t3.tv_usec);
        ret = 5;
    } else {
        printf("File: %s:%d: Test 5 OK\n",
                __FILE__,__LINE__);
    } //LCOV_EXCL_STOP

    //test 6
    t3.tv_sec = 0; t3.tv_usec = 25000;
    f->state = pfs_wait_resp;
    pcp_get_event_loop_timeout(ctx, &t3);
    if ((t3.tv_sec != 0) || ((t3.tv_usec+500)/1000 != 25)) { //LCOV_EXCL_START
        printf("File: %s:%d: Test 6 failed: t.tv_sec = %d, t.tv_usec = %d\n",
                __FILE__,__LINE__, (int)t3.tv_sec, (int) t3.tv_usec);
        ret = 6;
    } else {
        printf("File: %s:%d: Test 6 OK\n",
                __FILE__,__LINE__);
    } //LCOV_EXCL_STOP

    TEST(0==timeval_comp(&t3,&t3));

    {
        pcp_recv_msg_t msg;
        pcp_server_t s;

        memset(&s, 0, sizeof(s));
        s.epoch = ~0;
        msg.recv_epoch = 10;
        msg.received_time = 20;

        TEST(0==compare_epochs(&msg, &s));
        TEST(0==compare_epochs(&msg, &s));

        msg.recv_epoch += 5;
        msg.received_time += 5;
        TEST(0==compare_epochs(&msg, &s));

        msg.recv_epoch += 10;
        msg.received_time += 12;
        TEST(0==compare_epochs(&msg, &s));

        msg.received_time -= 4;
        TEST(0==compare_epochs(&msg, &s));

        msg.received_time -= 4;
        TEST(0!=compare_epochs(&msg, &s));

        msg.received_time += 10;
        TEST(0!=compare_epochs(&msg, &s));
    }

    //test 7
/*    printf("Testing retransmit delay calcul\n");
    int32_t r=0;
    int ii=0;
    for (ii=0; ii<20; ii++) {
        r=PCP_RT(r);
        printf("%d\n",r);
    }
    printf("\n");
    int ran;
    r=0;
    printf("Round 2\n");
    for (ii=0; ii<20; ii++) {
        ran = 8192+(1024-(random()&2047));
        r=r<<1;
        r = (((ran * ((r>3?r:3)<1024?(r>3?r:3):1024))>>13));
        printf("%d\n",r);
    }*/

    pcp_terminate(ctx, 1);

    PD_SOCKET_CLEANUP();

    return ret;
} /* main() */
