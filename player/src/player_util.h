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
 *              Bill May        wmay@cisco.com
 */
#ifndef __PLAYER_UTIL_H__
#define __PLAYER_UTIL_H__

#include "mpeg4ip_utils.h"
#ifdef __cplusplus
extern "C" {
#endif

void player_error_message(const char *fmt, ...)
#ifndef _WIN32
       __attribute__((format(__printf__, 1, 2)))
#endif
;

void player_debug_message(const char *fmt, ...)
#ifndef _WIN32
       __attribute__((format(__printf__, 1, 2)))
#endif
	   ;

int getIpAddressFromInterface(const char *ifname,
			      struct in_addr *retval);
char *convert_number(char *transport, uint32_t *value);
char *convert_hex(char *transport, uint32_t *value);

#ifdef __cplusplus
}
#endif


#ifdef _WIN32
#define DEFINE_MESSAGE_MACRO(funcname, libname) \
static inline void funcname (int loglevel, const char *fmt, ...) \
{ \
	va_list ap; \
	va_start(ap, fmt); \
	library_message(loglevel, libname, fmt, ap); \
	va_end(ap); \
} 
#else
#define DEFINE_MESSAGE_MACRO(funcname, libname) 
#endif

static  __inline uint64_t get_time_of_day (void) {
  struct timeval t;
  gettimeofday(&t, NULL);
  return ((((uint64_t)t.tv_sec) * TO_U64(1000)) +
	  (((uint64_t)t.tv_usec) / TO_U64(1000)));
}

static __inline uint64_t get_time_of_day_usec (void) {
  struct timeval t;
  uint64_t result;
  gettimeofday(&t, NULL);
  result = t.tv_sec;
  result *= TO_U64(1000000);
  result += t.tv_usec;
  return result;
}

#endif
