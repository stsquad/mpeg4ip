#ifndef __SYSTEMS_H__
#define __SYSTEMS_H__

#include <stdio.h>
#include <errno.h>
#ifdef _WINDOWS
#include <windows.h>
#include <time.h>
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef unsigned short in_port_t;
typedef unsigned int socklen_t;

#define snprintf _snprintf
#define strncasecmp _strnicmp
#define strcasecmp _stricmp

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
#else
/*
 * Unix definitions
 */
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#define closesocket close
#define IOSBINARY ios::bin
#define MAX_UINT64 -1LLU
#endif
#endif
