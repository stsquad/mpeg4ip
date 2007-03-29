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
/* mpeg2ps_util.h - mpeg2 program stream utilities */

#include <mpeg4ip.h>
#include <time.h>
#include "mpeg2_ps.h"
#include "mpeg2ps_private.h"

static int mpeg2ps_debug_level = LOG_ALERT;
static error_msg_func_t mpeg2ps_error_msg = NULL;

void mpeg2ps_set_loglevel (int loglevel)
{
  mpeg2ps_debug_level = loglevel;
}

void mpeg2ps_set_error_func (error_msg_func_t func)
{
  mpeg2ps_error_msg = func;
}

void mpeg2ps_message (int loglevel, const char *fmt, ...)
{
  va_list ap;
  if (loglevel <= mpeg2ps_debug_level) {
    va_start(ap, fmt);
    if (mpeg2ps_error_msg != NULL) {
      (mpeg2ps_error_msg)(loglevel, "libmpeg2ps", fmt, ap);
    } else {
 #if _WIN32 && _DEBUG
	  char msg[1024];

      _vsnprintf(msg, 1024, fmt, ap);
      OutputDebugStringA(msg);
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
      printf("%s.%03ld-libmpeg2ps-%d: ",
	     buffer, (unsigned long)thistime.tv_usec / 1000, loglevel);
      vprintf(fmt, ap);
      printf("\n");
#endif
    }
    va_end(ap);
  }
}
static mpeg2ps_record_pes_t *create_record (off_t loc, uint64_t ts)
{
  mpeg2ps_record_pes_t *ret = MALLOC_STRUCTURE(mpeg2ps_record_pes_t);

  ret->next_rec = NULL;
  ret->dts = ts;
  ret->location = loc;
  return ret;
}

#define MPEG2PS_RECORD_TIME (TO_U64(5 * 90000))
void mpeg2ps_record_pts (mpeg2ps_stream_t *sptr, off_t location, 
			 mpeg2ps_ts_t *pTs)
{

  uint64_t ts;
  mpeg2ps_record_pes_t *p, *q;
  if (sptr->is_video) {
    if (pTs->have_dts == false) return;
    ts = pTs->dts;
  } else {
    if (pTs->have_pts == false) return;
    ts = pTs->pts;
  }

  if (sptr->record_first == NULL) {
    sptr->record_first = sptr->record_last = create_record(location, ts);
    return;
  } 
  if (ts > sptr->record_last->dts) {
    if (ts < MPEG2PS_RECORD_TIME + sptr->record_last->dts) return;
    sptr->record_last->next_rec = create_record(location, ts);
    return;
  }
  if (ts < sptr->record_first->dts) {
    if (ts < MPEG2PS_RECORD_TIME + sptr->record_first->dts) return;
    p = create_record(location, ts);
    p->next_rec = sptr->record_first;
    sptr->record_first = p;
    return;
  }
  p = sptr->record_first;
  q = p->next_rec;

  while (q != NULL && q->dts < ts) {
    p = q;
    q = q->next_rec;
  }
  if (p->dts + MPEG2PS_RECORD_TIME <= ts &&
      ts + MPEG2PS_RECORD_TIME <= q->dts) {
    p->next_rec = create_record(location, ts);
    p->next_rec->next_rec = q;
  }
}

mpeg2ps_record_pes_t *search_for_ts (mpeg2ps_stream_t *sptr, 
				     uint64_t dts)
{
  mpeg2ps_record_pes_t *p, *q;
  uint64_t p_diff, q_diff;
  if (sptr->record_last == NULL) return NULL;

  if (dts > sptr->record_last->dts) return sptr->record_last;

  if (dts < sptr->record_first->dts) return NULL;
  if (dts == sptr->record_first->dts) return sptr->record_first;

  p = sptr->record_first;
  q = p->next_rec;

  while (q != NULL && q->dts > dts) {
    p = q;
    q = q->next_rec;
  }
  if (q == NULL) {
    return sptr->record_last;
  }

  p_diff = dts - p->dts;
  q_diff = q->dts - dts;
  
  if (p_diff < q_diff) return p;
  if (q_diff > 90000) return p;

  return q;
}
  
  
