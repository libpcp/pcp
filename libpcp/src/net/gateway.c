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

#include <ctype.h>
#include <errno.h>
//#include <syslog.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/sysctl.h>
#include <sys/socket.h>         // place it before <net/if.h> struct sockaddr
#endif //WIN32

#ifdef __linux__
#define USE_NETLINK
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
#include <sys/ioctl.h>          // ioctl()
#include <net/if.h>             //struct ifreq
#endif //WIN32

#ifdef WIN32
#undef USE_NETLINK
#undef USE_SOCKET_ROUTE
#define USE_WIN32_CODE

#include <winsock2.h>
#include <ws2ipdef.h>
#include <Iphlpapi.h>
#include <ws2tcpip.h>

#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_dl.h>          //struct sockaddr_dl
#define USE_SOCKET_ROUTE
#endif

#ifndef WIN32
#include <arpa/inet.h>          // inet_addr()
#include <net/route.h>          // struct rt_msghdr
#include <ifaddrs.h>            //getifaddrs() freeifaddrs()
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>         //IPPROTO_GRE sturct sockaddr_in INADDR_ANY
#endif

#if defined(BSD) || defined(__FreeBSD_kernel__)
#define USE_SOCKET_ROUTE
#undef USE_WIN32_CODE
#endif

#if (defined(sun) && defined(__SVR4))
#define USE_SOCKET_ROUTE
#undef USE_WIN32_CODE
#endif

#include "gateway.h"
#include "pcp_logger.h"
#include "unp.h"
#include "pcp_utils.h"

#ifndef WIN32
#define SUCCESS (0)
#define FAILED  (-1)
#define USE_WIN32_CODE
#endif


void get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info);

#define TO_IPV6MAPPED(x)        S6_ADDR32(x)[3] = S6_ADDR32(x)[0];\
                                S6_ADDR32(x)[0] = 0;\
                                S6_ADDR32(x)[1] = 0;\
                                S6_ADDR32(x)[2] = htonl(0xFFFF);


#ifdef USE_NETLINK

/* if we use USE_SOCKET_ROUTE it does not work on BSD based systems since there is no Netlink.
   Netlink needs a specific compilation directive
 */

#define BUFSIZE 8192

static int readNlSock(int sockFd, char *bufPtr, unsigned seqNum, unsigned pId){
    struct nlmsghdr *nlHdr;
    ssize_t readLen = 0, msgLen = 0;

    do{
        /* Receive response from the kernel */
        readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0);
        if(readLen == PCP_SOCKET_ERROR){ //LCOV_EXCL_START
            perror("SOCK READ: ");
            return -1;
        }//LCOV_EXCL_STOP

        nlHdr = (struct nlmsghdr *)bufPtr;

        /* Check if the header is valid */
        if ((NLMSG_OK(nlHdr, (unsigned)readLen) == 0) ||//LCOV_EXCL_START
            (nlHdr->nlmsg_type == NLMSG_ERROR))
        {
            perror("Error in recieved packet");
            return -1;
        }//LCOV_EXCL_STOP

        /* Check if the its the last message */
        if(nlHdr->nlmsg_type == NLMSG_DONE) {
            break;
        }
        else{
            /* Else move the pointer to buffer appropriately */
            bufPtr += readLen;
            msgLen += readLen;
        }

        /* Check if its a multi part message */
        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) { //LCOV_EXCL_START
            /* return if its not */
            break;
        } //LCOV_EXCL_STOP
    } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
    return msgLen;
}

