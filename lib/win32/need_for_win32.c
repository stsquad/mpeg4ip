#include "systems.h"
#include <sys/timeb.h>

int gettimeofday (struct timeval *t, void *foo)
{
	struct _timeb temp;
	_ftime(&temp);
	t->tv_sec = temp.time;
	t->tv_usec = temp.millitm * 1000;
	return (0);
}

char *strsep (char **sptr, const char *delim)
{
	char *start, *ret;
	start = ret = *sptr;
	if ((ret == NULL) || ret == '\0') {
	   return (NULL);
	}

	while (*ret != '\0' &&
		   strchr(delim, *ret) == NULL) {
		ret++;
	}
	if (*ret == '\0') {
		*sptr = NULL;
	} else {
	    *ret = '\0';
	    ret++;
	    *sptr = ret;
	}
	return (start);
}
