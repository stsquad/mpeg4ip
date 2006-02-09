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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

/*
 * http_test - provide test harness for libhttp.
 */
#include "mpeg4ip.h"
#include "http.h"

int main (int argc, char **argv)
{
  http_client_t *http_client;
  http_resp_t *http_resp;
  argv++;
  http_set_loglevel(LOG_DEBUG);

  http_client = http_init_connection(*argv);
  if (http_client == NULL) {
    printf("no client\n");
    return (1);
  }

  argv++;
  http_resp = NULL;
  printf("argv is %s\n", *argv);
  http_get(http_client, *argv, &http_resp);
  http_resp_free(http_resp);
  http_free_connection(http_client);
  return (0);
}

