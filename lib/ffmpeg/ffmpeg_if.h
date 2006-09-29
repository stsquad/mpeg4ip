

#ifndef __FFMPEG_IF_H__
#define __FFMPEG_IF_H__ 1

#ifdef __cplusplus
extern "C" {
#endif
  int ffmpeg_interface_lock(void);

  int ffmpeg_interface_unlock(void);

#ifdef __cplusplus
}
#endif

#endif
