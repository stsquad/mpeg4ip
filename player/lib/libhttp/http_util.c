#include "systems.h"
#ifndef _WINDOWS
#include <time.h>
#include <sys/time.h>
#endif

#include "http_private.h"

static int http_disect_url (const char *name,
			    http_client_t *cptr)
{
  // Assume name points at host name
  const char *p = name;
  char *host;
  size_t len;
  uint16_t port;
  
  while (*p != '\0' && *p != ':' && *p != '/') p++;

  if (p == name) return (-1);
  len = p - name;
  host = (char *) malloc(len + 1);
  memcpy(host, name, len);
  host[len] = '\0';

  cptr->m_port = 80;
  if (*p == ':') {
    // port number
    p++;
    port = 0;
    while (*p != '/' && *p != '\0') {
      if (!(isdigit(*p))) {
	return (-1);
      }
      port *= 10;
      port += *p - '0';
      p++;
    }
    if (port == 0) {
      cptr->m_port = 80;
    } else {
      cptr->m_port = port;
    }
  } 

  FREE_CHECK(cptr, m_host);
  cptr->m_host = host;

  FREE_CHECK(cptr, m_resource);
  if (*p == '\0') {
    cptr->m_resource = strdup("/");
  } else {
    cptr->m_resource = strdup(p);
  }
  return (0);
}
  

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

  if (http_disect_url(name, cptr) < 0) {
    // If there's an error - nothing's changed
    return (-1);
  }

  if (check_open) {
    int match = 0;
    if (strcasecmp(old_host, cptr->m_host) == 0) {
      // match
      if (port == cptr->m_port) {
	match = 1;
      }
    } else {
      // Might be same - resolve new address and compare
      host = gethostbyname(cptr->m_host);
      if (host == NULL) {
	return (h_errno);
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
    host = gethostbyname(cptr->m_host);
    if (host == NULL) {
      return (h_errno);
    }
    cptr->m_server_addr = *(struct in_addr *)host->h_addr;
  }

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

int http_build_header (char *buffer,
		       uint32_t maxlen,
		       uint32_t *at,
		       http_client_t *cptr,
		       const char *method)
{
  int ret;
#define SNPRINTF_CHECK(fmt, value) \
  ret = snprintf(buffer + *at, maxlen - *at, (fmt), (value)); \
  if (ret == -1) { \
    return (-1); \
  }\
  *at += ret;

  ret = snprintf(buffer,
		 maxlen,
		 "%s %s HTTP/1.1\r\nHost: %s\r\n",
		 method,
		 cptr->m_resource,
		 cptr->m_host);
  if (ret == -1) return -1;
  *at += ret;
  SNPRINTF_CHECK("User-Agent: %s\r\n", user_agent);
  SNPRINTF_CHECK("%s", "\r\n");
#undef SNPRINTF_CHECK
  return (ret);
}

void http_debug (int loglevel, const char *fmt, ...)
{
  va_list ap;
  struct timeval thistime;
  char buffer[80];

  gettimeofday(&thistime, NULL);
  // To add date, add %a %b %d to strftime
  strftime(buffer, sizeof(buffer), "%X", localtime(&thistime.tv_sec));
  printf("%s.%03ld-libhttp-%d: ", buffer, thistime.tv_usec / 1000, loglevel);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}
