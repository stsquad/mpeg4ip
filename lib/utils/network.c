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

char *get_host_ip_address (void)
{
  char sHostName[256];
  char sIpAddress[16];
  struct hostent* h;

  if (gethostname(sHostName, sizeof(sHostName)) < 0) {
    message(LOG_CRIT, "net", "Couldn't gethostname"); 
    strcpy(sIpAddress, "0.0.0.0");
  } else {
    h = gethostbyname(sHostName);
    if (h == NULL) {
      message(LOG_INFO, "net", "Couldn't gethostbyname of %s", sHostName); 
      strcpy(sIpAddress, "0.0.0.0");
    } else {
      strcpy(sIpAddress, 
	     inet_ntoa(*(struct in_addr*)(h->h_addr_list[0])));
    }
  }
  return strdup(sIpAddress);
}

