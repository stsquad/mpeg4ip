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
 *		Bill May 		wmay@cisco.com
 */

#ifndef __MEDIA_TIME_H__
#define __MEDIA_TIME_H__

#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

typedef u_int64_t Timestamp;
#define TimestampTicks TO_U64(1000000)
#define TIMESTAMP_TICKS_TO_MSEC (TimestampTicks / TO_U64(1000))
typedef int64_t Duration;

inline Timestamp GetTimestampFromTimeval(struct timeval* pTimeval) {
	return ((u_int64_t)pTimeval->tv_sec * TimestampTicks) + pTimeval->tv_usec;
}

inline Timestamp GetTimestamp(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return GetTimestampFromTimeval(&tv);
}

inline u_int32_t GetSecsFromTimestamp(Timestamp t) {
	return t / TimestampTicks;
}

#ifndef NTP_TO_UNIX_TIME
#define NTP_TO_UNIX_TIME 2208988800UL
#endif
inline Timestamp GetTimestampFromNtp(uint32_t ntp_sec, uint32_t ntp_frac)
{
  Timestamp ret = ntp_frac;
  ret *= TimestampTicks;
  ret /= (TO_U64(1) << 32);
  if (ntp_sec > NTP_TO_UNIX_TIME) {
    ntp_sec -= NTP_TO_UNIX_TIME;
  }
  ret += (ntp_sec * TimestampTicks);
  return ret;
}

inline Duration GetTimescaleFromTicks (Duration toconvert,
				       uint32_t timescale)
{
  toconvert *= (Duration)timescale;
  toconvert /= TimestampTicks;
  return toconvert;
}
inline Timestamp GetTicksFromTimescale (Timestamp toconvert,
					Timestamp base_on_scale,
					Timestamp base_in_ticks,
					uint32_t timescale)
{
  Duration add_to_base;

  add_to_base = toconvert;
  add_to_base -= base_on_scale;
  add_to_base *= TimestampTicks;
  add_to_base /= (Duration)timescale;

  return add_to_base + base_in_ticks;
}
#endif /* __MEDIA_TIME_H__ */