int getgateways(struct in6_addr ** gws)
{
    struct nlmsghdr *nlMsg;
    char msgBuf[BUFSIZE];

    int sock, msgSeq = 0;
    ssize_t len;
    uint32_t rtCount;

    if (!gws) {

        return PCP_ERR_BAD_ARGS;
    }

    /* Create Socket */
    sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if(sock < 0) { //LCOV_EXCL_START
        perror("Socket Creation: ");
        return PCP_ERR_UNKNOWN;
    } //LCOV_EXCL_STOP

    /* Initialize the buffer */
    memset(msgBuf, 0, BUFSIZE);

    /* point the header and the msg structure pointers into the buffer */
    nlMsg = (struct nlmsghdr *)msgBuf;

    /* Fill in the nlmsg header*/
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table.

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

    /* Send the request */
    len = send(sock, nlMsg, nlMsg->nlmsg_len, 0);
    if(len == PCP_SOCKET_ERROR){ //LCOV_EXCL_START
        printf("Write To Socket Failed...\n");
        return PCP_ERR_UNKNOWN;
    } //LCOV_EXCL_STOP

    /* Read the response */
    len = readNlSock(sock, msgBuf, msgSeq, getpid());
    if(len < 0) { //LCOV_EXCL_START
        printf("Read From Socket Failed...\n");
        return PCP_ERR_UNKNOWN;
    } //LCOV_EXCL_STOP
    /* Parse and print the response */
    rtCount = 0;

    for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len)){
        {
            struct rtmsg *rtMsg;
            struct rtattr *rtAttr;
            int rtLen;
            rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

            /* If the route is not for AF_INET(6) or does not belong to main
               routing table then return. */
            if(((rtMsg->rtm_family != AF_INET) &&
                    (rtMsg->rtm_family != AF_INET6))
                    || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
                continue;
            }

            /* get the rtattr field */
            rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
            rtLen = RTM_PAYLOAD(nlMsg);
            for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){
                size_t rtaLen = RTA_PAYLOAD(rtAttr);
                if (rtaLen>sizeof(struct in6_addr)){
                    continue;
                }
                if (rtAttr->rta_type == RTA_GATEWAY) {
                    *gws = (struct in6_addr*)
                        realloc(*gws, sizeof(struct in6_addr)*(rtCount+1));

                    memset(*gws+rtCount, 0, sizeof(struct in6_addr));

                    memcpy((*gws)+rtCount, RTA_DATA(rtAttr), rtaLen);

                    if (rtMsg->rtm_family == AF_INET) {
                        TO_IPV6MAPPED(((*gws)+rtCount));
                    }
                    rtCount++;
                }

            }
        }
    }
    close(sock);
    return rtCount;
}

#endif /* #ifdef USE_NETLINK */


#if defined (USE_WIN32_CODE) && defined(WIN32)
int getgateways(struct in6_addr ** gws)
{
    PMIB_IPFORWARD_TABLE2 ipf_table;
    unsigned int i;

    if (gws == NULL) {
        return -1;
    }

    if (GetIpForwardTable2(AF_UNSPEC, &ipf_table)!=NO_ERROR) {
        return -1;
    }

    if (!ipf_table) {
        return -1;
    }

    *gws =(struct in6_addr*)calloc(ipf_table->NumEntries, sizeof(struct in6_addr));
    for (i=0; i<ipf_table->NumEntries; ++i) {
        if (ipf_table->Table[i].NextHop.si_family == AF_INET) {
            S6_ADDR32((*gws)+i)[0] = ipf_table->Table[i].NextHop.Ipv4.sin_addr.s_addr;
            TO_IPV6MAPPED(((*gws)+i));
        }
        if (ipf_table->Table[i].NextHop.si_family == AF_INET6) {
            memcpy((*gws)+i,
                &ipf_table->Table[i].NextHop.Ipv6.sin6_addr,
                sizeof(struct in6_addr));
        }
    }
    return i;
}
#endif /* #ifdef USE_WIN32_CODE */

#ifdef USE_SOCKET_ROUTE

static int
find_if_with_name(const char *iface, struct sockaddr_dl *out)
{
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_dl *sdl = NULL;

    if (getifaddrs(&ifap)) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_LINK &&
                /*(ifa->ifa_flags & IFF_POINTOPOINT) && \ */
                strcmp(iface, ifa->ifa_name) == 0) {
            sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            break;
        }
    }

    /* If we found it, then use it */
    if (sdl)
        bcopy((char *)sdl, (char *)out, (size_t) (sdl->sdl_len));

    freeifaddrs(ifap);

    if (sdl == NULL) {
        printf("interface %s not found or invalid(must be p-p)\n", iface);
        return -1;
    }
    return 0;
}

