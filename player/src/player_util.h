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

typedef struct video_info_t {
  int height;
  int width;
  int frame_rate;
  int file_has_vol_header;
} video_info_t;

typedef struct audio_info_t {
  int freq;
} audio_info_t;

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
void message(int loglevel, const char *lib, const char *fmt, ...)
#ifndef _WIN32
       __attribute__((format(__printf__, 3, 4)))
#endif
	   ;

void player_library_message(int loglevel,
			    const char *lib,
			    const char *fmt,
			    va_list ap);
char *strcasestr(const char *haystack, const char *needle);

#ifdef _WIN32
#define DEFINE_MESSAGE_MACRO(funcname, libname) \
static inline void funcname (int loglevel, const char *fmt, ...) \
{ \
	va_list ap; \
	va_start(ap, fmt); \
	player_library_message(loglevel, libname, fmt, ap); \
	va_end(ap); \
} 
#else
#define DEFINE_MESSAGE_MACRO(funcname, libname) 
#endif
#ifdef __cplusplus
}
#endif

#endif
