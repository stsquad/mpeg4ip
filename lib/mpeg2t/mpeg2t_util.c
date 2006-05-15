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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */
/* mpeg2t_util.h - mpeg2 transport stream utilities */

#include <mpeg4ip.h>
#include <time.h>
#include "mpeg2t_private.h"

static int mpeg2t_debug_level = LOG_ALERT;
static error_msg_func_t mpeg2t_error_msg = NULL;

void mpeg2t_set_loglevel (int loglevel)
{
  mpeg2t_debug_level = loglevel;
}

void mpeg2t_set_error_func (error_msg_func_t func)
{
  mpeg2t_error_msg = func;
}

void mpeg2t_message (int loglevel, const char *fmt, ...)
{
  va_list ap;
  if (loglevel <= mpeg2t_debug_level) {
    va_start(ap, fmt);
    if (mpeg2t_error_msg != NULL) {
      (mpeg2t_error_msg)(loglevel, "libmpeg2t", fmt, ap);
    } else {
 #if _WIN32 && _DEBUG
	  char msg[1024];

      _vsnprintf(msg, 1024, fmt, ap);
      OutputDebugString(msg);
      OutputDebugString("\n");
#else
      struct timeval thistime;
      struct tm thistm;
      char buffer[80];
      time_t secs;

      gettimeofday(&thistime, NULL);
      // To add date, add %a %b %d to strftime
      secs = thistime.tv_sec;
      localtime_r(&secs, &thistm);
      strftime(buffer, sizeof(buffer), "%X", &thistm);
      printf("%s.%03ld-libmpeg2t-%d: ",
	     buffer, (unsigned long)thistime.tv_usec / 1000, loglevel);
      vprintf(fmt, ap);
      printf("\n");
#endif
    }
    va_end(ap);
  }
}
