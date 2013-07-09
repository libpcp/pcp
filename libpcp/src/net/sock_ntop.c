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

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>    /* basic system data types */
#ifdef WIN32
#include    "pcp_win_defines.h"
#else
#include    <sys/un.h>        /* for Unix domain sockets */
#include    <sys/socket.h>    /* basic socket definitions */
#include    <netinet/in.h>    /* sockaddr_in{} and other Internet defns */
#include    <arpa/inet.h>    /* inet(3) functions */
#include    <netdb.h>
#endif
#include    <errno.h>
#include    <ctype.h>
#include    "gateway.h"

#include    "unp.h"

#ifdef    HAVE_SOCKADDR_DL_STRUCT
#include    <net/if_dl.h>
#endif


/* include sock_ntop */
char *
sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char        portstr[8];
    static char str[128];        /* Unix domain is largest */

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in    *sin = (struct sockaddr_in *) sa;

        if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
            return(NULL);  //LCOV_EXCL_LINE
        if (ntohs(sin->sin_port) != 0) {
            snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
            strcat(str, portstr);
        }
        return(str);
    }
/* end sock_ntop */

#ifdef  AF_INET6
    case AF_INET6: {
        struct sockaddr_in6    *sin6 = (struct sockaddr_in6 *) sa;

        str[0] = '[';
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeof(str) - 1) == NULL)
            return(NULL);  //LCOV_EXCL_LINE
        if (ntohs(sin6->sin6_port) != 0) {
            snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
            strcat(str, portstr);
            return(str);
        }
        return (str + 1);
    }
#endif

#if defined (AF_UNIX) && !defined (WIN32)

    case AF_UNIX: {
        struct sockaddr_un    *unp = (struct sockaddr_un *) sa;

            /* OK to have no pathname bound to the socket: happens on
               every connect() unless client calls bind() first. */
        if (unp->sun_path[0] == 0)
            strcpy(str, "(no pathname bound)");
        else
            snprintf(str, sizeof(str), "%s", unp->sun_path);
        return(str);
    }
#endif /*defined (AF_UNIX) && !defined (WIN32)*/

#ifdef    HAVE_SOCKADDR_DL_STRUCT
    case AF_LINK: {
        struct sockaddr_dl    *sdl = (struct sockaddr_dl *) sa;

        if (sdl->sdl_nlen > 0)
            snprintf(str, sizeof(str), "%*s (index %d)",
                     sdl->sdl_nlen, &sdl->sdl_data[0], sdl->sdl_index);
        else
            snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
        return(str);
    }
#endif
    default:
        snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
                 sa->sa_family, salen);
        return(str);
    }
    return (NULL);
}

char *
Sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char    *ptr;

    if ( (ptr = sock_ntop(sa, salen)) == NULL)
        perror("sock_ntop");    /* inet_ntop() sets errno */ //LCOV_EXCL_LINE
    return(ptr);
}


int
sock_pton(const char* cp, struct sockaddr *sa)
{
    const char * ip_end;
    char * host_name = NULL;
    const char* port=NULL;
    if ((!cp)||(!sa)) {
        return -1;
    }

    //skip ws
    while ((cp)&&(isspace(*cp))) {
        ++cp;
    }

    ip_end = cp;
    if (*cp=='[') { //find matching bracket ']'
        ++cp;
        while ((*ip_end)&&(*ip_end!=']')) {
            ++ip_end;
        }

        if (!*ip_end) {
            return -2;
        }
        host_name=strndup(cp, ip_end-cp);
        ++ip_end;
    }
    { //find start of port part
        while (*ip_end) {
            if (*ip_end==':') {
                if (!port) {
                    port = ip_end+1;
                } else if (host_name==NULL) { // means addr has [] block
                    port=NULL; // more than 1 ":" => assume the whole addr is IPv6 address w/o port
                    host_name=strdup(cp);
                    break;
                }
            }
            ++ip_end;
        }
        if (!host_name) {
            if ((*ip_end==0)&&(port!=NULL)) {
                if (port-cp>1) { //only port entered
                    host_name=strndup(cp, port-cp-1);
                }
            } else {
                host_name=strndup(cp, ip_end-cp);
            }
        }
    }

    // getaddrinfo for host
    {
        struct addrinfo hints, *servinfo, *p;
        int rv;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_V4MAPPED;

        if ((rv = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n",
                    gai_strerror(rv));
            if (host_name)
                free (host_name);
            return -2;
        }

        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((p->ai_family == AF_INET)||(p->ai_family == AF_INET6)) {
                memcpy(sa, p->ai_addr, SA_LEN(p->ai_addr));
                if(host_name==NULL) { // getaddrinfo returns localhost ip if hostname is null
                    switch (p->ai_family) {
                    case AF_INET:
                        ((struct sockaddr_in*)sa)->sin_addr.s_addr = INADDR_ANY;
                        break;
                    case AF_INET6:
                        memset(&((struct sockaddr_in6*)sa)->sin6_addr, 0,
                                sizeof(struct sockaddr_in6));
                        break;
                    default:   // Should never happen LCOV_EXCL_START
                        if (host_name)
                            free (host_name);
                        return -2;
                    }          //LCOV_EXCL_STOP
                }
                break;
            }
        }
        freeaddrinfo(servinfo);
    }

    if (host_name)
        free (host_name);
    return 0;
}

