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
 * http_resp.c - read and decode http response.
 */
#include "mpeg4ip.h"
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
#include "http_private.h"

/*
 * http_recv - receive up to len bytes in the buffer
 */
static int http_recv (int server_socket,
		      char *buffer,
		      uint32_t len)
{
  int ret;
#ifdef HAVE_POLL
  struct pollfd pollit;

  pollit.fd = server_socket;
  pollit.events = POLLIN | POLLPRI;
  pollit.revents = 0;

  ret = poll(&pollit, 1, 2000);
#else
  fd_set read_set;
  struct timeval timeout;
  FD_ZERO(&read_set);
  FD_SET(server_socket, &read_set);
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  ret = select(server_socket + 1, &read_set, NULL, NULL, &timeout);
#endif
  if (ret <= 0) {
    return -1;
  }

  ret = recv(server_socket, buffer, len, 0);
  http_debug(LOG_DEBUG, "Return from recv is %d", ret);
  return ret;
}

/*
 * http_read_into_buffer - read bytes into buffer at the buffer offset
 * to the end of the buffer.
 */
static int http_read_into_buffer (http_client_t *cptr,
			   uint32_t buffer_offset)
{
  int ret;

  ret = http_recv(cptr->m_server_socket,
		  cptr->m_resp_buffer + buffer_offset,
		  RESP_BUF_SIZE - buffer_offset);

  if (ret <= 0) return (ret);

  cptr->m_buffer_len = buffer_offset + ret;

  return ret;
}

/*
 * http_get_next_line()
 * Use the existing buffers that we've read, and try to get a coherent
 * line.  This saves having to do a bunch of 1 byte reads looking for a
 * cr/lf - instead, we read as many bytes as we can into the buffer, and
 * then process it.
 */
static const char *http_get_next_line (http_client_t *cptr)
{
  int ret;
  uint32_t ix;
  int last_on;

  /*
   * If we don't have any data, try to read a buffer full
   */
  if (cptr->m_buffer_len <= 0) {
    cptr->m_offset_on = 0;
    ret = http_read_into_buffer(cptr, 0);
    if (ret <= 0) {
      return (NULL);
    }
  }

  /*
   * Look for CR/LF in the buffer.  If we find it, NULL terminate at
   * the CR, then set the buffer values to look past the crlf
   */
  for (ix = cptr->m_offset_on + 1;
       ix < cptr->m_buffer_len;
       ix++) {
    if (cptr->m_resp_buffer[ix] == '\n' &&
	cptr->m_resp_buffer[ix - 1] == '\r') {
      const char *retval = &cptr->m_resp_buffer[cptr->m_offset_on];
      cptr->m_offset_on = ix + 1;
      cptr->m_resp_buffer[ix - 1] = '\0'; // make it easy
      return (retval);
    }
  }

  if (cptr->m_offset_on == 0) {
    return (NULL);
  }

  /*
   * We don't have a line.  So, move the data down in the buffer, then
   * read into the rest of the buffer
   */
  cptr->m_buffer_len -= cptr->m_offset_on;
  if (cptr->m_buffer_len != 0) {
    memmove(cptr->m_resp_buffer,
	    &cptr->m_resp_buffer[cptr->m_offset_on],
	    cptr->m_buffer_len);
    last_on = cptr->m_buffer_len;
  } else {
    last_on = 1;
  }
  cptr->m_offset_on = 0;
  
  ret = http_read_into_buffer(cptr, cptr->m_buffer_len);
  if (ret <= 0) {
    return (NULL);
  }

  /*
   * Continue searching through the buffer.  If we get this far, we've
   * received like 2K or 4K without a CRLF - most likely a bad response
   */
  for (ix = last_on;
       ix < cptr->m_buffer_len;
       ix++) {
    if (cptr->m_resp_buffer[ix] == '\n' &&
	cptr->m_resp_buffer[ix - 1] == '\r') {
      const char *retval = &cptr->m_resp_buffer[cptr->m_offset_on];
      cptr->m_offset_on = ix + 1;
      cptr->m_resp_buffer[ix - 1] = '\0'; // make it easy
      return (retval);
    }
  }
  return (NULL);
}

/****************************************************************************
 * HTTP header decoding
 ****************************************************************************/

#define HTTP_CMD_DECODE_FUNC(a) static void a(const char *lptr, http_client_t *cptr)

/*
 * Connection: header
 */
HTTP_CMD_DECODE_FUNC(http_cmd_connection)
{
  // connection can be comma seperated list.
  while (*lptr != '\0') {
    if (strncasecmp(lptr, "close", strlen("close")) == 0) {
      cptr->m_connection_close = 1;
      return;
    } else {
      while (*lptr != '\0' && *lptr != ',') lptr++;
    }
  }
}

/*
 * Content-length: header
 */
HTTP_CMD_DECODE_FUNC(http_cmd_content_length)
{
  cptr->m_content_len = 0;
  while (isdigit(*lptr)) {
    cptr->m_content_len_received = 1;
    cptr->m_content_len *= 10;
    cptr->m_content_len += *lptr - '0';
    lptr++;
  }
}

/*
 * Content-type: header
 */
