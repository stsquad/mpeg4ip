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
#include <sdp/sdp.h>
#include <time.h>
#include "rtsp_private.h"

#if 0
static const char *optbuff =
"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n";
static const char *optbuff2 =
"OPTIONS * RTSP/1.0\r\nCSeq: 2\r\n\r\n";
#endif
static void local_error_msg (int loglevel,
			     const char *lib,
			     const char *fmt,
			     va_list ap)
{
  struct timeval thistime;
  char buffer[80];
#if _WIN32
  if (IsDebuggerPresent()) {
        char msg[512];

		sprintf(msg, "%s-%d:", lib, loglevel);
		OutputDebugString(msg);
//        va_start(ap, fmt);
	    _vsnprintf(msg, 512, fmt, ap);
//        va_end(ap);
        OutputDebugString(msg);
		OutputDebugString("\n");
		return;
	}
#define sleep(a) Sleep((a) * 1000)
#endif
  long int msec;
  gettimeofday(&thistime, NULL);
  strftime(buffer, sizeof(buffer), "%X", localtime(&thistime.tv_sec));
  msec = thistime.tv_usec / 1000;
  printf("%s.%03ld-%s-%d: ",
	 buffer,
	 msec,
	 lib,
	 loglevel);
  vprintf(fmt, ap);
  printf("\n");

}

static int callback (void *foo)
{
  rtsp_debug(LOG_DEBUG, "callback - SDL thread %d\n", SDL_ThreadID());
  return (SDL_ThreadID());
}
static void do_relative_url_to_absolute (const char **control_string,
				  const char *base_url,
				  int dontfree)
{
  char *str;
  const char *cpystr;
  uint32_t cblen, malloclen;

  malloclen = cblen = strlen(base_url);

  if (base_url[cblen - 1] != '/') malloclen++;
  /*
   * If the control string is just a *, use the base url only
   */
  cpystr = *control_string;
  if (strcmp(cpystr, "*") != 0) {
    if (*cpystr == '/') cpystr++;

    // duh - add 1 for \0...
    str = (char *)malloc(strlen(cpystr) + malloclen + 1);
    if (str == NULL)
      return;
    strcpy(str, base_url);
    if (base_url[cblen - 1] != '/') {
      strcat(str, "/");
    }
    if (*cpystr == '/') cpystr++;
    strcat(str, cpystr);
  } else {
    str = strdup(base_url);
  }
  if (dontfree == 0) 
    CHECK_AND_FREE(*control_string);
  *control_string = str;
}

/*
 * convert_relative_urls_to_absolute - for every url inside the session
 * description, convert relative to absolute.
 */
static void convert_relative_urls_to_absolute (session_desc_t *sdp,
					const char *base_url)
{
  media_desc_t *media;
  
  if (base_url == NULL)
    return;

  if ((sdp->control_string != NULL) &&
      (strncmp(sdp->control_string, "rtsp://", strlen("rtsp://"))) != 0) {
    do_relative_url_to_absolute(&sdp->control_string, base_url, 0);
  }
  
  for (media = sdp->media; media != NULL; media = media->next) {
    if ((media->control_string != NULL) &&
	(strncmp(media->control_string, "rtsp://", strlen("rtsp://")) != 0)) {
      do_relative_url_to_absolute(&media->control_string, base_url, 0);
    }
  }
}

int main (int argc, char **argv)
{
  rtsp_client_t *rtsp_client;
  int ret;
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  #if 1
  session_desc_t *sdp;
  media_desc_t *media;
  sdp_decode_info_t *sdpdecode;
  rtsp_session_t *session;
  int dummy;
  #endif

  rtsp_set_error_func(local_error_msg);
  rtsp_set_loglevel(LOG_DEBUG);
  memset(&cmd, 0, sizeof(rtsp_command_t));

  callback(NULL);
  
  argv++;
  rtsp_client = rtsp_create_client_for_rtp_tcp(*argv, &ret, NULL, 0);

  if (rtsp_client == NULL) {
    printf("No client created - error %d\n", ret);
    return (1);
  }

  ret = rtsp_thread_perform_callback(rtsp_client, callback, NULL);
  rtsp_debug(LOG_DEBUG, "Return from callback is %d\n", ret);
  
  if (rtsp_send_describe(rtsp_client, &cmd, &decode) != RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_CRIT, "Describe response not good\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return(1);
  }

  sdpdecode = set_sdp_decode_from_memory(decode->body);
  if (sdpdecode == NULL) {
    rtsp_debug(LOG_CRIT, "Couldn't get sdp decode\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return(1);
  }

  if (sdp_decode(sdpdecode, &sdp, &dummy) != 0) {
    rtsp_debug(LOG_CRIT, "Couldn't decode sdp\n");
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return (1);
  }
  free(sdpdecode);

  if (decode->content_base != NULL) {
    convert_relative_urls_to_absolute(sdp, decode->content_base);
  } else {
    convert_relative_urls_to_absolute (sdp, *argv);
  }

  free_decode_response(decode);
  decode = NULL;
#if 0
  cmd.transport = "RTP/AVP;unicast;client_port=4588-4589";
#else
  cmd.transport = "RTP/AVP/TCP;unicast;interleaved=0-1";
#endif
  media = sdp->media;
  dummy = rtsp_send_setup(rtsp_client,
			  media->control_string,
			  &cmd,
			  &session,
			  &decode, 0);

  if (dummy != RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_DEBUG, "Response to setup is %d\n", dummy);
    sdp_free_session_desc(sdp);
    free_decode_response(decode);
    free_rtsp_client(rtsp_client);
    return (1);
  }
  free_decode_response(decode);
  cmd.range = "npt=0.0-30.0";
  cmd.transport = NULL;
  if (sdp->control_string != NULL) {
    // send aggregate play
    dummy = rtsp_send_aggregate_play(rtsp_client, sdp->control_string, &cmd, &decode);
  } else {
    dummy = rtsp_send_play(session, &cmd, &decode);
  }
  if (dummy != RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_INFO, "response to play is %d\n", dummy);
  } else {
    sleep(10);
  }
  free_decode_response(decode);
  cmd.range = NULL;
  if (sdp->control_string) {
    dummy = rtsp_send_aggregate_pause(rtsp_client, sdp->control_string, &cmd, &decode);
  } else {
    dummy = rtsp_send_pause(session, &cmd, &decode);
  }
  if (dummy != RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_INFO, "response to pause is %d\n", dummy);
  } else {
    sleep(5);
  }
  free_decode_response(decode);
  cmd.range = "npt=15.0-30.0";
  cmd.transport = NULL;
  if (sdp->control_string != NULL) {
    dummy = rtsp_send_aggregate_play(rtsp_client, sdp->control_string, &cmd, &decode);
  } else {
    dummy = rtsp_send_play(session, &cmd, &decode);
  }
  if (dummy != RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_INFO, "response to play is %d\n", dummy);
  } else {
    sleep(10);
  }
  free_decode_response(decode);
  cmd.transport = NULL;
  if (sdp->control_string != NULL) {
    dummy = rtsp_send_aggregate_teardown(rtsp_client, sdp->control_string, &cmd, &decode);
  } else {
    dummy = rtsp_send_teardown(session, NULL, &decode);
  }
  rtsp_debug(LOG_DEBUG, "Teardown response %d\n", dummy);
  sdp_free_session_desc(sdp);
  free_decode_response(decode);
  sleep(5);
  free_rtsp_client(rtsp_client);
  return (0);
}  
  






