#include "systems.h"
#include "http_private.h"


http_client_t *http_init_connection (const char *name)
{
  http_client_t *ptr;

  ptr = (http_client_t *)malloc(sizeof(http_client_t));
  if (ptr == NULL) {
    return (NULL);
  }

  memset(ptr, 0, sizeof(http_client_t));
  ptr->m_state = HTTP_STATE_INIT;
  if (http_decode_and_connect_url(name, ptr) < 0) {
    http_free_connection(ptr);
    return (NULL);
  }
  return (ptr);
}

void http_free_connection (http_client_t *ptr)
{
  if (ptr->m_state == HTTP_STATE_CONNECTED) {
    closesocket(ptr->m_server_socket);
    ptr->m_server_socket = -1;
  }
  FREE_CHECK(ptr, m_orig_url);
  FREE_CHECK(ptr, m_current_url);
  FREE_CHECK(ptr, m_host);
  FREE_CHECK(ptr, m_resource);
  FREE_CHECK(ptr, m_redir_location);
  free(ptr);
}

int http_get (http_client_t *cptr,
	      const char *url,
	      http_resp_t **resp)
{
  char header_buffer[4096];
  uint32_t buffer_len;
  int ret;
  int more;
  
  if (cptr == NULL)
    return (-1);
  
  if (*resp != NULL) {
    http_resp_clear(*resp);
  }
  buffer_len = 0;
  ret = http_build_header(header_buffer, 4096, &buffer_len, cptr, "GET");
  http_debug(LOG_DEBUG, header_buffer);
  if (send(cptr->m_server_socket,
	   header_buffer,
	   buffer_len,
	   0) < 0) {
    http_debug(LOG_ERR,"Send failure");
    return (-1);
  }
  cptr->m_redirect_count = 0;
  more = 0;
  do {
    ret = http_get_response(cptr, resp);
    http_debug(LOG_INFO, "Response %d", (*resp)->ret_code);
    http_debug(LOG_INFO, (*resp)->body);
    if (ret < 0) return (ret);
    switch ((*resp)->ret_code / 100) {
    default:
    case 1:
      more = 0;
      break;
    case 2:
      return (1);
    case 3:
      cptr->m_redirect_count++;
      if (cptr->m_redirect_count > 5) {
	return (-1);
      }
      if (http_decode_and_connect_url(cptr->m_redir_location, cptr) < 0) {
	http_debug(LOG_ERR, "Couldn't reup location %s", cptr->m_redir_location);
	return (-1);
      }
      buffer_len = 0;
      ret = http_build_header(header_buffer, 4096, &buffer_len, cptr, "GET");
      http_debug(LOG_DEBUG, header_buffer);
      if (send(cptr->m_server_socket,
	       header_buffer,
	       buffer_len,
	       0) < 0) {
	http_debug(LOG_ERR,"Send failure");
	return (-1);
      }
      
      break;
    case 4:
    case 5:
      return (0);
    }
  } while (more == 0);
  return (ret);
}
