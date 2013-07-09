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

#ifndef PCP_SOCKET_H
#define PCP_SOCKET_H

#ifdef WIN32

int pcp_win_sock_startup();
int pcp_win_sock_cleanup();

#define PD_SOCKET_STARTUP pcp_win_sock_startup
#define PD_SOCKET_CLEANUP pcp_win_sock_cleanup
#define PCP_SOCKET SOCKET
#define PCP_SOCKET_ERROR SOCKET_ERROR
#define PCP_INVALID_SOCKET INVALID_SOCKET
#define CLOSE(sockfd) closesocket(sockfd)

#else

#define PD_SOCKET_STARTUP()
#define PD_SOCKET_CLEANUP()
#define PCP_SOCKET int
#define PCP_SOCKET_ERROR (-1)
#define PCP_INVALID_SOCKET (-1)
#define CLOSE(sockfd) close(sockfd)

#endif

#endif /* PCP_SOCKET_H*/
