#include <windows.h>
#include <wininet.h>
#include <malloc.h>

HINTERNET hInetFAAD;

void *http_file_open(char *url)
{
	HINTERNET hInet;
	static int init=0;

	if(!init)
	{
		hInetFAAD = InternetOpen("FAAD", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

		if(!hInetFAAD)
		{
			return NULL;
		}

		init = 1;
	}

	hInet = InternetOpenUrl(hInetFAAD, url, 0, 0, INTERNET_FLAG_TRANSFER_BINARY, 1);

	if(!hInet)
	{
		return NULL;
	}

	return hInet;
}

int http_file_read(void *hInet, char *buffer, int length)
{
	unsigned long bytes_read;

	bytes_read = 0;

	InternetReadFile(hInet, buffer, length, &bytes_read);

	return bytes_read;
}

unsigned long http_file_length(void *hInet)
{
	unsigned long tmp_length, file_length;

	tmp_length = InternetSetFilePointer(hInet, 0, NULL, FILE_CURRENT, 0);
	
	file_length = InternetSetFilePointer(hInet, 0, NULL, FILE_END, 0);
	
	InternetSetFilePointer(hInet, tmp_length, NULL, FILE_BEGIN, 0);

	return file_length;
}

void http_file_close(void *hInet)
{
	InternetCloseHandle(hInet);
}