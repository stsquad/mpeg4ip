#include <mpeg4ip.h>
#include <time.h>
#include "mpeg2t_private.h"

static int mpeg2t_debug_level = LOG_ALERT;
static error_msg_func_t mpeg2t_error_msg = NULL;

void mpeg2t_set_loglevel (int loglevel)
{
  mpeg2t_debug_level = loglevel;
}

void mpeg2t_set_error_func (error_msg_func_t func)
{
  mpeg2t_error_msg = func;
}

void mpeg2t_message (int loglevel, const char *fmt, ...)
{
  va_list ap;
  if (loglevel <= mpeg2t_debug_level) {
    va_start(ap, fmt);
    if (mpeg2t_error_msg != NULL) {
      (mpeg2t_error_msg)(loglevel, "mpeg2t", fmt, ap);
    } else {
 #if _WIN32 && _DEBUG
	  char msg[1024];

      _vsnprintf(msg, 1024, fmt, ap);
      OutputDebugString(msg);
      OutputDebugString("\n");
#else
      struct timeval thistime;
      char buffer[80];
      time_t secs;

      gettimeofday(&thistime, NULL);
      // To add date, add %a %b %d to strftime
      secs = thistime.tv_sec;
      strftime(buffer, sizeof(buffer), "%X", localtime(&secs));
      printf("%s.%03ld-mpeg2t-%d: ",
	     buffer, (unsigned long)thistime.tv_usec / 1000, loglevel);
      vprintf(fmt, ap);
      printf("\n");
#endif
    }
    va_end(ap);
  }
}
