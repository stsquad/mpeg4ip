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
 * rtsp_command.c - process API calls to send/receive rtsp commands
 */
#include "rtsp_private.h"
#include <math.h>


static const char *UserAgent = "Cisco RTSP 1.0";

/*
 * rtsp_build_common()
 * Builds a common header based on rtsp_command_t information.
 */
static int rtsp_build_common (char *buffer,
			      uint32_t maxlen,
			      uint32_t *at,
			      rtsp_client_t *info,
			      rtsp_command_t *cmd,
			      const char *session)
{
  int ret;

  /*
   * The below is ugly, but it will allow us to remove a lot of lines
   * of code.  SNPRINTF_CHECK makes sure (for this routine), that we
   * don't have more data in the buffer than allowed - it will return
   * an error code if that happens.
   */
#define SNPRINTF_CHECK(fmt, value) \
  ret = snprintf(buffer + *at, maxlen - *at, (fmt), (value)); \
  if (ret == -1) { \
    return (-1); \
  }\
  *at += ret;

  SNPRINTF_CHECK("CSeq: %u\r\n", info->next_cseq);
  if (info->cookie) {
    SNPRINTF_CHECK("Cookie: %s\r\n", info->cookie);
  }

  if (cmd && cmd->accept) {
    SNPRINTF_CHECK("Accept: %s\r\n", cmd->accept);
  }
  if (cmd && cmd->accept_encoding) {
    SNPRINTF_CHECK("Accept-Encoding: %s\r\n", cmd->accept_encoding);
  }
  if (cmd && cmd->accept_language) {
    SNPRINTF_CHECK("Accept-Language: %s\r\n", cmd->accept_language);
  }
  if (cmd && cmd->authorization) {
    SNPRINTF_CHECK("Authorization: %s\r\n", cmd->authorization);
  }
  if (cmd && cmd->bandwidth != 0) {
    SNPRINTF_CHECK("Bandwidth: %u\r\n", cmd->bandwidth);
  }
  if (cmd && cmd->blocksize != 0) {
    SNPRINTF_CHECK("Blocksize: %u\r\n", cmd->blocksize);
  }
  if (cmd && cmd->cachecontrol) {
    SNPRINTF_CHECK("Cache-Control: %s\r\n", cmd->cachecontrol);
  }
  if (cmd && cmd->conference) {
    SNPRINTF_CHECK("Conference: %s\r\n", cmd->conference);
  }
  if (cmd && cmd->from) {
    SNPRINTF_CHECK("From: %s\r\n", cmd->from);
  }
  if (cmd && cmd->proxyauth) {
    SNPRINTF_CHECK("Proxy-Authorization: %s\r\n", cmd->proxyauth);
  }
  if (cmd && cmd->proxyrequire) {
    SNPRINTF_CHECK("Proxy-Require: %s\r\n", cmd->proxyrequire);
  }
  if (cmd && cmd->range) {
    SNPRINTF_CHECK("Range: %s\r\n", cmd->range);
  }
  if (cmd && cmd->referer) {
    SNPRINTF_CHECK("Referer: %s\r\n", cmd->referer);
  }
  if (cmd && cmd->scale != 0.0) {
    SNPRINTF_CHECK("Scale: %f\r\n", cmd->scale);
  }
  if (session) {
    SNPRINTF_CHECK("Session: %s\r\n", session);
  } else if (cmd && cmd->session) {
    SNPRINTF_CHECK("Session: %s\r\n", cmd->session);
  }
  if (cmd && cmd->speed != 0.0) {
    SNPRINTF_CHECK("Speed: %f\r\n", cmd->speed);
  }
  if (cmd && cmd->transport) {
    SNPRINTF_CHECK("Transport: %s\r\n", cmd->transport);
  }
    
  SNPRINTF_CHECK("User-Agent: %s\r\n",
	      (cmd && cmd->useragent != NULL ?  cmd->useragent : UserAgent));
  if (cmd && cmd->User) {
    SNPRINTF_CHECK("%s", cmd->User);
  }
#undef SNPRINTF_CHECK
  return (0);
}

/*
 * rtsp_send_describe - send the describe info to a server
 */