static int
route_op(u_char op, in_addr_t * dst, in_addr_t * mask, in_addr_t * gateway, char *iface, route_in_t *routein, route_out_t *routeout)
{

#define ROUNDUP_CT(n)  ((n) > 0 ? (1 + (((n) - 1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))
#define ADVANCE_CT(x, n) (x += ROUNDUP_CT((n)->sa_len))

#define NEXTADDR_CT(w, u) \
    if (msg.msghdr.rtm_addrs & (w)) {\
        len = ROUNDUP_CT(u.sa.sa_len); bcopy((char *)&(u), cp, len); cp += len;\
    }

    static int seq = 0;
    int err = 0;
    size_t len = 0;
    char *cp;
    pid_t pid;

    union {
        struct sockaddr sa;
        struct sockaddr_in sin;
        struct sockaddr_dl sdl;
        struct sockaddr_storage ss;     /* added to avoid memory overrun */
    } so_addr[RTAX_MAX];

    struct {
        struct rt_msghdr msghdr;
        char buf[512];
    } msg;

    bzero(so_addr, sizeof(so_addr));
    bzero(&msg, sizeof(msg));

    PCP_LOGGER_BEGIN(PCP_DEBUG_DEBUG);
    cp = msg.buf;
    pid = getpid();
    //msg.msghdr.rtm_msglen  = 0;
    msg.msghdr.rtm_version = RTM_VERSION;
    //msg.msghdr.rtm_type    = RTM_ADD;
    msg.msghdr.rtm_index = 0;
    msg.msghdr.rtm_pid = pid;
    msg.msghdr.rtm_addrs = 0;
    msg.msghdr.rtm_seq = ++seq;
    msg.msghdr.rtm_errno = 0;
    msg.msghdr.rtm_flags = 0;

    // Destination
    /* if (dst && *dst != 0xffffffff) { */
    if (routein && routein->dst4.sin_addr.s_addr != 0xffffffff) {
        msg.msghdr.rtm_addrs |= RTA_DST;

        so_addr[RTAX_DST].sin.sin_len = sizeof(struct sockaddr_in);
        so_addr[RTAX_DST].sin.sin_family = AF_INET;
        so_addr[RTAX_DST].sin.sin_addr.s_addr = mask ? *dst & *mask : *dst;
    } else {
        fprintf(stderr, "invalid(require) dst address.\n");
        return -1;
    }

    // Netmask
    if (mask && *mask != 0xffffffff) {
        msg.msghdr.rtm_addrs |= RTA_NETMASK;

        so_addr[RTAX_NETMASK].sin.sin_len = sizeof(struct sockaddr_in);
        so_addr[RTAX_NETMASK].sin.sin_family = AF_INET;
        so_addr[RTAX_NETMASK].sin.sin_addr.s_addr = *mask;

    } else
        msg.msghdr.rtm_flags |= RTF_HOST;

    switch (op) {
        case RTM_ADD:
        case RTM_CHANGE:
            msg.msghdr.rtm_type = op;
            msg.msghdr.rtm_addrs |= RTA_GATEWAY;
            msg.msghdr.rtm_flags |= RTF_UP;

            // Gateway
            if ((gateway && *gateway != 0x0 && *gateway != 0xffffffff)) {
                msg.msghdr.rtm_flags |= RTF_GATEWAY;

                so_addr[RTAX_GATEWAY].sin.sin_len = sizeof(struct sockaddr_in);
                so_addr[RTAX_GATEWAY].sin.sin_family = AF_INET;
                so_addr[RTAX_GATEWAY].sin.sin_addr.s_addr = *gateway;

                if (iface != NULL) {
                    msg.msghdr.rtm_addrs |= RTA_IFP;
                    so_addr[RTAX_IFP].sdl.sdl_family = AF_LINK;

                    //link_addr(iface, &so_addr[RTAX_IFP].sdl);
                    if (find_if_with_name(iface, &so_addr[RTAX_IFP].sdl) < 0)
                        return -2;
                }

            } else {
                if (iface == NULL) {
                    fprintf(stderr, "Requir gateway or iface.\n");
                    return -1;
                }

                if (find_if_with_name(iface, &so_addr[RTAX_GATEWAY].sdl) < 0)
                    return -1;
            }
            break;
        case RTM_DELETE:
            msg.msghdr.rtm_type = op;
            msg.msghdr.rtm_addrs |= RTA_GATEWAY;
            msg.msghdr.rtm_flags |= RTF_GATEWAY;
            break;
        case RTM_GET:
            msg.msghdr.rtm_type = op;
            msg.msghdr.rtm_addrs |= RTA_IFP;
            so_addr[RTAX_IFP].sa.sa_family = AF_LINK;
            so_addr[RTAX_IFP].sa.sa_len = sizeof(struct sockaddr_dl);
            break;
        default:
            return EINVAL;
    }

    NEXTADDR_CT(RTA_DST, so_addr[RTAX_DST]);
    NEXTADDR_CT(RTA_GATEWAY, so_addr[RTAX_GATEWAY]);
    NEXTADDR_CT(RTA_NETMASK, so_addr[RTAX_NETMASK]);
    NEXTADDR_CT(RTA_GENMASK, so_addr[RTAX_GENMASK]);
    NEXTADDR_CT(RTA_IFP, so_addr[RTAX_IFP]);
    NEXTADDR_CT(RTA_IFA, so_addr[RTAX_IFA]);

    msg.msghdr.rtm_msglen = len = cp - (char *)&msg;

    int sock = socket(PF_ROUTE, SOCK_RAW, AF_INET);
    if (sock == PCP_INVALID_SOCKET) {
        perror("socket(PF_ROUTE, SOCK_RAW, AF_INET) failed");
        return -1;
    }

    if (write(sock, (char *)&msg, len) < 0) {
        err = -1;
        goto end;
    }

    if (op == RTM_GET) {
        do {
            len = read(sock, (char *)&msg, sizeof(msg));
        } while (len > 0 && (msg.msghdr.rtm_seq != seq || msg.msghdr.rtm_pid != pid));

        if (len < 0) {
            perror("read from routing socket");
            err = -1;
        } else {
            struct sockaddr *s_netmask = NULL;
            struct sockaddr *s_gate = NULL;
            struct sockaddr_dl *s_ifp = NULL;
            struct sockaddr *sa;
            struct sockaddr *rti_info[RTAX_MAX];

            if (msg.msghdr.rtm_version != RTM_VERSION) {
                fprintf(stderr, "routing message version %d not understood\n", msg.msghdr.rtm_version);
                err = -1;
                goto end;
            }
            if (msg.msghdr.rtm_msglen > len) {
                fprintf(stderr, "message length mismatch, in packet %d, returned %lu\n", msg.msghdr.rtm_msglen, (unsigned long)len);
            }
            if (msg.msghdr.rtm_errno) {
                fprintf(stderr, "message indicates error %d, %s\n", msg.msghdr.rtm_errno, strerror(msg.msghdr.rtm_errno));
                err = -1;
                goto end;
            }
            cp = msg.buf;
            if (msg.msghdr.rtm_addrs) {

                sa = (struct sockaddr *)cp;
                get_rtaddrs(msg.msghdr.rtm_addrs, sa, rti_info);

                if ((sa = rti_info[RTAX_DST]) != NULL) {
                    if (sa->sa_family == AF_INET) {
                        char routeto4_str[INET_ADDRSTRLEN];

                        memcpy((void *)&(routeout->dst4), (void *)sa, sizeof(struct sockaddr_in));
                        inet_ntop(AF_INET, &(routein->dst4.sin_addr), routeto4_str, INET_ADDRSTRLEN);
                        //memcpy(routeto4_str, sock_ntop((struct sockaddr *)&(routein->dst4), sizeof(struct sockaddr_in)),
                        //        INET_ADDRSTRLEN);
                        /* BSD uses a peculiar nomenclature. 'Route to' is the host for which we are looking up
                           the route and 'destinaton' is the intermediate gateway or the final destination
                           when hosts are in the same LAN */
                        PCP_LOGGER(PCP_DEBUG_DEBUG, "Dest: %s", sock_ntop(sa, sa->sa_len));
                        if (msg.msghdr.rtm_flags & RTF_WASCLONED) {
                            PCP_LOGGER(PCP_DEBUG_DEBUG, "Route to %s in the same subnet", routeto4_str);
                        } else if (msg.msghdr.rtm_flags & RTF_CLONING) {
                            PCP_LOGGER(PCP_DEBUG_DEBUG, "Route to %s in the same subnet but not up", routeto4_str);
                        } else if (msg.msghdr.rtm_flags & RTF_GATEWAY) {
                            PCP_LOGGER(PCP_DEBUG_DEBUG, "Route to %s not in the same subnet", routeto4_str);
                        }
                        if (routeout->dst4.sin_addr.s_addr == routein->dst4.sin_addr.s_addr) {
                            PCP_LOGGER(PCP_DEBUG_DEBUG, "Destination %s same as route to %s",
                                    sock_ntop(sa, sa->sa_len), routeto4_str);
                        }
                    }
                }

                if ((sa = rti_info[RTAX_GATEWAY]) != NULL) {
                    s_gate = sa;
                    PCP_LOGGER(PCP_DEBUG_DEBUG, "Gateway: %s \n", sock_ntop(sa, sa->sa_len));
                    if (msg.msghdr.rtm_flags & RTF_GATEWAY) {
                        *gateway = ((struct sockaddr_in *)s_gate)->sin_addr.s_addr;
                        routeout->gw4.sin_addr.s_addr = ((struct sockaddr_in *)s_gate)->sin_addr.s_addr;
                    } else {
                        *gateway = 0;
                    }

                }

                if ((sa = rti_info[RTAX_IFP]) != NULL) {
                    PCP_LOGGER(PCP_DEBUG_DEBUG, "family: %d AF_LINK family : %d \n", sa->sa_family, AF_LINK);
                    if ((sa->sa_family == AF_LINK) && ((struct sockaddr_dl *)sa)->sdl_nlen) {
                        uint32_t slen;
                        s_ifp = (struct sockaddr_dl *)sa;
                        slen =  s_ifp->sdl_nlen < IFNAMSIZ ? s_ifp->sdl_nlen : IFNAMSIZ;
                        strncpy(iface, s_ifp->sdl_data, slen);
                        strncpy(&(routeout->ifname[0]), s_ifp->sdl_data, slen);
                        routeout->ifname[slen] = '\0';
                        iface[slen] = '\0';
                        PCP_LOGGER(PCP_DEBUG_DEBUG, "Interface name %s and type %d \n", &(routeout->ifname[0]), s_ifp->sdl_type);
                    }
                }
            }

            if (mask) {
                if (*dst == 0)
                    *mask = 0;
                else if (s_netmask)
                    *mask = ((struct sockaddr_in *)s_netmask)->sin_addr.s_addr;     // there must be something wrong here....Ah..
                else
                    *mask = 0xffffffff;     // it is a host
            }

#if 0
            if (iface && s_ifp) {
                strncpy(iface, s_ifp->sdl_data, s_ifp->sdl_nlen < IFNAMSIZ ? s_ifp->sdl_nlen : IFNAMSIZ);
                iface[IFNAMSIZ - 1] = '\0';
            }
#endif
        }
    }

end:
    if (close(sock) < 0) {
        perror("close");
    }

    PCP_LOGGER_END(PCP_DEBUG_DEBUG);
    return err;
#undef MAX_INDEX
}

    int
