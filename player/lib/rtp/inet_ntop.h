#ifndef HAVE_INET_NTOP
#if defined(__cplusplus)
extern "C" {
#endif

const char *inet_ntop(int af, const void *src, char *dst, size_t size);

#if defined(__cplusplus)
}
#endif
#endif /* HAVE_INET_NTOP */
