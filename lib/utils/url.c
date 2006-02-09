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

static const char *allowed="-_.!~*'();/?:@&=+$,";

char *unconvert_url (const char *from_convert) 
{
  char *ret, *to;
  if (from_convert == NULL) return NULL;
  ret = (char *)malloc(strlen(from_convert) + 1);
  to = ret;
  while (*from_convert != '\0') {
    if (*from_convert == '%') {
      from_convert++;
      if (*from_convert == '%') {
	*to++ = '%';
      } else {
	*to = (*from_convert - '0') << 4;
	from_convert++;
	*to |= *from_convert - '0';
	from_convert++;
	to++;
      }
    } else *to++ = *from_convert++;
  }
  *to = '\0';
  return ret;
}

char *convert_url (const char *to_convert)
{
  char *ret, *p;
  const char *q;
  size_t count;
  if (to_convert == NULL) return NULL;

  count = 0;
  q = to_convert;
  while (*q != '\0') {
    if (isalnum(*q)) count++;
    else if (strchr(allowed, *q) != NULL) {
      count++;
    } else count += 3;
    q++;
  }
  count++;

  ret = (char *)malloc(count);
  p = ret;
  while (*to_convert != '\0') {
    if (isalnum(*to_convert)) *p++ = *to_convert++;
    else if (strchr(allowed, *to_convert) != NULL) *p++ = *to_convert++;
    else {
      *p++ = '%';
      *p++ = '0' + ((*to_convert >> 4) & 0xf);
      *p++ = '0' + (*to_convert & 0xf);
      to_convert++;
    } 
  }
  *p++ = *to_convert; // final \0
  return ret;
}

  
