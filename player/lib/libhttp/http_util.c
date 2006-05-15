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
 * http_util.c - http utilities.
 */
#include "mpeg4ip.h"
#include <time.h>
#include "http_private.h"

/*
 * http_disect_url
 * Carve URL up into portions that we can use - store them in the
 * client structure.  url points after http:://
 * We're looking for m_host (destination name), m_port (destination port)
 * and m_resource (location of file on m_host - also called path)
 */
static int http_dissect_url (const char *name,
			    http_client_t *cptr)
{
  // Assume name points at host name
  const char *uptr = name;
  const char *nextslash, *nextcolon, *rightbracket;
  char *host;
  size_t hostlen;

  // skip ahead after host
  rightbracket = NULL;
  if (*uptr == '[') {
    rightbracket = strchr(uptr, ']');
    if (rightbracket != NULL) {
      uptr++;
      // literal IPv6 address
      if (rightbracket[1] == ':') {
	nextcolon = rightbracket + 1;
      } else
	nextcolon = NULL;
      nextslash = strchr(rightbracket, '/');
    } else {
      return -1;
    }
  } else {
    nextslash = strchr(uptr, '/');
    nextcolon = strchr(uptr, ':');
  }

  cptr->m_port = 80;
  if (nextslash != NULL || nextcolon != NULL) {
    if (nextcolon != NULL &&
	(nextcolon < nextslash || nextslash == NULL)) {
      hostlen = nextcolon - uptr;
      // have a port number
      nextcolon++;
      cptr->m_port = 0;
      while (isdigit(*nextcolon)) {
	cptr->m_port *= 10;
	cptr->m_port += *nextcolon - '0';
	nextcolon++;
      }
      if (cptr->m_port == 0 || (*nextcolon != '/' && *nextcolon != '\0')) {
	return (-1);
      }
    } else {
      // no port number
      hostlen = nextslash - uptr;
	
    }
    if (hostlen == 0) {
      return (-1);
    }
    FREE_CHECK(cptr, m_host);
    if (rightbracket != NULL) hostlen--;
    host = malloc(hostlen + 1);
    if (host == NULL) {
      return (-1);
    }
    memcpy(host, uptr, hostlen);
    host[hostlen] = '\0';
    cptr->m_host = host;
  } else {
    if (*uptr == '\0') {
      return (EINVAL);
    }
    FREE_CHECK(cptr, m_host);
    host = strdup(uptr);
    if (rightbracket != NULL) {
       host[strlen(host) - 1] = '\0';
    }
    cptr->m_host = host;
  }
  
  FREE_CHECK(cptr, m_content_location);
  if (nextslash != NULL) {
    cptr->m_content_location = strdup(nextslash);
  } else {
    cptr->m_content_location = strdup("/");
  }
  http_debug(LOG_DEBUG, "content location is %s", cptr->m_content_location);
  return (0);
}
  
/*
 * http_decode_and_connect_url
 * decode the url, and connect it.  If we were already connected,
 * disconnect the socket and move forward
 */
int http_decode_and_connect_url (const char *name,
				 http_client_t *cptr)
{
  int check_open;
  uint16_t port;
  const char *old_host;
  struct hostent *host;
  struct sockaddr_in sockaddr;
  int result;
  
  if (strncasecmp(name, "http://", strlen("http://")) != 0) {
    return (-1);
  }
  name += strlen("http://");

  check_open = 0;
  port = 80;
  old_host = NULL;
  if (cptr->m_state == HTTP_STATE_CONNECTED) {
    check_open = 1;
    port = cptr->m_port;
    old_host = cptr->m_host;
    cptr->m_host = NULL; // don't inadvertantly free it
  }

  if (http_dissect_url(name, cptr) < 0) {
    // If there's an error - nothing's changed
    return (-1);
  }

  if (check_open) {
    // See if the existing host matches the new one
    int match = 0;
    // Compare strings, first
    if (strcasecmp(old_host, cptr->m_host) == 0) {
      // match
      if (port == cptr->m_port) {
	match = 1;
      }
    } else {
      // Might be same - resolve new address and compare
      host = gethostbyname(cptr->m_host);
      if (host == NULL) {
#ifdef _WIN32
		  return -1;
#else
	if (h_errno > 0) h_errno = 0 - h_errno;
	return (h_errno);
#endif
      }
      if (memcmp(host->h_addr,
		 &cptr->m_server_addr,
		 sizeof(struct in_addr)) == 0 &&
	  (port == cptr->m_port)) {
	match = 1;
      } else {
	cptr->m_server_addr = *(struct in_addr *)host->h_addr;
      }
    }
    free((void *)old_host); // free off the old one we saved
    if (match == 0) {
      cptr->m_state = HTTP_STATE_CLOSED;
      closesocket(cptr->m_server_socket);
      cptr->m_server_socket = -1;
    } else {
      // keep using the same socket...
      return 0;
    }
    
  } else {
    // No existing connection - get the new address.
    host = gethostbyname(cptr->m_host);
    if (host == NULL) {
#ifdef _WIN32
		return -1;
#else
      if (h_errno > 0) h_errno = 0 - h_errno;
      return (h_errno);
#endif
    }
    cptr->m_server_addr = *(struct in_addr *)host->h_addr;
  }

  // Create and connect the socket
  cptr->m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (cptr->m_server_socket == -1) {
    return (-1);
  }
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(cptr->m_port);
  sockaddr.sin_addr = cptr->m_server_addr;

  result = connect(cptr->m_server_socket,
		   (struct sockaddr *)&sockaddr,
		   sizeof(sockaddr));

  if (result == -1) {
    return (-1);
  }
  cptr->m_state = HTTP_STATE_CONNECTED;
  return (0);
}

