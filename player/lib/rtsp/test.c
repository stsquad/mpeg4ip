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
 * test.c - test program for rtsp library
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "/home/wmay/sdp/include/sdp.h"
#include "rtsp_private.h"

#if 0
static const char *optbuff =
"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n";
static const char *optbuff2 =
"OPTIONS * RTSP/1.0\r\nCSeq: 2\r\n\r\n";
#endif

int main (int argc, char **argv)
{
  rtsp_client_t *rtsp_client;
  int ret;
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  session_desc_t *sdp;
  media_desc_t *media;
  sdp_decode_info_t *sdpdecode;
  rtsp_session_t *session;
  int dummy;

  memset(&cmd, 0, sizeof(rtsp_command_t));

  argv++;
  rtsp_client = rtsp_create_client(*argv, &ret);

  if (rtsp_client == NULL) {
    printf("No client created - error %d\n", ret);
    return (1);
  }

  if (rtsp_send_describe(rtsp_client, &cmd, &decode) != RTSP_RESPONSE_GOOD) {
    printf("Describe response not good\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return(1);
  }

  sdpdecode = set_sdp_decode_from_memory(decode->body);
  if (sdpdecode == NULL) {
    printf("Couldn't get sdp decode\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return(1);
  }

  if (sdp_decode(sdpdecode, &sdp, &dummy) != 0) {
    printf("Couldn't decode sdp\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return (1);
  }
  free(sdpdecode);

  if (decode->content_base != NULL) {
    for (media = sdp->media; media != NULL; media = media->next) {
      if (media->control_string != NULL) {
	if (strncmp(media->control_string, "rtsp://", strlen("rtsp://")) != 0){
	  // missing content base - make an absolute url
	  char *str;
	  size_t cblen;
	  cblen = strlen(decode->content_base);
	  if (decode->content_base[cblen - 1] != '/') cblen++;
	  str = malloc(strlen(media->control_string) + cblen);
	  strcpy(str, decode->content_base);
	  if (decode->content_base[cblen - 1] != '/') {
	    strcat(str, "/");
	  }
	  strcat(str, media->control_string);
	  printf("converted %s %s to %s\n", decode->content_base,
		 media->control_string,
		 str);
	  free(media->control_string);
	  media->control_string = str;
	    
	}
      }
    }
  }

  free_decode_response(decode);
  decode = NULL;
  cmd.transport = "RTP/AVP;unicast;client_port=4588-4589";
  media = sdp->media;
  dummy = rtsp_send_setup(rtsp_client,
			  media->control_string,
			  &cmd,
			  &session,
			  &decode);

  if (dummy != RTSP_RESPONSE_GOOD) {
    printf("Response to setup is %d\n", dummy);
    free_session_desc(sdp);
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return (1);
  }
  free_decode_response(decode);
  cmd.range = "npt=0.0-30.0";
  cmd.transport = NULL;
  dummy = rtsp_send_play(session, &cmd, &decode);
  if (dummy != RTSP_RESPONSE_GOOD) {
    printf("response to play is %d\n", dummy);
  } else {
    sleep(10);
  }
  free_decode_response(decode);
  cmd.transport = NULL;
  dummy = rtsp_send_teardown(session, NULL, &decode);
  printf("Teardown response %d\n", dummy);
  free_session_desc(sdp);
  free_decode_response(decode);
  free_rtsp_client(rtsp_client);
  return (0);
}  
  
