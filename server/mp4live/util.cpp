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
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include "util.h"

void error_message (const char *fmt, ...)
{
  va_list ap;
  struct timeval thistime;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  // To add date, add %a %b %d to strftime
  strftime(buffer, sizeof(buffer), "%T", localtime(&thistime.tv_sec));
  printf("%s.%03ld-mp4live-3: ", buffer, thistime.tv_usec / 1000);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

bool PrintDebugMessages = false;

void debug_message (const char *fmt, ...)
{
  if (!PrintDebugMessages) {
    return;
  }

  va_list ap;
  struct timeval thistime;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  // To add date, add %a %b %d to strftime
  strftime(buffer, sizeof(buffer), "%T", localtime(&thistime.tv_sec));
  printf("%s.%03ld-mp4live-7: ", buffer, thistime.tv_usec / 1000);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

void lib_message (int loglevel,
		  const char *lib,
		  const char *fmt,
		  va_list ap)
{
  struct timeval thistime;
  time_t secs;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  secs = thistime.tv_sec;
  strftime(buffer, sizeof(buffer), "%T", localtime(&secs));
  printf("%s.%03lu-%s-%d: ",
	 buffer,
	 (unsigned long)thistime.tv_usec / 1000,
	 lib,
	 loglevel);
  vprintf(fmt, ap);
  printf("\n");
}

#if 0
void message (int loglevel, const char *lib, const char *fmt, ...)
{
  va_list ap;
  struct timeval thistime;
  time_t secs;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  secs = thistime.tv_sec;
  // To add date, add %a %b %d to strftime
  strftime(buffer, sizeof(buffer), "%T", localtime(&secs));
  printf("%s.%03lu-%s-%d: ",
	 buffer, (unsigned long)thistime.tv_usec / 1000, lib, loglevel);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

#endif
extern "C" char *get_host_ip_address (void)
{
  char sHostName[256];
  char sIpAddress[16];
  struct hostent* h;

  if (gethostname(sHostName, sizeof(sHostName)) < 0) {
    error_message("Couldn't gethostname"); 
    strcpy(sIpAddress, "0.0.0.0");
  } else {
    h = gethostbyname(sHostName);
    if (h == NULL) {
      debug_message("Couldn't gethostbyname of %s", sHostName); 
      strcpy(sIpAddress, "0.0.0.0");
    } else {
      strcpy(sIpAddress, 
	     inet_ntoa(*(struct in_addr*)(h->h_addr_list[0])));
    }
  }
  return strdup(sIpAddress);
}
