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
 * rtsp_resp.c - process rtsp response
 */

#include "rtsp_private.h"

static const char *end2 = "\n\r\n\r";
static const char *end1 = "\r\n\r\n";
static const char *end3 = "\r\r";
static const char *end4 = "\n\n";

/*
 * find_end_seperator()
 * Will search through the string pointer to by ptr, and figure out
 * what character is being used as a seperator - either \r \n, \r\n
 * or \n\r.
 *
 * Inputs:
 *    ptr - string to decode
 *    len - pointer to end seperator len
 *
 * Outputs:
 *    pointer to character to use as end seperator.  Dividing the len
 *    by 2, and adding to the return pointer will give the normal
 *    line seperator.
 */
static const char *find_end_seperator (const char *ptr, uint32_t *len)
{
  while (*ptr != '\0') {
    if (*ptr == '\r') {
      if (*(ptr + 1) == '\n') {
	*len = strlen(end1);
	return (end1);
      } else {
	*len = strlen(end3);
	return (end3);
      }
    } else if (*ptr == '\n') {
      if (*(ptr + 1) == '\r') {
	*len = strlen(end2);
	return (end2);
      } else {
	*len = strlen(end4);
	return (end4);
      }
    }
    ptr++;
  }
  return (NULL);
}

/************************************************************************
 * Decode rtsp header lines.
 *
 * These routines will decode the various RTSP header lines, setting the
 * correct information in the rtsp_decode_t structure.
 *
 * DEC_DUP_WARN macro will see if the field is already set, and send a warning
 * if it is.
 *
 ************************************************************************/
#define DEC_DUP_WARN(a, b) if (dec->a == NULL) dec->a = strdup(lptr); \
    else rtsp_debug(LOG_WARNING, "2nd "b" %s", lptr);

static void rtsp_header_allow_public (char *lptr,
				      rtsp_decode_t *decode)
{
  CHECK_AND_FREE(decode, allow_public);
  decode->allow_public = strdup(lptr);
}

static void rtsp_header_connection (char *lptr,
				    rtsp_decode_t *decode_response)
{
  if (strncasecmp(lptr, "close", strlen("close")) == 0) {
    decode_response->close_connection = TRUE;
  }
}

static void rtsp_header_cookie (char *lptr,
				rtsp_decode_t *dec)
{
  DEC_DUP_WARN(cookie, "cookie");
}

static void rtsp_header_content_base (char *lptr,
				      rtsp_decode_t *dec)
{
  DEC_DUP_WARN(content_base, "Content-Base");
}

static void rtsp_header_content_length (char *lptr,
					rtsp_decode_t *decode_response)
{
  decode_response->content_length = strtoul(lptr, NULL, 10);
}

static void rtsp_header_content_loc (char *lptr,
				     rtsp_decode_t *dec)
{
  DEC_DUP_WARN(content_location, "Content-Location");
}

static void rtsp_header_content_type (char *lptr,
				      rtsp_decode_t *dec)
{
  DEC_DUP_WARN(content_type, "Content-Type");
}

static void rtsp_header_cseq (char *lptr,
			      rtsp_decode_t *decode_response)
{
  decode_response->cseq = strtoul(lptr, NULL, 10);
}

static void rtsp_header_location (char *lptr,
				  rtsp_decode_t *dec)
{
  DEC_DUP_WARN(location, "Location");
}

static void rtsp_header_range (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(range, "Range");
}

static void rtsp_header_retry_after (char *lptr,
				     rtsp_decode_t *dec)
{
  // May try strptime if we need to convert
  DEC_DUP_WARN(retry_after, "Retry-After");
}

static void rtsp_header_rtp (char *lptr,
			     rtsp_decode_t *dec)
{
  if (dec->rtp_info != NULL) {
    char *newrtp;
    uint32_t len = strlen(dec->rtp_info);
    len += strlen(lptr);
    len++;
    newrtp = malloc(len);
    strcpy(newrtp, dec->rtp_info);
    strcat(newrtp, lptr);
    free(dec->rtp_info);
    dec->rtp_info = newrtp;
    rtsp_debug(LOG_DEBUG, "rtp adding new is %s", dec->rtp_info);
  } else {
    dec->rtp_info = strdup(lptr);
  }
}

static void rtsp_header_session (char *lptr,
				 rtsp_decode_t *dec)
{
  DEC_DUP_WARN(session, "Session");
}

static void rtsp_header_speed (char *lptr,
			       rtsp_decode_t *dec)
{
  DEC_DUP_WARN(speed, "Speed");
}