HTTP_CMD_DECODE_FUNC(http_cmd_content_type)
{
  FREE_CHECK(cptr->m_resp, content_type);
  cptr->m_resp->content_type = strdup(lptr);
}

/*
 * Location: header
 */
HTTP_CMD_DECODE_FUNC(http_cmd_location)
{
  FREE_CHECK(cptr, m_redir_location);
  cptr->m_redir_location = strdup(lptr);
}

/*
 * Transfer-Encoding: header
 */
HTTP_CMD_DECODE_FUNC(http_cmd_transfer_encoding)
{
  do {
    if (strncasecmp(lptr, "chunked", strlen("chunked")) == 0) {
      cptr->m_transfer_encoding_chunked = 1;
      return;
    }
    while ((*lptr != '\0') && (*lptr != ';')) lptr++;
    ADV_SPACE(lptr);
  } while (*lptr != '\0');
}

static struct {
  const char *val;
  uint32_t val_length;
  void (*parse_routine)(const char *lptr, http_client_t *cptr);
} header_types[] =
{
#define HEAD_TYPE(a, b) { a, sizeof(a), b }
  HEAD_TYPE("connection", http_cmd_connection),
  HEAD_TYPE("content-length", http_cmd_content_length),
  HEAD_TYPE("content-type", http_cmd_content_type),
  HEAD_TYPE("location", http_cmd_location),
  HEAD_TYPE("transfer-encoding", http_cmd_transfer_encoding),
  {NULL, 0, NULL },
};

static void http_decode_header (http_client_t *cptr, const char *lptr)
{
  int ix;
  const char *after;

  ix = 0;
  ADV_SPACE(lptr);
  
  while (header_types[ix].val != NULL) {
    if (strncasecmp(lptr,
		    header_types[ix].val,
		    header_types[ix].val_length - 1) == 0) {
      after = lptr + header_types[ix].val_length - 1;
      ADV_SPACE(after);
      if (*after == ':') {
	after++;
	ADV_SPACE(after);
	/*
	 * Call the correct parsing routine
	 */
	(header_types[ix].parse_routine)(after, cptr);
	return;
      }
    }
    ix++;
  }

  //rtsp_debug(LOG_DEBUG, "Not processing response header: %s", lptr);
}

static uint32_t to_hex (const char **hex_string)
{
  const char *p = *hex_string;
  uint32_t ret = 0;
  while (isxdigit(*p)) {
    ret *= 16;
    if (isdigit(*p)) {
      ret += *p - '0';
    } else {
      ret += tolower(*p) - 'a' + 10;
    }
    p++;
  }
  *hex_string = p;
  return (ret);
}

/*
 * http_get_response - get the response, process it, and fill in the response
 * structure.
 */