static const char *user_agent = "Mpeg4ip http library 0.1";

/*
 * http_build_header - create a header string
 * Will eventually want to expand this if we want to specify
 * content type, etc.
 */
int http_build_header (char *buffer,
		       uint32_t maxlen,
		       uint32_t *at,
		       http_client_t *cptr,
		       const char *method,
		       const char *add_header,
		       char *content_body)
{
  int ret;
#define SNPRINTF_CHECK(fmt, value) \
  ret = snprintf(buffer + *at, maxlen - *at, (fmt), value); \
  if (ret == -1) { \
    return (-1); \
  }\
  *at += ret;

  SNPRINTF_CHECK("%s ", method);
  if (cptr->m_content_location != NULL && 
      (strcmp(cptr->m_content_location, "/") != 0 ||
       *cptr->m_resource != '/')) {
    SNPRINTF_CHECK("%s", cptr->m_content_location);
  } 
  SNPRINTF_CHECK("%s HTTP/1.1\r\n",
		 cptr->m_resource);
  SNPRINTF_CHECK("Host: %s\r\n",
		 cptr->m_host);
  SNPRINTF_CHECK("User-Agent: %s\r\n", user_agent);
  SNPRINTF_CHECK("Connection: Keep-Alive%s", "\r\n");
  if (add_header != NULL) {
    SNPRINTF_CHECK("%s\r\n", add_header);
  }
  if (content_body != NULL) {
    int len = strlen(content_body);
    SNPRINTF_CHECK("Content-length: %d\r\n\r\n", len);
    SNPRINTF_CHECK("%s", content_body);
  } else {
    SNPRINTF_CHECK("%s", "\r\n");
  }
#undef SNPRINTF_CHECK
  return (ret);
}

/*
 * Logging code
 */
static int http_debug_level = LOG_ERR;
static error_msg_func_t error_msg_func = NULL;

void http_set_loglevel (int loglevel)
{
  http_debug_level = loglevel;
}

void http_set_error_func (error_msg_func_t func)
{
  error_msg_func = func;
}

void http_debug (int loglevel, const char *fmt, ...)
{
  va_list ap;
  if (loglevel <= http_debug_level) {
    va_start(ap, fmt);
    if (error_msg_func != NULL) {
      (error_msg_func)(loglevel, "libhttp", fmt, ap);
    } else {
#if _WIN32 && _DEBUG
	  char msg[1024];

      _vsnprintf(msg, 1024, fmt, ap);
      OutputDebugString(msg);
      OutputDebugString("\n");
#else
      struct timeval thistime;
      struct tm thistm;
      char buffer[80];
      time_t secs;

      gettimeofday(&thistime, NULL);
      // To add date, add %a %b %d to strftime
      secs = thistime.tv_sec;
      localtime_r(&secs, &thistm);
      strftime(buffer, sizeof(buffer), "%X", &thistm);
      printf("%s.%03ld-libhttp-%d: ",
	     buffer, (unsigned long)thistime.tv_usec / 1000, loglevel);
      vprintf(fmt, ap);
      printf("\n");
#endif
    }
    va_end(ap);
  }
}

