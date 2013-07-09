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

#ifndef PCP_WIN_DEFINES
#define PCP_WIN_DEFINES

#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <winbase.h> /*GetCurrentProcessId*/ /*link with kernel32.dll*/
#include "stdint.h"
#define sleep(x) Sleep((x) * 1000) /* windows uses Sleep(miliseconds) method, instead of UNIX sleep(seconds) */

#ifdef _MSC_VER
#define inline __inline /*In Visual Studio inline keyword only available in C++ */
#endif

typedef uint16_t in_port_t;

#define ssize_t SSIZE_T
#define strdup _strdup

#define getpid GetCurrentProcessId

#define snprintf _snprintf

/* replacement for missing strndup() method on windows */
static inline char * strndup(const char *s, unsigned int size) {
  char *ret;
  char *end = memchr(s, 0, size);

  if (end) {
    /* Length + 1 */
    size = end - s + 1;
  } else {
    size++;
  }
  ret = malloc(size);

  if (size) {
      memcpy(ret, s, size);
      ret[size-1] = '\0';
  }
  return ret;
}

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif /*PCP_WIN_DEFINES*/
