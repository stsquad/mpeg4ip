#ifndef HAVE_VSNPRINTF

#if defined(__cplusplus)
extern "C" {
#endif

int vsnprintf(char *s, size_t buf_size, const char *format, va_list ap);

#if defined(__cplusplus)
}
#endif

#endif /* HAVE_VSNPRINTF */

