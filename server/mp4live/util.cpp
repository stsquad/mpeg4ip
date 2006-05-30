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

#include "mpeg4ip.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include "util.h"

void error_message (const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  library_message(LOG_ERR, "mp4live", fmt, ap);
  va_end(ap);
}

bool PrintDebugMessages = false;

void debug_message (const char *fmt, ...)
{
  if (!PrintDebugMessages) {
    return;
  }

  va_list ap;
  va_start(ap, fmt);
  library_message(LOG_DEBUG, "mp4live", fmt, ap);
  va_end(ap);
}

bool ValidateIpAddress (const char *address)
{
  struct in_addr in;
  if (inet_pton(AF_INET, address, &in) > 0) {
    return true;
  }
  
  struct in6_addr in6;
  if (inet_pton(AF_INET6, address, &in6) > 0) {
    return true;
  }

  // Might have a DNS address...
  if (gethostbyname(address) != NULL) {
    return true;
  }
  return false;
}

bool ValidateIpPort (in_port_t port)
{
  if (port < 1024 || port > 65534 || (port & 1)) {
    return false;
  }
  return true;
}

static void SeedRandom(void) {
  static bool once = false;
  if (!once) {
    srandom(time(NULL));
    once = true;
  }
}

in_addr_t GetRandomMcastAddress(void) 
{
  SeedRandom();

  // pick a random number in the multicast range
  u_int32_t mcast = ((random() & 0x0FFFFFFF) | 0xE0000000);
  
  // screen out undesirable values
  // introduces small biases in the results
  
  // stay away from 224.0.0.x
  if ((mcast & 0x0FFFFF00) == 0) {
    mcast |= 0x00000100;	// move to 224.0.1
  } 
  
  // stay out of SSM range 232.x.x.x
  // user should explictly select this if they want SSM
  if ((mcast & 0xFF000000) == 232) {
    mcast |= 0x01000000;	// move to 233
  }
  
  // stay away from .0 .1 and .255
  if ((mcast & 0xFF) == 0 || (mcast & 0xFF) == 1 
      || (mcast & 0xFF) == 255) {
    mcast = (mcast & 0xFFFFFFF0) | 0x4;	// move to .4 or .244
  }
  
  return htonl(mcast);
}

in_port_t GetRandomPort(void) 
{
  SeedRandom();

  // Get random block of 4 port numbers above 20000
  return (in_port_t)(20000 + ((random() >> 18) << 2));
}

char *create_payload_number_string (uint8_t payload)
{
  char *ret = (char *)malloc(4);
  sprintf(ret, "%u", payload);
  return ret;
}