route_get_sa(struct sockaddr *dst, in_addr_t * mask, struct sockaddr * gateway, char *ifname, route_in_t *routein, route_out_t *routeout)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    return route_op(RTM_GET, &(((struct sockaddr_in *) dst)->sin_addr.s_addr), mask,
            &(((struct sockaddr_in *) gateway)->sin_addr.s_addr), ifname, routein, routeout);
#else
    printf("%s: todo...\n", __FUNCTION__);
    return 0;
#endif
}

int
route_get(in_addr_t * dst, in_addr_t * mask, in_addr_t * gateway, char iface[], route_in_t *routein, route_out_t *routeout)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    return route_op(RTM_GET, dst, mask, gateway, iface, routein, routeout);
#else
    printf("%s: todo...\n", __FUNCTION__);
    return 0;
#endif
}

int
route_add(in_addr_t dst, in_addr_t mask, in_addr_t gateway, const char *iface, route_in_t *routein, route_out_t *routeout)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    return route_op(RTM_ADD, &dst, &mask, &gateway, (char *)iface, routein, routeout);
#else
    printf("%s: todo...\n", __FUNCTION__);
    return -1;
#endif
}

int
route_change(in_addr_t dst, in_addr_t mask, in_addr_t gateway, const char *iface, route_in_t *routein, route_out_t *routeout)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    return route_op(RTM_CHANGE, &dst, &mask, &gateway, (char *)iface, routein, routeout);
#else
    printf("%s: todo...\n", __FUNCTION__);
    return -1;
