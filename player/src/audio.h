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

#include <stdint.h>
#include <unistd.h>
#include <SDL.h>
#include "player_session.h"

#define DECODE_BUFFERS_MAX 8

// states
class CAudioSync {
 public:
  CAudioSync(CPlayerSession *psptr, int volume);
  ~CAudioSync();
  // APIs from  codec
  short *get_audio_buffer(void);
  void filled_audio_buffer(uint64_t ts, int resync);
  void set_config(int freq, int channels, int format, size_t max_buffer_size);
  void set_eof(void) { m_eof_found = 1; };
  
  // APIs from sync task
  int initialize_audio(int have_video);
  int is_audio_ready(uint64_t &disptime);
  uint64_t check_audio_sync(uint64_t current_time, int &have_eof);
  void play_audio(void);
  void audio_callback(Uint8 *stream, int len);
  void flush_sync_buffers(void);
  void flush_decode_buffers(void);

  // Initialization, other APIs
  void set_sync_sem(SDL_sem *p) {m_sync_sem = p; } ;
  void set_wait_sem(SDL_sem *p) {m_audio_waiting = p; } ;
  void set_volume(int volume);
 private:
  int m_dont_fill;
  size_t m_buffer_size;
  size_t m_fill_index, m_play_index;
  volatile int m_buffer_filled[DECODE_BUFFERS_MAX];
  uint64_t m_buffer_time[DECODE_BUFFERS_MAX];
  uint64_t m_play_time;
  SDL_AudioSpec m_obtained;
  short *m_sample_buffer[DECODE_BUFFERS_MAX];
  int m_config_set;
  int m_audio_initialized;
  int m_freq;
  int m_channels;
  int m_format;
  int m_resync_required;
  int m_audio_paused;
  int m_consec_no_buffers;
  int m_audio_waiting_buffer;
  int m_eof_found;
  size_t m_resync_buffer;
  SDL_sem *m_sync_sem, *m_audio_waiting;
  CPlayerSession *m_psptr;
  size_t m_skipped_buffers;
  size_t m_didnt_fill_buffers;
  int m_first_time;
  size_t m_msec_per_frame;
  uint64_t m_buffer_latency;
  int m_consec_wrong_latency;
  int64_t m_wrong_latency_total;
  int m_volume;
  int m_do_sync;
};

#endif


