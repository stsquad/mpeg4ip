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

#ifndef __AUDIO_H__
#define __AUDIO_H__ 1

#include "sync.h"
#include "mpeg4ip_sdl_includes.h"

#define DECODE_BUFFERS_MAX 32

// states
class CAudioSync : public CSync {
 public:
  CAudioSync(CPlayerSession *psptr) : CSync(psptr) { 
    m_convert_buffer = NULL;
    m_fmt_buffer = NULL;
    m_audio_initialized = false;
    m_sample_interpolate_buffer = NULL;
  } ;
  virtual ~CAudioSync(void) {
    CHECK_AND_FREE(m_convert_buffer);
    CHECK_AND_FREE(m_fmt_buffer);
    CHECK_AND_FREE(m_sample_interpolate_buffer);
  };
  // APIs from sync task
  virtual int initialize_audio(int have_video);
  virtual int is_audio_ready(uint64_t &disptime);
  virtual bool check_audio_sync(uint64_t current_time, 
				uint64_t &resync_time,
				int64_t &wait_time,
				bool &have_eof,
				bool &restart_sync);
  virtual void play_audio(void);

  virtual void set_config(uint32_t freq, uint32_t channels, audio_format_t format, uint32_t max_samples) {return;};

  // Initialization, other APIs
  virtual void set_volume(int volume) = 0;
  uint32_t get_bytes_per_sample_input (void) { return m_bytes_per_sample_input;};
 protected:
  void audio_convert_init(uint32_t size, uint32_t samples) {
    m_convert_buffer = malloc(size);
    m_fmt_buffer = (int16_t *)malloc(samples * sizeof(int16_t) * m_channels);
  };
  void audio_convert_data(void *from, uint32_t len);
  void *interpolate_3_samples(void *next_sample);
  SDL_sem *m_audio_waiting;
  uint32_t m_channels, m_got_channels;
  audio_format_t m_decode_format;
  void *m_convert_buffer;
  int16_t *m_fmt_buffer;
  void *m_sample_interpolate_buffer;
  bool m_audio_initialized;
  uint32_t m_bytes_per_sample_input;
  uint32_t m_bytes_per_sample_output;

};

CAudioSync *create_audio_sync(CPlayerSession *);

audio_vft_t *get_audio_vft(void);

int do_we_have_audio(void);

#endif