int http_get_response (http_client_t *cptr,
		       http_resp_t **resp)
{
  const char *p;
  int resp_code;
  int ix;
  int done;
  uint32_t len;
  int ret;

  /*
   * Clear out old response header
   */
  if (*resp != NULL) {
    cptr->m_resp = *resp;
    http_resp_clear(*resp);
  } else {
    cptr->m_resp = (http_resp_t *)malloc(sizeof(http_resp_t));
    memset(cptr->m_resp, 0, sizeof(http_resp_t));
    *resp = cptr->m_resp;
  }

  /*
   * Reset all relevent variables
   */
  FREE_CHECK(cptr, m_redir_location);
  cptr->m_connection_close = 0;
  cptr->m_content_len_received = 0;
  cptr->m_offset_on = 0;
  cptr->m_buffer_len = 0;
  cptr->m_transfer_encoding_chunked = 0;

  /*
   * Get the first line and process it
   */
  p = http_get_next_line(cptr);
  if (p == NULL) {
    http_debug(LOG_INFO, "did not get first line");
    return (-1);
  }

  /*
   * http/version.version processing
   */
  ADV_SPACE(p);
  if (*p == '\0' || strncasecmp(p, "http/", strlen("http/")) != 0) {
    http_debug(LOG_INFO, "first line did not start with HTTP/");
    return (-1);
  }
  p += strlen("http/");
  ADV_SPACE(p);
  while (*p != '\0' && isdigit(*p)) p++;
  if (*p++ != '.') return (-1);
  while (*p != '\0' && isdigit(*p)) p++;
  if (*p++ == '\0') return (-1);
  ADV_SPACE(p);

  /*
   * error code processing - 200 is gold
   */
  resp_code = 0;
  for (ix = 0; ix < 3; ix++) {
    if (isdigit(*p)) {
      resp_code *= 10;
      resp_code += *p++ - '0';
    } else {
      http_debug(LOG_ERR, "did not get 3-digit response code");
      return (-1);
    }
  }
  (*resp)->ret_code = resp_code;
  ADV_SPACE(p);
  if (*p != '\0') {
    (*resp)->resp_phrase = strdup(p);
  }

  /*
   * Now begin processing the headers
   */
  done = 0;
  do {
    p = http_get_next_line(cptr);
    if (p == NULL) {
      return (-1);
    }
    if (*p == '\0') {
      done = 1;
    } else {
      http_debug(LOG_DEBUG, "%s", p);
      // we have a header.  See if we want to process it...
      http_decode_header(cptr, p);
    }
  } while (done == 0);

  // Okay - at this point - we have the headers done.  Let's
  // read the body
  if (cptr->m_content_len_received != 0) {
    /*
     * We have content-length - read that many bytes.
     */
    if (cptr->m_content_len != 0) {
      cptr->m_resp->body = (char *)malloc(cptr->m_content_len + 1);
      len = cptr->m_buffer_len - cptr->m_offset_on;
      memcpy(cptr->m_resp->body,
	     &cptr->m_resp_buffer[cptr->m_offset_on],
	     MIN(len, cptr->m_content_len));
      while (len < cptr->m_content_len) {
	ret = http_read_into_buffer(cptr, 0);
	if (ret <= 0) {
	  return (-1);
	} 
	memcpy(cptr->m_resp->body + len,
	       cptr->m_resp_buffer,
	       MIN(cptr->m_content_len - len, cptr->m_buffer_len));
	len += cptr->m_buffer_len;
      }
      cptr->m_resp->body[cptr->m_content_len] = '\0';
      cptr->m_resp->body_len = cptr->m_content_len;
    }
  } else if (cptr->m_transfer_encoding_chunked != 0) {
    /*
     * Chunk encoded - size in hex, body, size in hex, body, 0
     */
    // read a line,
    uint32_t te_size;
    p = http_get_next_line(cptr);
    if (p == NULL) {
      http_debug(LOG_ALERT, "no chunk size reading chunk transitions");
      return (-1);
    }
    te_size = to_hex(&p);
    cptr->m_resp->body = NULL;
    cptr->m_resp->body_len = 0;
    /*
     * Read a te_size chunk of bytes, read CRLF at end of that many bytes,
     * read next size
     */
    while (te_size != 0) {
      http_debug(LOG_DEBUG, "Chunk size %d", te_size);
      cptr->m_resp->body = (char *)realloc(cptr->m_resp->body,
					   cptr->m_resp->body_len + te_size + 1);
      len = MIN(te_size, cptr->m_buffer_len - cptr->m_offset_on);
      memcpy(cptr->m_resp->body + cptr->m_resp->body_len,
	     &cptr->m_resp_buffer[cptr->m_offset_on],
	     len);
      cptr->m_offset_on += len;
      cptr->m_resp->body_len += len;
      http_debug(LOG_DEBUG, "chunk - copied %d from rest of buffer(%d)",
		 len, cptr->m_resp->body_len);
      while (len < te_size) {
	int ret;
	ret = http_recv(cptr->m_server_socket,
			cptr->m_resp->body + cptr->m_resp->body_len,
			te_size - len);
	if (ret <= 0) return (-1);
	len += ret;
	cptr->m_resp->body_len += ret;
	http_debug(LOG_DEBUG, "chunk - recved %d bytes (%d)",
		   ret, cptr->m_resp->body_len);
      }
      p = http_get_next_line(cptr); // should read CRLF at end
      if (p == NULL || *p != '\0') {
	http_debug(LOG_ALERT, "Http chunk reader - should be CRLF at end of chunk, is %s", p);
	return (-1);
      }
      p = http_get_next_line(cptr); // read next size
      if (p == NULL) {
	http_debug(LOG_ALERT,"No chunk size after first");
	return (-1);
      }
      te_size = to_hex(&p);
    }
    cptr->m_resp->body[cptr->m_resp->body_len] = '\0'; // null terminate
	
  } else {
    // No termination - just keep reading, I guess...
    len = cptr->m_buffer_len - cptr->m_offset_on;
    cptr->m_resp->body = (char *)malloc(len + 1);
    cptr->m_resp->body_len = len;
    memcpy(cptr->m_resp->body,
	   &cptr->m_resp_buffer[cptr->m_offset_on],
	   len);
    http_debug(LOG_INFO, "Len bytes copied - %d", len);
    while (http_read_into_buffer(cptr, 0) > 0) {
      char *temp;
      len = cptr->m_resp->body_len + cptr->m_buffer_len;
      temp = realloc(cptr->m_resp->body, len + 1);
      if (temp == NULL) {
	return -1;
      }
      cptr->m_resp->body = temp;
      memcpy(&cptr->m_resp->body[cptr->m_resp->body_len],
	     cptr->m_resp_buffer,
	     cptr->m_buffer_len);
      cptr->m_resp->body_len = len;
    http_debug(LOG_INFO, "Len bytes added - %d", len);
    }
    cptr->m_resp->body[cptr->m_resp->body_len] = '\0';
      
  }
  return (0);
}

/*
 * http_resp_clear - clean out http_resp_t structure
 */
void http_resp_clear (http_resp_t *rptr)
{
  FREE_CHECK(rptr, body);
  FREE_CHECK(rptr, content_type);
  FREE_CHECK(rptr, resp_phrase);
}

/*
 * http_resp_free - clean out http_resp_t and free
 */
void http_resp_free (http_resp_t *rptr)
{
  if (rptr != NULL) {
    http_resp_clear(rptr);
    free(rptr);
  }
}
