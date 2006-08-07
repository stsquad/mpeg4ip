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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *              Bill May        wmay@cisco.com
 */
#include "mpeg4ip.h"
#include "mpeg4ip_utils.h"
#if _WIN32
#include "mpeg4ip_sdl_includes.h"

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

static FILE *outfile = NULL;
static int global_loglevel = LOG_DEBUG;
int get_global_loglevel (void) 
{
  return global_loglevel;
}

void set_global_loglevel (int loglevel) 
{
  if (loglevel > LOG_DEBUG || loglevel < 0) return;
  global_loglevel = loglevel;
}

void open_log_file (const char *filename)
{
  if (outfile != NULL && outfile != stdout) {
    fclose(outfile);
  }
  outfile = fopen(filename, "w");
}

void flush_log_file (void)
{
  fflush(outfile);
}

void clear_log_file (void)
{
#ifndef _WIN32
  rewind(outfile);
  ftruncate(fileno(outfile), 0);
  rewind(outfile);
#endif
}

void close_log_file (void)
{
  fclose(outfile);
  outfile = NULL;
}

void message (int loglevel, const char *lib, const char *fmt, ...)
{
  va_list ap;
  struct timeval thistime;
  struct tm thistm;
  time_t secs;
  char buffer[80];
#if defined(_WIN32) && defined(_DEBUG)&& !defined(WINDOWS_IS_A_PIECE_OF_CRAP)
  if (IsDebuggerPresent()) {
    char msg[512];

    if (initialized == 0) init_local_mutex();
    lock_mutex();
    va_start(ap, fmt);
    _vsnprintf(msg, 512, fmt, ap);
    va_end(ap);
    OutputDebugString(msg);
    OutputDebugString("\n");
    unlock_mutex();
    return;
  }
#endif
  if (outfile == NULL) outfile = stdout;

  if (loglevel > global_loglevel) return;
  gettimeofday(&thistime, NULL);
  secs = thistime.tv_sec;
  // To add date, add %a %b %d to strftime
  localtime_r(&secs, &thistm);
  strftime(buffer, sizeof(buffer), 
#ifndef _WIN32
	       "%T",
#else
		   "%H:%M:%S",
#endif
		   &thistm);
  fprintf(outfile, "%s.%03lu-%s-%d: ",
	 buffer, (unsigned long)thistime.tv_usec / 1000, lib, loglevel);
  va_start(ap, fmt);
  vfprintf(outfile, fmt, ap);
  va_end(ap);
  fprintf(outfile, "\n");
}

void library_message (int loglevel,
		      const char *lib,
		      const char *fmt,
		      va_list ap)
{
  struct timeval thistime;
  struct tm thistm;
  time_t secs;
  char buffer[80];
#if defined(_WIN32) && defined(_DEBUG)&& !defined(WINDOWS_IS_A_PIECE_OF_CRAP)
  if (IsDebuggerPresent()) {
    char msg[512];

    if (initialized == 0) init_local_mutex();
    lock_mutex();
    sprintf(msg, "%s:", lib);
    OutputDebugString(msg);
    //va_start(ap, fmt);
    _vsnprintf(msg, 512, fmt, ap);
    //va_end(ap);
    OutputDebugString(msg);
    OutputDebugString("\n");
    unlock_mutex();
    return;
  }
#endif

  if (outfile == NULL) outfile = stdout;
  //  if (loglevel > global_loglevel) return;
  gettimeofday(&thistime, NULL);
  secs = thistime.tv_sec;
  localtime_r(&secs, &thistm);
  strftime(buffer, sizeof(buffer), 
#ifndef _WIN32
	       "%T",
#else
		   "%H:%M:%S",
#endif
	   &thistm);
  fprintf(outfile, "%s.%03lu-%s-%d: ",
	 buffer,
	 (unsigned long)thistime.tv_usec / 1000,
	 lib,
	 loglevel);
  vfprintf(outfile, fmt, ap);
  if (fmt[strlen(fmt) - 1] != '\n')
     fprintf(outfile, "\n");
}

