
#ifndef __AUDIO_MP3_H__
#define __AUDIO_MP3_H__
#include "lame.h"
#include "file_write.h"

class CMp3Encoder {
 public:
  CMp3Encoder(int sample_freq, int bitrate, CFileWriteBase *fb);
  ~CMp3Encoder(void);
  void encode(unsigned char *buffer, size_t size);
  size_t get_requested_sample_size(void);
  const char *get_codec_name(void) { return "mp3 "; };
 private:
  size_t get_frame_size(void);
  void check_data_end(void);
  void process_encoded_data(int bytes_ret);
  lame_global_flags m_gf;
  unsigned char *m_encoded_data;
  size_t m_encoded_data_size;
  unsigned char *m_data_end;
  unsigned char *m_next_frame_start;
  unsigned char *m_prev_frame_start;
  size_t m_data_in_buffer;
  size_t m_framesize; // frame size of current frame
  CFileWriteBase *m_file_write;

};

int mp3_check_params(int freq, int bitrate, const char **ermsg);

#endif
