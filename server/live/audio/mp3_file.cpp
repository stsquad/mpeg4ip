#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mp3_file.h"


CMp3FileWrite::CMp3FileWrite (const char *filename) : CFileWriteBase(filename)
{
  m_fd = creat(filename, S_IWRITE | S_IREAD);
}

CMp3FileWrite::~CMp3FileWrite (void)
{
  close(m_fd);
}

int CMp3FileWrite::initialize_audio (int channels,
				     int sample_rate,
				     int sample_duration,
				     int time_scale,
				     const char *codec_name,
				     struct timeval *start_time)
{
  m_start_time = *start_time;
  return 1;
}

int CMp3FileWrite::write_audio_frame (unsigned char *buffer, 
				      size_t size)
{
  return (write(m_fd, buffer, size));
}