#endif
}

int
route_delete(in_addr_t dst, in_addr_t mask, route_in_t *routein, route_out_t *routeout)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    return route_op(RTM_DELETE, &dst, &mask, 0, NULL, routein, routeout);
#else
    printf("%s: todo...\n", __FUNCTION__);
    return -1;
#endif
}


/* Adapted from Linux manual example */

/* We need to pass the family because an interface might have multiple addresses
   each assocaited with different family
   */
int
get_if_addr_from_name(char *ifname, struct sockaddr *ifsock, int family)
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        /* For an AF_INET* interface address, display the address */
        if (!strcmp(ifname, ifa->ifa_name) && (ifa->ifa_addr->sa_family == family)) {
            memcpy(ifsock, ifa->ifa_addr, sizeof(struct sockaddr));
            freeifaddrs(ifaddr);
            return 0;
        }

    }

    freeifaddrs(ifaddr);
    return -1;
}



/* Adapted from Richard Stevens, UNIX Network Programming  */

/*
 * Round up 'a' to next multiple of 'size', which must be a power of 2
 */
#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))

/*
 * Step to next socket address structure;
 * if sa_len is 0, assume it is sizeof(u_long). Using u_long only works on 32-bit
   machines. In 64-bit machines it needs to be u_int32_t !!
 */
