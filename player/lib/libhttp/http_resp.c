#include "systems.h"
#ifndef USE_SELECT
#include <sys/poll.h>
#endif
#include "http_private.h"

static int http_get_chunk (http_client_t *cptr,
			   uint32_t buffer_offset)
{
  int ret;
#ifndef USE_SELECT
  struct pollfd pollit;

  pollit.fd = cptr->m_server_socket;
  pollit.events = POLLIN | POLLPRI;
  pollit.revents = 0;

  ret = poll(&pollit, 1, 2000);
#else
  fd_set read_set;
  struct timeval timeout;
  FD_ZERO(&read_set);
  FD_SET(cptr->m_server_socket, &read_set);
  timeout.tv_sec = 2;
  timeout.tx_usec = 0;
  ret = select(1, &read_set, NULL, NULL, &timeout);
#endif
  if (ret <= 0) {
    return -1;
  }

  ret = recv(cptr->m_server_socket,
	     cptr->m_resp_buffer + buffer_offset,
	     RESP_BUF_SIZE - buffer_offset,
	     0);
  if (ret <= 0)
    return -1;
  cptr->m_buffer_len = buffer_offset + ret;
  return 0;
}

static const char *http_get_next_line (http_client_t *cptr)
{
  int ret;
  uint32_t ix;
  int last_on;
  
  if (cptr->m_buffer_len <= 0) {
    cptr->m_offset_on = 0;
    ret = http_get_chunk(cptr, 0);
    if (ret < 0) {
      return (NULL);
    }
  }

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

  cptr->m_buffer_len -= cptr->m_offset_on;
  memmove(cptr->m_resp_buffer,
	  &cptr->m_resp_buffer[cptr->m_offset_on],
	  cptr->m_buffer_len);
  cptr->m_offset_on = 0;
  last_on = cptr->m_buffer_len;
  
  ret = http_get_chunk(cptr, cptr->m_buffer_len);
  if (ret < 0) {
    return (NULL);
  }

  for (ix = last_on;
       ix < cptr->m_buffer_len;
       ix++) {
    if (cptr->m_resp_buffer[ix] == '\r' &&
	cptr->m_resp_buffer[ix - 1] == '\n') {
      const char *retval = &cptr->m_resp_buffer[cptr->m_offset_on];
      cptr->m_offset_on = ix + 1;
      cptr->m_resp_buffer[ix - 1] = '\0'; // make it easy
      return (retval);
    }
  }
  return (NULL);
}

#define HTTP_CMD_DECODE_FUNC(a) static void a(const char *lptr, http_client_t *cptr)

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

HTTP_CMD_DECODE_FUNC(http_cmd_location)
{
  FREE_CHECK(cptr, m_redir_location);
  cptr->m_redir_location = strdup(lptr);
}

HTTP_CMD_DECODE_FUNC(http_cmd_transfer_encoding)
{
  
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

int http_get_response (http_client_t *cptr,
		       http_resp_t **resp)
{
  const char *p;
  int resp_code;
  char *resp_phrase;
  int ix;
  int done;
  uint32_t len;
      int ret;

  if (*resp != NULL) {
    cptr->m_resp = *resp;
    http_resp_clear(*resp);
  } else {
    cptr->m_resp = (http_resp_t *)malloc(sizeof(http_resp_t));
    memset(cptr->m_resp, 0, sizeof(http_resp_t));
    *resp = cptr->m_resp;
  }
  FREE_CHECK(cptr, m_redir_location);
  cptr->m_connection_close = 0;
  cptr->m_content_len_received = 0;
  cptr->m_offset_on = 0;
  cptr->m_buffer_len = 0;
  cptr->m_transfer_encoding_chunked = 0;
  resp_phrase = NULL;
  
  p = http_get_next_line(cptr);
  if (p == NULL) {
    return (-1);
  }
  
  ADV_SPACE(p);
  if (*p == '\0' || strncasecmp(p, "http/", strlen("http/")) != 0) {
    return (-1);
  }
  p += strlen("http/");
  ADV_SPACE(p);
  while (*p != '\0' && isdigit(*p)) p++;
  if (*p++ != '.') return (-1);
  while (*p != '\0' && isdigit(*p)) p++;
  if (*p++ == '\0') return (-1);
  ADV_SPACE(p);
  // pointing at error code...
  resp_code = 0;
  for (ix = 0; ix < 3; ix++) {
    if (isdigit(*p)) {
      resp_code *= 10;
      resp_code += *p++ - '0';
    } else {
      return (-1);
    }
  }
  (*resp)->ret_code = resp_code;
  ADV_SPACE(p);
  if (*p != '\0') {
    resp_phrase = strdup(p);
  }

  done = 0;
  do {
    p = http_get_next_line(cptr);
    if (p == NULL) {
      return (-1);
    }
    if (*p == '\0') {
      done = 1;
    } else {
      http_debug(LOG_DEBUG, p);
      // we have a header.  See if we want to process it...
      http_decode_header(cptr, p);
    }
  } while (done == 0);
  // Okay - at this point - we have the headers done.  Let's
  // read the body

  if (cptr->m_content_len_received != 0) {
    if (cptr->m_content_len != 0) {
      cptr->m_resp->body = (char *)malloc(cptr->m_content_len + 1);
      len = cptr->m_buffer_len - cptr->m_offset_on;
      memcpy(cptr->m_resp->body,
	     &cptr->m_resp_buffer[cptr->m_offset_on],
	     MIN(len, cptr->m_content_len));
      while (len < cptr->m_content_len) {
	ret = http_get_chunk(cptr, 0);
	if (ret != 0) {
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
  } else {
    // No termination - just keep reading, I guess...
    len = cptr->m_buffer_len - cptr->m_offset_on;
    cptr->m_resp->body = (char *)malloc(len + 1);
    cptr->m_resp->body_len = len;
    memcpy(cptr->m_resp->body,
	   &cptr->m_resp_buffer[cptr->m_offset_on],
	   len);
    http_debug(LOG_INFO, "Len bytes copied - %d", len);
    while (http_get_chunk(cptr, 0) == 0) {
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
  
void http_resp_clear (http_resp_t *rptr)
{
  FREE_CHECK(rptr, body);
}

void http_resp_free (http_resp_t *rptr)
{
  if (rptr != NULL) {
    http_resp_clear(rptr);
    free(rptr);
  }
}