static void rtsp_header_transport (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(transport, "Transport");
}

static void rtsp_header_www (char *lptr,
			     rtsp_decode_t *dec)
{
  DEC_DUP_WARN(www_authenticate, "WWW-Authenticate");
}

static void rtsp_header_accept (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(accept, "Accept");
}

static void rtsp_header_accept_enc (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(accept_encoding, "Accept-Encoding");
}

static void rtsp_header_accept_lang (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(accept_language, "Accept-Language");
}

static void rtsp_header_auth (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(authorization, "Authorization");
}

static void rtsp_header_bandwidth (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(bandwidth, "Bandwidth");
}

static void rtsp_header_blocksize (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(blocksize, "blocksize");
}

static void rtsp_header_cache_control (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(cache_control, "Cache-control:");
}
static void rtsp_header_content_enc (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(content_encoding, "Content-Encoding:");
}
static void rtsp_header_content_lang (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(content_language, "Content-Language:");
}
static void rtsp_header_date (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(date, "Date:");
}
static void rtsp_header_expires (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(expires, "Expires:");
}
static void rtsp_header_from (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(from, "From:");
}
static void rtsp_header_ifmod (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(if_modified_since, "If-Modified-Since:")
}
static void rtsp_header_lastmod (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(last_modified, "Last-Modified:");
}
static void rtsp_header_proxyauth (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(proxy_authenticate, "Proxy-Authenticate:");
}
static void rtsp_header_proxyreq (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(proxy_require, "Proxy-Require:");
}
static void rtsp_header_referer (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(referer, "Referer:");
}
static void rtsp_header_scale (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(scale, "Scale:");
}
static void rtsp_header_server (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(server, "Server:");
}
static void rtsp_header_unsup (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(unsupported, "Unsupported:");
}
static void rtsp_header_uagent (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(user_agent, "User-Agent:");
}
static void rtsp_header_via (char *lptr, rtsp_decode_t *dec)
{
  DEC_DUP_WARN(via, "Via:");
}

/*
 * header_types structure will provide a lookup between certain headers
 * in RTSP, and the function routines (above) that deal with actions based
 * on the response
 */
static struct {
  const char *val;
  uint32_t val_length;
  void (*parse_routine)(char *lptr, rtsp_decode_t *decode);
  int allow_crlf_violation;
} header_types[] =
{
#define HEAD_TYPE(a, b) { a, sizeof(a), b, 0 }
  HEAD_TYPE("Allow:", rtsp_header_allow_public),
  HEAD_TYPE("Public:", rtsp_header_allow_public),
  HEAD_TYPE("Connection:", rtsp_header_connection),
  HEAD_TYPE("Content-Base:", rtsp_header_content_base),
  HEAD_TYPE("Content-Length:", rtsp_header_content_length),
  HEAD_TYPE("Content-Location:", rtsp_header_content_loc),
  HEAD_TYPE("Content-Type:", rtsp_header_content_type),
  HEAD_TYPE("CSeq:", rtsp_header_cseq),
  HEAD_TYPE("Location:", rtsp_header_location),
  HEAD_TYPE("Range:", rtsp_header_range),
  HEAD_TYPE("Retry-After:", rtsp_header_retry_after),
  {"RTP-Info:", sizeof("RTP-Info:"), rtsp_header_rtp, 1},
  HEAD_TYPE("Session:", rtsp_header_session),
  HEAD_TYPE("Set-Cookie:", rtsp_header_cookie),
  HEAD_TYPE("Speed:", rtsp_header_speed),
  HEAD_TYPE("Transport:", rtsp_header_transport),
  HEAD_TYPE("WWW-Authenticate:", rtsp_header_www),
  // None of these are needed for client, but are included for completion
  HEAD_TYPE("Accept:", rtsp_header_accept),
  HEAD_TYPE("Accept-Encoding:", rtsp_header_accept_enc),
  HEAD_TYPE("Accept-Language:", rtsp_header_accept_lang),
  HEAD_TYPE("Authorization:", rtsp_header_auth),
  HEAD_TYPE("Bandwidth:", rtsp_header_bandwidth),
  HEAD_TYPE("Blocksize:", rtsp_header_blocksize),
  HEAD_TYPE("Cache-Control:", rtsp_header_cache_control),
  HEAD_TYPE("Content-Encoding:", rtsp_header_content_enc),
  HEAD_TYPE("Content-Language:", rtsp_header_content_lang),
  HEAD_TYPE("Date:", rtsp_header_date),
  HEAD_TYPE("Expires:", rtsp_header_expires),
  HEAD_TYPE("From:", rtsp_header_from),
  HEAD_TYPE("If-Modified-Since:", rtsp_header_ifmod),
  HEAD_TYPE("Last-Modified:", rtsp_header_lastmod),
  HEAD_TYPE("Proxy-Authenticate:", rtsp_header_proxyauth),
  HEAD_TYPE("Proxy-Require:", rtsp_header_proxyreq),
  HEAD_TYPE("Referer:", rtsp_header_referer),
  HEAD_TYPE("Scale:", rtsp_header_scale),
  HEAD_TYPE("Server:", rtsp_header_server),
  HEAD_TYPE("Unsupported:", rtsp_header_unsup),
  HEAD_TYPE("User-Agent:", rtsp_header_uagent),
  HEAD_TYPE("Via:", rtsp_header_via),
  { NULL, 0, NULL },
};