#define NEXT_SA(ap)	ap = (struct sockaddr *) \
	((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof(uint32_t)) : \
									sizeof(uint32_t)))

/* thanks Stevens for this very handy function */
void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
	int		i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			NEXT_SA(sa);
		} else
			rti_info[i] = NULL;
	}
}

/* Portable (hopefully) function to lookup routing tables. sysctl()'s
   advantage is that it does not need root permissions. Routing sockets
   need root permission since it is of type SOCK_RAW. */
char *
net_rt_dump(int type, int family, int flags, size_t *lenp)
{
	int		mib[6];
	char	*buf;

	mib[0] = CTL_NET;
	mib[1] = AF_ROUTE;
	mib[2] = 0;
	mib[3] = family;		/* only addresses of this family */
	mib[4] = type;
	mib[5] = flags;			/* not looked at with NET_RT_DUMP */
	if (sysctl(mib, 6, NULL, lenp, NULL, 0) < 0)
		return(NULL);

	if ((buf = malloc(*lenp)) == NULL)
		return(NULL);
	if (sysctl(mib, 6, buf, lenp, NULL, 0) < 0)
		return(NULL);

	return(buf);
}

/* Performs a route table dump selecting only entries that have Gateways.
   This means that the return buffer will have many duplicate entries since
   certain gateways appear multiple times in the routing table.

   It is up to the caller to weed out duplicates
 */
