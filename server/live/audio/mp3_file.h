
#ifndef __MP3_FILE_H__
#define __MP3_FILE_H__
#include "file_write.h"

class CMp3FileWrite : public CFileWriteBase
{
 public:
  CMp3FileWrite(const char *filename);
  ~CMp3FileWrite(void);

  int initialize_audio(int channels,
		       int sample_rate,
		       int sample_duration,
		       int time_scale,
		       const char *codec_name,
		       struct timeval *start_time);
  
  int write_audio_frame(unsigned char *buffer, size_t size);
 private:
  int m_fd;
  struct timeval m_start_time;
};

#endif