// We don't care about the following: Cache-control, date, expires,
// Last-modified, Server, Unsupported, Via

/*
 * rtsp_decode_header - header line pointed to by lptr.  will be \0 terminated
 * Decode using above table
 */
static void rtsp_decode_header (char *lptr,
				rtsp_client_t *info,
				int *last_number)
{
  int ix;
  char *after;
  
  ix = 0;
  /*
   * go through above array, looking for a complete match
   */
  while (header_types[ix].val != NULL) {
    if (strncasecmp(lptr,
		    header_types[ix].val,
		    header_types[ix].val_length - 1) == 0) {
      after = lptr + header_types[ix].val_length;

      ADV_SPACE(after);
      /*
       * Call the correct parsing routine
       */
      (header_types[ix].parse_routine)(after, info->decode_response);
      if (header_types[ix].allow_crlf_violation != 0) {
	*last_number = ix;
      } else {
	*last_number = -1;
      }
      return;
    }
    ix++;
  }

  if (last_number >= 0) {
    //asdf
    ADV_SPACE(lptr);
    if (*lptr == ',') {
      (header_types[*last_number].parse_routine)(lptr, info->decode_response);
      return;
    }
  }
  rtsp_debug(LOG_DEBUG, "Not processing response header: %s", lptr);
}

/*
 * rtsp_parse_response - parse out response headers lines.  Figure out
 * where seperators are, then use them to determine where the body is
 */
static int rtsp_parse_response (rtsp_client_t *info)
{
  const char *seperator, *end_seperator;
  uint32_t seperator_len, end_seperator_len;
  char *beg_line, *next_line;
  bool did_status_line, illegal_response;
  rtsp_decode_t *decode;
  int ret;
  int last_header = -1;

  decode = info->decode_response;
  
  // Figure out what seperator is being used throughout the response
  end_seperator = find_end_seperator(info->recv_buff, &end_seperator_len);
  seperator_len = end_seperator_len / 2;
  seperator = end_seperator + seperator_len;
  
  next_line = info->recv_buff;
  
  rtsp_debug(LOG_DEBUG, "Response buffer:\n%s", info->recv_buff);

  did_status_line = FALSE;
  illegal_response = FALSE;
  do {
    beg_line = next_line;
    if (strncmp(beg_line, seperator, seperator_len) == 0) {
      // Have a header/body seperator
      if (did_status_line == FALSE) {
	// At beginning - shouldn't happen - just skip
	next_line += seperator_len;
	continue;
      } else {
	// Body processing
	char *resp_end;
	next_line += seperator_len;
	resp_end = info->recv_buff + info->recv_buff_used;
	if (decode->content_length != 0) {
	  if (next_line + decode->content_length > resp_end) {
	    uint32_t more_cnt;

	    more_cnt = next_line + decode->content_length - resp_end;
	    ret = rtsp_receive_more(info, more_cnt);
	    if (ret != 0) {
	      // return timeout error
	      return (-1);
	    }
	  }
	  
	  decode->body = malloc(decode->content_length + 1);
	  memcpy(decode->body, next_line, decode->content_length);
	  decode->body[decode->content_length] = '\0';
	  next_line += decode->content_length;
	} else if (decode->close_connection) {
	  if (resp_end > next_line) {
	    uint32_t len;
	    len = resp_end - next_line;
	    decode->body = malloc(1 + len);
	    memcpy(decode->body, next_line, len);
	    decode->body[len] = '\0';
	    next_line = resp_end;
	  }
	}
	//
	// Okay - need to check out the response
	//
	info->recv_buff_parsed = next_line - info->recv_buff;
	return (0);
      }
    } else {
      next_line = strstr(beg_line, seperator);
      *next_line = '\0'; // for previous line
      next_line += seperator_len;
      // We can process beg_line for header information
      if (did_status_line == FALSE) {
	did_status_line = TRUE;
	if (strncmp(beg_line, "RTSP/1.0", strlen("RTSP/1.0")) != 0) {
	  illegal_response = TRUE;
	  continue;
	}
	beg_line += strlen("RTSP/1.0");
	ADV_SPACE(beg_line);
	memcpy(decode->retcode, beg_line, 3);
	decode->retcode[3] = '\0';
	
	switch (*beg_line) {
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	  break;
	default:
	  illegal_response = TRUE;
	  continue;
	}
	beg_line += 3;
	ADV_SPACE(beg_line);
	if (beg_line != '\0') {
	  decode->retresp = strdup(beg_line);
	}
	rtsp_debug(LOG_DEBUG, "Decoded status - code %s %s",
		   decode->retcode, decode->retresp);
      } else {
	rtsp_decode_header(beg_line, info, &last_header);
      }
    }
  } while (*next_line != '\0');
  return (1);
}

