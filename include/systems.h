/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May		wmay@cisco.com
 */

#ifndef __SYSTEMS_H__
#define __SYSTEMS_H__

#include <stdio.h>
#include <errno.h>

#ifdef WIN32

#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <time.h>


typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int64 u_int64_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned char u_int8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef unsigned short in_port_t;
typedef unsigned int socklen_t;

#define snprintf _snprintf
#define strncasecmp _strnicmp
#define strcasecmp _stricmp

#include <io.h>
#include <fcntl.h>
#define write _write
#define lseek _lseek
#define close _close
#define open _open
#define O_RDWR _O_RDWR
#define O_CREAT _O_CREAT
#define O_RDONLY _O_RDONLY

#define IOSBINARY ios::binary

#ifdef __cplusplus
extern "C" {
#endif
char *strsep(char **strp, const char *delim); 
int gettimeofday(struct timeval *t, void *);
#ifdef __cplusplus
}
#endif

#define MAX_UINT64 -1
#define LLD "%I64d"
#define LLU "%I64u"
#define LLX "%I64x"
#define M_LLU 1000i64
#define I_LLU 1i64

#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_INFO 6
#define LOG_DEBUG 7

#else /* UNIX */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/stat.h>

#define closesocket close
#define IOSBINARY ios::bin
#define MAX_UINT64 -1LLU
#define LLD "%lld"
#define LLU "%llu"
#define LLX "%llx"
#define M_LLU 1000LLU
#define I_LLU 1LLU
#endif

#include <stdarg.h>
typedef void (*error_msg_func_t)(int loglevel,
				 const char *lib,
				 const char *fmt,
				 va_list ap);

#endif /* __SYSTEMS_H__ */
