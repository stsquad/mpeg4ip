#include "systems.h"
#include "http.h"

int main (int argc, char **argv)
{
  http_client_t *http_client;
  http_resp_t *http_resp;
  argv++;
  
  http_client = http_init_connection(*argv);
  if (http_client == NULL) {
    printf("no client\n");
    return (1);
  }

  http_resp = NULL;
  http_get(http_client, NULL, &http_resp);
  http_resp_free(http_resp);
  http_free_connection(http_client);
  return (0);
}

