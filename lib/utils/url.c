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

char *convert_url (const char *to_convert)
{
  static const char *spaces = " \t";
  char *ret, *p;

  if (to_convert == NULL) return NULL;

  size_t count = strspn(to_convert, spaces);
  count *= 3; // replace each space with %20
  count += strspn(to_convert, "%") * 2;

  count += strlen(to_convert) + 1;
  
  ret = (char *)malloc(count);
  p = ret;
  while (*to_convert != '\0') {
    if (isspace(*to_convert)) {
      *p++ = '%';
      *p++ = '2';
      *p++ = '0';
      to_convert++;
    } else if (*to_convert == '%') {
      *p++ = '%';
      *p++ = '%';
      to_convert++;
    } else
      *p++ = *to_convert++;
  }
  return ret;
}

  
