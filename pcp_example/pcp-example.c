#include <pcp/pcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

int main()
{
    struct sockaddr_in src, dst; int s; socklen_t src_len = sizeof(src);
    pcp_flow_t f; char buff[1024]; memset(buff, 0, sizeof(buff));

    dst.sin_family = AF_INET;
    dst.sin_port = htons(80);
    inet_pton(AF_INET, "www.google.com", &dst.sin_addr.s_addr);
    s = socket(s, SOCK_STREAM, 0);
    connect(s, (struct sockaddr *) &dst, sizeof(dst));
    getsockname(s, (struct sockaddr *) &src, &src_len);

    pcp_init(ENABLE_AUTODISCOVERY);
    f = pcp_new_flow((struct sockaddr*)&src, (struct sockaddr*)&dst, NULL, IPPROTO_TCP, 60);
    pcp_flow_set_flowp(f, 12, 16);
    pcp_wait(f, 500, 0);

    send(s, "GET index.html\n", sizeof("GET index.html\n"), 0);
    recv(s, buff, sizeof(buff), 0);
    printf("RESULT: %s\n", buff);

    close(s);
    pcp_close_flow(f);

    pcp_terminate(1);
    return 0;
}