int getgateways(struct in6_addr ** gws)
{
    char * buf, *next, *lim;
    size_t len;
    struct rt_msghdr	*rtm;
    struct sockaddr	*sa, *rti_info[RTAX_MAX];
    int rtcount = 0;


    /* net_rt_dump() will return all route entries with gateways */
    buf = net_rt_dump(NET_RT_FLAGS, 0, RTF_GATEWAY, &len);
    lim = buf + len;
    for (next = buf; next < lim; next += rtm->rtm_msglen) {
        rtm = (struct rt_msghdr *) next;
        sa = (struct sockaddr *)(rtm + 1);
        get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
        if ( (sa = rti_info[RTAX_DST]) != NULL) {
            printf("dest: %s", sock_ntop(sa, sa->sa_len));
        }

        if ( (sa = rti_info[RTAX_GATEWAY]) != NULL)
            printf(", gateway: %s \n", sock_ntop(sa, sa->sa_len));


            if ((rtm->rtm_addrs & (RTA_DST|RTA_GATEWAY)) == (RTA_DST|RTA_GATEWAY)) {
                struct in6_addr *in6 = NULL;

                *gws = (struct in6_addr*)
                    realloc(*gws, sizeof(struct in6_addr)*(rtcount + 1));

                memset(*gws + rtcount, 0, sizeof(struct in6_addr));

                in6 = (struct in6_addr *)((*gws) + rtcount);
                if (sa->sa_family == AF_INET) {
                    /* IPv4 gateways as returned as IPv4 mapped IPv6 addresses */
                    in6->s6_addr32[0] = ((struct sockaddr_in *)(rti_info[RTAX_GATEWAY]))->sin_addr.s_addr;
                    TO_IPV6MAPPED(in6);
                } else if (sa->sa_family == AF_INET6) {
                    memcpy(in6, &((((struct sockaddr_in6 *)rti_info[RTAX_GATEWAY]))->sin6_addr),
                            sizeof(struct in6_addr));
                } else {
                    continue;
                }
                rtcount++;
            }
    }
    free(buf);
    return rtcount;
}
#endif


#if 0

Examples of routes...Local network is 10.0.1/24

1 - A host in the same subnet which is up to respond to arp requests. Notice it has the
"WASCLONED" and "HOST" flag.

repenno-mac:pcp-client reinaldopenno$ route get 10.0.1.1
   route to: 10.0.1.1
destination: 10.0.1.1
  interface: en1
      flags: <UP,HOST,DONE,LLINFO,WASCLONED,IFSCOPE,IFREF>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
       0         0         0         0         0         0      1500      1109

2 - A host is the same subnet which is not up to respond to arp requests. Noticed it has the
"CLONING" flag but no HOST flag. The destination is still 10.0.1.0 (entire subnet) which
to the following route

10.0.1/24          link#5             UCS             3        0     en1

repenno-mac:pcp-client reinaldopenno$ route get 10.0.1.90
   route to: 10.0.1.90
destination: 10.0.1.0
       mask: 255.255.255.0
  interface: en1
      flags: <UP,DONE,CLONING,STATIC>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
       0         0         0         0         0         0      1500    -52784

3 - A host outside the subnet on another network. There is no destination only 0.0.0.0
and a PRCLONING

repenno-mac:pcp-client reinaldopenno$ route get 8.8.8.8
   route to: google-public-dns-a.google.com
destination: default
       mask: default
    gateway: 10.0.1.1
  interface: en1
      flags: <UP,GATEWAY,DONE,STATIC,PRCLONING>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
       0         0         0         0         0         0      1500         0
repenno-mac:pcp-client reinaldopenno$

PRCLONING was removed from FreeBSD

http://people.freebsd.org/~andre/FreeBSD-5.3-Networking.pdf
PRCLONING removed.
Routing Table
PRCLONING was previously done for two reasons. TCP stored/cached certain observations (for example RTT and RTT Variance) per remote host. For every host that has/had a TCP session with us it would create/clone a route to store these informations. Reduced rt_metrics from 14 to 3 fields and saving 11 times sizeof(u_long), on i386 44 Bytes. Most of the 11 removed fields are now in the
tcp_hostcache (see later):



#endif
