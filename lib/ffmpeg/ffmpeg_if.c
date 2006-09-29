
#include "mpeg4ip.h"
#include "ffmpeg_if.h"
#include "mutex.h"
#include "mpeg4ip_utils.h"

static bool inited = false;
static mutex_t mutex = NULL;

static void init (void)
{
  if (inited == false) {
    message(LOG_DEBUG, "ffmpegif", "initialize mutex");
    mutex = MutexCreate();
    inited = true;
  }
}

int ffmpeg_interface_lock (void)
{
  init();
  return MutexLock(mutex);
}

int ffmpeg_interface_unlock (void)
{
  return MutexUnlock(mutex);
}
