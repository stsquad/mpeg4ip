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

#include "systems.h"
#ifndef _WINDOWS
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#endif

#include "util.h"

#if _WIN32 && _DEBUG
#include "SDL.h"
#include "SDL_thread.h"

SDL_mutex *outex;
static int initialized = 0;
static void init_local_mutex (void)
{
	outex = SDL_CreateMutex();
	initialized = 1;
}
static void lock_mutex(void)
{
	SDL_mutexP(outex);
} 
static void unlock_mutex(void)
{
	SDL_mutexV(outex);
}
#endif

void error_message (const char *fmt, ...)
{
  va_list ap;
#if _WIN32 && _DEBUG
        char msg[512];

		if (initialized) init_local_mutex();
		lock_mutex();
        va_start(ap, fmt);
	_vsnprintf(msg, 512, fmt, ap);
        va_end(ap);
        OutputDebugString(msg);
		OutputDebugString("\n");
		unlock_mutex();
#else
  struct timeval thistime;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  // To add date, add %a %b %d to strftime
  strftime(buffer, sizeof(buffer), "%X", localtime(&thistime.tv_sec));
  printf("%s.%03ld-mp4live: ", buffer, thistime.tv_usec / 1000);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
#endif
}

void debug_message (const char *fmt, ...)
{
  va_list ap;
#if _WIN32 && _DEBUG
       char msg[512];

	   if (initialized) init_local_mutex();
		lock_mutex();
        va_start(ap, fmt);
	_vsnprintf(msg, 512, fmt, ap);
        va_end(ap);
        OutputDebugString(msg);
		OutputDebugString("\n");
		unlock_mutex();
#else
  struct timeval thistime;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  // To add date, add %a %b %d to strftime
  strftime(buffer, sizeof(buffer), "%X", localtime(&thistime.tv_sec));
  printf("%s.%03ld-debug-mp4live: ", buffer, thistime.tv_usec / 1000);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
#endif
}

#ifdef _WINDOWS
#include <sys/timeb.h>

int gettimeofday (struct timeval *t, void *foo)
{
	struct _timeb temp;
	_ftime(&temp);
	t->tv_sec = temp.time;
	t->tv_usec = temp.millitm * 1000;
	return (0);
}

char *strsep (char **sptr, const char *delim)
{
	char *start, *ret;
	start = ret = *sptr;
	if ((ret == NULL) || ret == '\0') {
	   return (NULL);
	}

	while (*ret != '\0' &&
		   strchr(delim, *ret) == NULL) {
		ret++;
	}
	if (*ret == '\0') {
		*sptr = NULL;
	} else {
	    *ret = '\0';
	    ret++;
	    *sptr = ret;
	}
	return (start);
}

#endif
