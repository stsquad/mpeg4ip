/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * audio.h - provides a class that interfaces between the codec and
 * the SDL audio application.  Will provide for volume, buffering,
 * syncronization
 */

#ifndef __AUDIO_SDL_H__
#define __AUDIO_SDL_H__ 1

#include "audio.h"
#include <portaudio.h>
#define DECODE_BUFFERS_MAX 32

class CSDLAudioSync : public CAudioSync {
 public:
  CSDLAudioSync(CPlayerSession *psptr, int volume);
  ~CSDLAudioSync(void);
  // APIs from  codec
  uint8_t *get_audio_buffer(void);
  void filled_audio_buffer(uint64_t ts, int resync);
  uint32_t set_config(int freq, int channels, audio_format_t format, uint32_t max_buffer_size);
  void set_eof(void);
  void load_audio_buffer(uint8_t *from, 
			 uint32_t bytes, 
			 uint64_t ts, 
			 int resync);
  
  // APIs from sync task
  int initialize_audio(int have_video);
  int is_audio_ready(uint64_t &disptime);
  uint64_t check_audio_sync(uint64_t current_time, int &have_eof);
  void play_audio(void);
  int audio_callback(void *stream, unsigned long len,
		      PaTimestamp outtime);
  void flush_sync_buffers(void);
  void flush_decode_buffers(void);

  // Initialization, other APIs
  void set_wait_sem(SDL_sem *p) { }; //m_audio_waiting = p; } ;
  void set_volume(int volume);

 private:
  volatile int m_dont_fill;
  uint64_t m_buffer_ts;
  uint32_t m_buffer_offset_on;
  uint32_t m_buffer_size;
  uint32_t m_fill_index, m_play_index;
  volatile int m_buffer_filled[DECODE_BUFFERS_MAX];
  uint64_t m_buffer_time[DECODE_BUFFERS_MAX];
  uint64_t m_last_fill_timestamp;
  uint64_t m_play_time;
  SDL_AudioSpec m_obtained;
  uint8_t *m_sample_buffer[DECODE_BUFFERS_MAX];
  int m_config_set;
  int m_audio_initialized;
  int m_freq;
  int m_resync_required;
  int m_audio_paused;
  int m_consec_no_buffers;
  volatile int m_audio_waiting_buffer;
  int m_use_SDL_delay;
  uint32_t m_resync_buffer;
  SDL_sem *m_audio_waiting;
  uint32_t m_skipped_buffers;
  uint32_t m_didnt_fill_buffers;
  int m_first_time;
  int m_first_filled;
  uint32_t m_msec_per_frame;
  uint64_t m_buffer_latency;
  int m_consec_wrong_latency;
  int64_t m_wrong_latency_total;
  int m_volume;
  int m_do_sync;
  int m_load_audio_do_next_resync;
  uint32_t m_sample_size;
  uint32_t m_play_sample_index;
  uint32_t m_buffer_bytes_loaded;
  uint32_t m_bytes_per_sample_input;
  uint32_t m_bytes_per_sample_output;
  uint64_t m_loaded_next_ts;
  int m_silence;
  PortAudioStream *m_pa_stream;
  SDL_mutex *m_pa_mutex;

  int m_have_buffer_latency;
  uint64_t m_first_outtime;
  uint64_t m_first_currtime;
};

#endif


