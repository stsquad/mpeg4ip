#ifndef __MPEG4IP_HTTP_H__
#define __MPEG4IP_HTTP_H__ 1

typedef struct http_client_ http_client_t;

typedef struct http_resp_t {
  int ret_code;
  char *body;
  uint32_t body_len;
} http_resp_t;

#ifdef __cplusplus
extern "C" {
#endif

  http_client_t *http_init_connection(const char *url);
  void http_free_connection(http_client_t *handle);

  int http_get(http_client_t *, const char *url, http_resp_t **resp);
  void http_resp_free(http_resp_t *);
  
#ifdef __cplusplus
}
#endif
#endif