struct sockaddr *Sock_pton(const char* cp)
{
    static struct sockaddr_storage sa_s;
    if (sock_pton(cp, (struct sockaddr *)&sa_s)==0) {
        return (struct sockaddr *)&sa_s;
    } else {
        return NULL;
    }
}

int
sock_pton_with_prefix(const char* cp, struct sockaddr *sa, int *int_prefix)
{
#define IPV4_OFFSET_PREFIX 96

    const char * prefix_begin = NULL;
    const char * find_prefix=NULL;
    char * prefix = NULL;
    struct sockaddr_in tmp_sa;
    struct sockaddr_in6 tmp_sa6;

    const char * ip_end;
    char * host_name = NULL;
    const char* port=NULL;
    if ((!cp)||(!sa)) {
        return -1;
    }

    //skip ws
    while ((cp)&&(isspace(*cp))) {
        ++cp;
    }

    ip_end = cp;
    find_prefix = cp;
    if (*cp=='[') { //find matching bracket ']'
        ++cp;
        while ((*ip_end)&&(*ip_end!=']')) {
            if (*ip_end == '/' ){
                prefix_begin = ip_end+1;
            }
            ++ip_end;
        }

        if (!*ip_end) {
            return -2;
        }

        if (prefix_begin){
            host_name=strndup(cp, prefix_begin-cp-1);
            prefix = strndup(prefix_begin, ip_end-prefix_begin);
            *int_prefix = atoi(prefix);

            if ( inet_pton(AF_INET, host_name,
                    &tmp_sa.sin_addr ) ) {
                if (*int_prefix>32) {
                    //cleanup
                    if (host_name)
                        free(host_name);
                    if (prefix)
                        free(prefix);
                    return -2;
                } else {//prepare prefix for IPv4 mapped IPv6
                    *int_prefix = *int_prefix + IPV4_OFFSET_PREFIX;
                }
            }
            if (*int_prefix > 128) {
                if (host_name)
                    free(host_name);
                if (prefix)
                    free(prefix);
                return -2;
            }
        } else {
            host_name=strndup(cp, ip_end-cp);
            if ( inet_pton(AF_INET, host_name,
                    &tmp_sa.sin_addr) ) {
                *int_prefix = IPV4_OFFSET_PREFIX + 32;
            } else if ( inet_pton(AF_INET6, host_name,
                    &tmp_sa6.sin6_addr) ) {
                *int_prefix = 128;
            }
        }
        ++ip_end;
    } else {
        return -2;
    }

    //if (host_name)
    //    fprintf(stderr, "host_name in [] is %s\n", host_name);
    //if (prefix)
    //    fprintf(stderr, "prefix in [] is %s and uint prefix %d \n", prefix, *uint_prefix);

    {//find begining of the prefix
        if (!prefix_begin) {
            while(*find_prefix){
                if (*find_prefix == '/'){
                    prefix_begin = find_prefix+1;
                }
                ++find_prefix;
            }
        }
    }

    { //find start of port part
        while (*ip_end) {
            if (*ip_end==':') {
                if (!port) {
                    port = ip_end+1;
                } else if (host_name==NULL) { // means addr has [] block
                    port=NULL; // more than 1 ":" => assume the whole addr is IPv6 address w/o port
                    host_name=strdup(cp);
                    break;
                }
            }
            ++ip_end;
        }
        if (!host_name) {
            if ((*ip_end==0)&&(port!=NULL)) {
                if (port-cp>1) { //only port entered
                    host_name=strndup(cp, port-cp-1);
                }
            } else {
                host_name=strndup(cp, ip_end-cp);
            }
        }
    }

    // getaddrinfo for host
    {
        struct addrinfo hints, *servinfo, *p;
        int rv;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_V4MAPPED;

        if ((rv = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n",
                    gai_strerror(rv));
            if (host_name)
                free (host_name);
            return -2;
        }

        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((p->ai_family == AF_INET)||(p->ai_family == AF_INET6)) {
                memcpy(sa, p->ai_addr, SA_LEN(p->ai_addr));
                if(host_name==NULL) { // getaddrinfo returns localhost ip if hostname is null
                    switch (p->ai_family) {
                    case AF_INET6:
                        memset(&((struct sockaddr_in6*)sa)->sin6_addr, 0,
                                sizeof(struct sockaddr_in6));
                        break;
                    default:   // Should never happen LCOV_EXCL_START
                        if (host_name)
                            free (host_name);
                        return -2;
                    }          //LCOV_EXCL_STOP
                }
                break;
            }
        }
        freeaddrinfo(servinfo);
    }

    if (host_name)
        free (host_name);
    return 0;
}