int rtsp_send_describe (rtsp_client_t *info,
			rtsp_command_t *cmd,
			rtsp_decode_t **decode_result)
{
  char buffer[2048];
  uint32_t maxlen, buflen;
  int ret;
  rtsp_decode_t *decode;

  *decode_result = NULL;
  info->redirect_count = 0;

  do {
    maxlen = sizeof(buffer);
    buflen = snprintf(buffer, maxlen, "DESCRIBE %s RTSP/1.0\r\n", info->url);

    if (rtsp_build_common(buffer, maxlen, &buflen, info, cmd, NULL) == -1) {
      return (RTSP_RESPONSE_RECV_ERROR);
    }
    
    ret = snprintf(buffer + buflen, maxlen - buflen, "\r\n");
    if (ret == -1) {
      return (RTSP_RESPONSE_RECV_ERROR);
    }
    buflen += ret;

    rtsp_debug(LOG_INFO, "Sending DESCRIBE %s", info->url);
    rtsp_debug(LOG_DEBUG, "%s", buffer);

    ret = rtsp_send_and_get(info, buffer, buflen);
    decode = info->decode_response;
    
    if (ret == RTSP_RESPONSE_GOOD) {
      rtsp_debug(LOG_INFO, "DESCRIBE returned correctly");
      *decode_result = info->decode_response;
      info->decode_response = NULL;
      return (RTSP_RESPONSE_GOOD);
    } else if (ret != RTSP_RESPONSE_REDIRECT) {
      
      rtsp_debug(LOG_ERR, "DESCRIBE return code %d", ret);
      if (ret != RTSP_RESPONSE_RECV_ERROR &&
	  decode != NULL) {
	*decode_result = info->decode_response;
	info->decode_response = NULL;
	rtsp_debug(LOG_ERR, "Error code %s %s",
		   decode->retcode,
		   decode->retresp);
      }
      return (ret);
    }
    /*
     * Handle this through the redirects
     */
  }  while (ret == RTSP_RESPONSE_REDIRECT);
  
  return (RTSP_RESPONSE_RECV_ERROR);
}

/*
 * rtsp_send_setup - When we get the describe, this will set up a
 * particular stream.  Use the session handle for all further commands for
 * the stream (play, pause, teardown).
 */
int rtsp_send_setup (rtsp_client_t *info,
		     const char *url,
		     rtsp_command_t *cmd,
		     rtsp_session_t **session_result,
		     rtsp_decode_t **decode_result,
		     int is_aggregate)
{
  char buffer[2048], *temp;
  uint32_t maxlen, buflen;
  int ret;
  rtsp_decode_t *decode;
  rtsp_session_t *sptr;
  
  *decode_result = NULL;
  *session_result = NULL;
  info->redirect_count = 0;

  if (cmd == NULL || cmd->transport == NULL || url == NULL) {
    return (RTSP_RESPONSE_MISSING_OR_BAD_PARAM);
  }

  if (strncmp(url, "rtsp://", strlen("rtsp://")) != 0) {
    return (RTSP_RESPONSE_BAD_URL);
  }

  temp = strchr(url + strlen("rtsp://"), '/');
  if (temp == NULL) {
    return (RTSP_RESPONSE_BAD_URL);
  }
  if (strncmp(url, info->url, temp - url) != 0) {
    rtsp_debug(LOG_ALERT, "Bad url %s", url);
    rtsp_debug(LOG_ALERT, "Should be %s", info->url);
    return (RTSP_RESPONSE_BAD_URL);
  }

  maxlen = sizeof(buffer);
  buflen = snprintf(buffer, maxlen, "SETUP %s RTSP/1.0\r\n", url);

  // always use the session here...
  if (rtsp_build_common(buffer,
			maxlen,
			&buflen,
			info,
			cmd,
			info->session) == -1) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }
  ret = snprintf(buffer + buflen, maxlen - buflen, "\r\n");
  if (ret == -1) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }
  buflen += ret;

  rtsp_debug(LOG_INFO, "Sending SETUP %s", url);
  rtsp_debug(LOG_DEBUG, "%s", buffer);

  ret = rtsp_send_and_get(info, buffer, buflen);
  decode = info->decode_response;
    
  if (ret == RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_INFO, "SETUP returned correctly");
    *decode_result = info->decode_response;
    info->decode_response = NULL;
#ifndef IPTV_COMPATIBLE
    if ((*decode_result)->session == NULL) {
      return (RTSP_RESPONSE_BAD);
    }
