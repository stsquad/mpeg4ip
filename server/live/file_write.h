
#ifndef __FILE_WRITE_H__
#define __FILE_WRITE_H__

#include <stdlib.h>
#include <sys/time.h>

class CFileWriteBase
{
 public:
  CFileWriteBase(const char *name) {};
  virtual ~CFileWriteBase(void) {};

  virtual int initialize_audio(int channels,
			       int sample_rate,
			       int sample_duration,
			       int time_scale,
			       const char *codec_name,
			       struct timeval *start_time) = 0;
  virtual int write_audio_frame(unsigned char *buffer,
				size_t size) = 0;
  
};

#endif
		   