/*
 * rtsp_get_response - wait for a complete response.  Interesting that
 * it could be in multiple packets, so deal with that.
 */
int rtsp_get_response (rtsp_client_t *info)
{
  int ret;
  rtsp_decode_t *decode;
  bool response_okay;

  while (1) {
    // In case we didn't get a response that we wanted.
    if (info->decode_response != NULL) {
      clear_decode_response(info->decode_response);
      decode = info->decode_response;
    } else {
      decode = info->decode_response = malloc(sizeof(rtsp_decode_t));
      if (decode == NULL) {
	rtsp_debug(LOG_ERR, "Couldn't create decode response");
	return (RTSP_RESPONSE_RECV_ERROR);
      }
    }
    
    memset(decode, 0, sizeof(rtsp_decode_t));

    // receive data
    ret = rtsp_receive(info);
    if (ret <= 0) {
      rtsp_debug(LOG_ERR, "Receive returned error %d", errno);
      return (RTSP_RESPONSE_RECV_ERROR);
    }

    do {
      // Parse response.
      ret = rtsp_parse_response(info);
      if (ret != 0) {
	rtsp_close_socket(info);
	rtsp_debug(LOG_ERR, "return code %d from rtsp_parse_response", ret);
	return (RTSP_RESPONSE_RECV_ERROR);
      }

      if (decode->close_connection) {
	rtsp_debug(LOG_DEBUG, "Response requested connection close");
	rtsp_close_socket(info);
      }
  
      response_okay = TRUE;
      // Okay - we have a good response. - check the cseq, and return
      // the correct error code.
      if (info->next_cseq == decode->cseq) {
	info->next_cseq++;
	if ((decode->retcode[0] == '4') ||
	    (decode->retcode[0] == '5')) {
	  return (RTSP_RESPONSE_BAD);
	}
	if (decode->cookie != NULL) {
	  // move cookie to rtsp_client
	  info->cookie = strdup(decode->cookie);
	}
	if (decode->retcode[0] == '2') {
	  return (RTSP_RESPONSE_GOOD);
	}
	if (decode->retcode[0] == '3') {
	  if (decode->location == NULL) {
	    return (RTSP_RESPONSE_BAD);
	  }
	  if (rtsp_setup_redirect(info) != 0) {
	    return (RTSP_RESPONSE_BAD);
	  }
	  return (RTSP_RESPONSE_REDIRECT);
	}
      }
      if (info->recv_buff_parsed != info->recv_buff_used) {
	// We have more in the buffer - see if it's what
	// we need
	memmove(info->recv_buff, &info->recv_buff[info->recv_buff_parsed],
		info->recv_buff_used - info->recv_buff_parsed);
	info->recv_buff_used -= info->recv_buff_parsed;
	info->recv_buff_parsed = 0;
	rtsp_debug(LOG_DEBUG, "More in buffer - working from %s",
		   info->recv_buff);
	// clear out last response
	clear_decode_response(decode);
	memset(decode, 0, sizeof(rtsp_decode_t));
      }
    } while (info->recv_buff_parsed != info->recv_buff_used);
  } 
  return (RTSP_RESPONSE_RECV_ERROR);
}

/* end file rtsp_resp.c */