#endif
    if (is_aggregate && info->session != NULL) {
      if (strcmp(info->session, (*decode_result)->session) != 0) {
	rtsp_debug(LOG_ALERT, "Session for %s returned different %s %s",
		   url, info->session, (*decode_result)->session);
	return (RTSP_RESPONSE_BAD);
      }
    }
    sptr = info->session_list;
    // we really need to seperate out the session from the
    // rest of the Session: header
    while (sptr != NULL) {
      if (strcmp(sptr->url, url) == 0)
	break;
      sptr = sptr->next;
    }
    if (sptr == NULL) {
      sptr = malloc(sizeof(rtsp_session_t));
      if (sptr == NULL) {
	return (RTSP_RESPONSE_RECV_ERROR);
      }
      sptr->url = strdup(url);
      if ((*decode_result)->session != NULL)
	sptr->session = strdup((*decode_result)->session);
      else
	sptr->session = NULL;
      sptr->parent = info;
      sptr->next = info->session_list;
      info->session_list = sptr;
      info->session = sptr->session;
    }
    *session_result = sptr;
    return (RTSP_RESPONSE_GOOD);
  } else {
    rtsp_debug(LOG_ERR, "SETUP return code %d", ret);
    if (ret != RTSP_RESPONSE_RECV_ERROR &&
	decode != NULL) {
      *decode_result = info->decode_response;
      info->decode_response = NULL;
      rtsp_debug(LOG_ERR, "Error code %s %s",
		 decode->retcode,
		 decode->retresp);
    }
    return (ret);
  }
  
  return (RTSP_RESPONSE_RECV_ERROR);
}

/*
 * check_session - make sure that the session is correct for that command
 */
static int check_session (rtsp_session_t *session,
			   rtsp_command_t *cmd)
{
  rtsp_session_t *sptr;
  rtsp_client_t *info;

  info = session->parent;
  if (info == NULL) {
    rtsp_debug(LOG_ALERT, "Session doesn't point to parent");
    return (FALSE);
  }
  
  sptr = info->session_list;
  while (sptr != session && sptr != NULL) sptr = sptr->next;
  if (sptr == NULL) {
    rtsp_debug(LOG_ALERT, "session not found in info list");
    return (FALSE);
  }
  
  if ((cmd != NULL) &&
      (cmd->session != NULL) &&
      (strcmp(cmd->session, session->session) != 0)) {
    rtsp_debug(LOG_ALERT, "Have cmd->session set wrong");
    return (FALSE);
  }
  return (TRUE);
}

static int rtsp_send_play_or_pause (const char *command,
				    const char *url,
				    const char *session,
				    rtsp_client_t *info,
				    rtsp_command_t *cmd,
				    rtsp_decode_t **decode_result)
{
  char buffer[2048];
  uint32_t maxlen, buflen;
  int ret;
  rtsp_decode_t *decode;

  *decode_result = NULL;
  if (info->server_socket < 0) {
    return (RTSP_RESPONSE_CLOSED_SOCKET);
  }

  maxlen = sizeof(buffer);
  buflen = snprintf(buffer, maxlen, "%s %s RTSP/1.0\r\n", command, url);

  if (rtsp_build_common(buffer, maxlen, &buflen,
			info, cmd, session) == -1) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }
    
  ret = snprintf(buffer + buflen, maxlen - buflen, "\r\n");
  if (ret == -1) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }
  buflen += ret;

  rtsp_debug(LOG_INFO, "Sending %s %s", command, url);
  rtsp_debug(LOG_DEBUG, "%s", buffer);

  ret = rtsp_send_and_get(info, buffer, buflen);
  decode = info->decode_response;
    
  if (ret == RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_ERR, "%s returned correctly", command);
    *decode_result = info->decode_response;
    info->decode_response = NULL;

    return (RTSP_RESPONSE_GOOD);
  } else {
    rtsp_debug(LOG_ERR, "%s return code %d", command, ret);
    if (ret != RTSP_RESPONSE_RECV_ERROR &&
	decode != NULL) {
      *decode_result = info->decode_response;
      info->decode_response = NULL;
      rtsp_debug(LOG_ERR, "Error code %s %s",
		 decode->retcode,
		 decode->retresp);
    }
    return (ret);
  }
  
  return (RTSP_RESPONSE_RECV_ERROR);
}

/*
 * rtsp_send_play - send play command.  It helps if Range is set
 */
int rtsp_send_play (rtsp_session_t *session,
		    rtsp_command_t *cmd,
		    rtsp_decode_t **decode_result)
{
  if (check_session(session, cmd) == FALSE) {
    return (RTSP_RESPONSE_MISSING_OR_BAD_PARAM);
  }

  return (rtsp_send_play_or_pause("PLAY",
				  session->url,
				  session->session,
				  session->parent,
				  cmd,
				  decode_result));
}

/*
 * rtsp_send_pause - send a pause on a particular session
 */
