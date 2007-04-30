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
/*
 * test.c - test utility for sdp library
 */
#include "mpeg4ip.h"
#include <time.h>
#include "sdp.h"

int main (int argc, char **argv)
{
  sdp_decode_info_t *sdpd;
  session_desc_t *session;
  int err;
  int num_translated;
  char *formatted;
  
  argc--;
  argv++;

  if (argc == 0) {
    printf("No arguments specified\n");
    exit(1);
  }
  sdpd = set_sdp_decode_from_filename(*argv);
  if (sdpd == NULL) {
    printf("Didn't find file %s\n", *argv);
    exit(1);
  }
  err = sdp_decode(sdpd, &session, &num_translated);
  if (err != 0) {
    printf("couldn't decode it - error code is %d - number %d\n", err,
	   num_translated + 1);
  }

  if (num_translated != 0) {
    session_dump_list(session);
    err = sdp_encode_list_to_memory(session, &formatted, NULL);
    if (err == 0) {
      printf("%s\n", formatted);
    } else {
      printf("Error formating session %d\n", err);
    }
    free(formatted);
    sdp_free_session_desc(session);
  }
  sdp_decode_info_free(sdpd);

  return (0);
}



