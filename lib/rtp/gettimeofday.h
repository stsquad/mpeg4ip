#if defined(NEED_GETTIMEOFDAY) || defined(_WIN32)

int gettimeofday(struct timeval *tp, void *);

#endif

