#include "BogusDefs.h"
#include "mymutex.h"

ssize_t	recvfrom (int, void *, size_t, int, struct sockaddr *, int *)
{
	return 0;
}

ssize_t	sendto (int, const void *, size_t, int, const struct sockaddr *, int)
{
	return 0;
}

int daemon (int, int)
{
	return 0;
}

int inet_aton (const char *, struct in_addr *)
{
	return 0;
}

int bind(int, struct sockaddr *, int)
{
	return 0;
}

char * inet_ntoa(struct in_addr)
{
	return 0;
}

int ioctl(int, unsigned long, char*)
{
	return 0;
}

int socket(int, int, int)
{
	return 0;
}


void mymutex_unlock(void* x)
{
	x =x;
}

void mymutex_free(void* x)
{
	x =x;

}

void mymutex_lock(void* x)
{
	x =x;
}

long long timestamp = 0;


int ftruncate(int fd, int length)
{
	return 0;
}

mymutex_t mymutex_alloc()
{	
	return 0;
}

char *optarg = NULL;