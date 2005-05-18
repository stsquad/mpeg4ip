
#ifndef __MP3IF_H__
#define __MP3IF_H__ 1

#include "mpeg4ip.h"
#include <MPEGaudio.h>
#include "codec_plugin.h"
#include <fposrec/fposrec.h>

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr
typedef struct mp3_codec_t {
  codec_data_t c;

  MPEGaudio *m_mp3_info;
  int m_record_sync_time;
  uint64_t m_first_time_offset;
  uint64_t m_current_time;
  uint64_t m_last_rtp_ts;
  uint32_t m_current_frame;
  int m_audio_inited;
  uint32_t m_freq;  // frequency
  int m_chans, m_samplesperframe;
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  /*
   * raw file
   */
  FILE *m_ifile;
  uint8_t *m_buffer;
  uint32_t m_buffer_size_max;
  uint32_t m_buffer_size;
  uint32_t m_buffer_on;
  uint64_t m_framecount;
  CFilePosRecorder *m_fpos;
} mp3_codec_t;

codec_data_t *mp3_file_check(lib_message_func_t message,
			     const char *name, 
			     double *max,
			     char *desc[4],
			     CConfigSet *pConfig);
int mp3_file_next_frame(codec_data_t *your_data,
			uint8_t **buffer,
			frame_timestamp_t *ts);

int mp3_raw_file_seek_to(codec_data_t *ptr, uint64_t ts);
#endif /* __MP3IF_H__ */