int rtsp_send_pause (rtsp_session_t *session,
		     rtsp_command_t *cmd,
		     rtsp_decode_t **decode_result)
{
  if (check_session(session, cmd) == FALSE) {
    return (RTSP_RESPONSE_MISSING_OR_BAD_PARAM);
  }

  return (rtsp_send_play_or_pause("PAUSE",
				  session->url,
				  session->session,
				  session->parent,
				  cmd,
				  decode_result));
}

int rtsp_send_aggregate_play (rtsp_client_t *info,
			      const char *aggregate_url,
			      rtsp_command_t *cmd,
			      rtsp_decode_t **decode_result)
{
  return (rtsp_send_play_or_pause("PLAY",
				  aggregate_url,
				  info->session,
				  info,
				  cmd,
				  decode_result));
}

int rtsp_send_aggregate_pause (rtsp_client_t *info,
			       const char *aggregate_url,
			       rtsp_command_t *cmd,
			       rtsp_decode_t **decode_result)
{
  return (rtsp_send_play_or_pause("PAUSE",
				  aggregate_url,
				  info->session,
				  info,
				  cmd,
				  decode_result));
}

static int rtsp_send_teardown_common (rtsp_client_t *info,
				      const char *url,
				      const char *session,
				      rtsp_command_t *cmd,
				      rtsp_decode_t **decode_result)
{
  char buffer[2048];
  uint32_t maxlen, buflen;
  int ret;
  rtsp_decode_t *decode;
  
  *decode_result = NULL;
  if (info->server_socket < 0) {
    return (RTSP_RESPONSE_CLOSED_SOCKET);
  }

  maxlen = sizeof(buffer);
  buflen = snprintf(buffer, maxlen, "TEARDOWN %s RTSP/1.0\r\n", url);

  if (rtsp_build_common(buffer, maxlen, &buflen,
			info, cmd, session) == -1) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }
    
  ret = snprintf(buffer + buflen, maxlen - buflen, "\r\n");
  if (ret == -1) {
    return (RTSP_RESPONSE_RECV_ERROR);
  }
  buflen += ret;

  rtsp_debug(LOG_INFO, "Sending TEARDOWN %s", url);
  rtsp_debug(LOG_DEBUG, "%s", buffer);

  ret = rtsp_send_and_get(info, buffer, buflen);
  decode = info->decode_response;
    
  if (ret == RTSP_RESPONSE_GOOD) {
    rtsp_debug(LOG_INFO, "TEARDOWN returned correctly");
    *decode_result = info->decode_response;
    info->decode_response = NULL;
    return (RTSP_RESPONSE_GOOD);
  } else {
    rtsp_debug(LOG_ERR, "TEARDOWN return code %d", ret);
    if (ret != RTSP_RESPONSE_RECV_ERROR &&
	decode != NULL) {
      *decode_result = info->decode_response;
      info->decode_response = NULL;
      rtsp_debug(LOG_ERR, "Error code %s %s",
		 decode->retcode,
		 decode->retresp);
    }
    return (ret);
  }
  
  return (RTSP_RESPONSE_RECV_ERROR);
}
/*
 * rtsp_send_teardown.  Sends a teardown for a session.  We might eventually
 * want to provide a teardown for the base url, rather than one for each
 * session
 */
int rtsp_send_teardown (rtsp_session_t *session,
			rtsp_command_t *cmd,
			rtsp_decode_t **decode_result)
{
  int ret;
  rtsp_client_t *info;
  rtsp_session_t *sptr;
  if (check_session(session, cmd) == FALSE) {
    return (RTSP_RESPONSE_MISSING_OR_BAD_PARAM);
  }
  info = session->parent;

  ret = rtsp_send_teardown_common(info,
				  session->url,
				  session->session,
				  cmd,
				  decode_result);
  if (ret == RTSP_RESPONSE_GOOD) {
    if (info->session_list == session) {
      info->session_list = session->next;
    } else {
      sptr = info->session_list;
      while (sptr->next != session) sptr = sptr->next;
      sptr->next = session->next;
    }
    free_session_info(session);
  }
  return (ret);
}

int rtsp_send_aggregate_teardown (rtsp_client_t *info,
				  const char *url,
				  rtsp_command_t *cmd,
				  rtsp_decode_t **decode_result)
{
  int ret;
  rtsp_session_t *p;
  ret = rtsp_send_teardown_common(info,
				  url,
				  info->session,
				  cmd,
				  decode_result);
  if (ret == RTSP_RESPONSE_GOOD) {
    while (info->session_list != NULL) {
      p = info->session_list;
      info->session_list = info->session_list->next;
      free_session_info(p);
    }
  }
  return (ret);
}
  

		       
