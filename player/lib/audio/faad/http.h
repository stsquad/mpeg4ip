void *http_file_open(char *url);
int http_file_read(void *hInet, char *buffer, int length);
unsigned long http_file_length(void *hInet);
void http_file_close(void *hInet);