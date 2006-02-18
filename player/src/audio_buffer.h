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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#ifndef __AUDIO_BUFFER_H__
#define __AUDIO_BUFFER_H__ 1

#include "audio.h"

class CBufferAudioSync : public CAudioSync {
 public:
  CBufferAudioSync(CPlayerSession *psptr, int volume);
  ~CBufferAudioSync(void);

  // APIs from plugins
  uint8_t *get_audio_buffer(uint32_t freq_ts, uint64_t ts);
  void filled_audio_buffer(void);
  void set_config(uint32_t freq, uint32_t chans, 
		      audio_format_t format, uint32_t samples_per_frame);
  void load_audio_buffer(const uint8_t *from,
			 uint32_t bytes, 
			 uint32_t freq_ts,
			 uint64_t ts);
  // APIs from decoder task
  void flush_decode_buffers(void);

  // APIs from sync task
  int initialize_audio(int have_video);
  int is_audio_ready(uint64_t &disptime);
  bool check_audio_sync(uint64_t current_time, 
			uint64_t &resync_time,
			int64_t &wait_time,
			bool &have_eof, 
			bool &restart_sync);
  void flush_sync_buffers(void);
  void play_audio(void);
  virtual void set_volume(int volume) = 0;
  void display_status(void);
 protected:
  // APIs for hardware routines
  // initialize hardware needs to call audio_convert_init.
  bool audio_buffer_callback(uint8_t *outbuf, 
			     uint32_t len_bytes,
			     uint32_t latency_samples,
			     uint64_t current_time);
  virtual int InitializeHardware(void) = 0;
  virtual void StartHardware(void)  = 0;
  virtual void StopHardware(void) = 0;
  virtual void CopyBytesToHardware(uint8_t *from, 
				   uint8_t *to, 
				   uint32_t bytes) = 0;
  virtual bool Lock(void) {
    SDL_LockMutex(m_mutex);
    return true;
  };
  virtual void Unlock(void) {
    SDL_UnlockMutex(m_mutex);
  };
 protected:
  SDL_mutex *m_mutex;
  bool m_audio_configured;
  uint32_t m_freq;
  
  uint8_t *m_sample_buffer;
  uint32_t m_sample_buffer_size;
  uint32_t m_fill_offset;
  uint32_t m_play_offset;
  uint32_t m_filled_bytes;

  uint32_t m_output_buffer_size_bytes; // filled in by InitializeHardware

  uint32_t m_bytes_per_frame;
  uint32_t m_samples_per_frame;
  uint32_t m_msec_per_frame;

  uint8_t *m_local_buffer;
  uint64_t m_first_ts;
  uint32_t m_first_freq_ts;
  bool m_first_filled;
  uint64_t m_buffer_next_ts;      // next ts to fill
  uint32_t m_buffer_next_freq_ts; // same here
  
  uint64_t m_got_buffer_ts;
  uint32_t m_got_buffer_freq_ts;
  uint32_t m_got_buffer_silence_bytes;
  volatile bool m_dont_fill;
  volatile bool m_audio_waiting_buffer;
  SDL_sem *m_audio_waiting;

  bool     m_have_resync;
  bool     m_waiting_on_resync;
  uint32_t m_resync_offset;
  uint32_t m_resync_freq_ts;
  uint64_t m_resync_ts;

  bool     m_restart_request;
  uint8_t m_silence;

  bool m_hardware_initialized;
  int m_volume;

  bool m_first_callback;
  uint64_t m_hw_start_time;
  uint64_t m_samples_written;
  uint64_t m_latency_value;

  uint64_t m_total_samples_played;
  uint64_t m_sync_samples_added;
  uint64_t m_sync_samples_removed;
  bool m_have_jitter;
  int64_t m_jitter_msec_total;
  int64_t m_jitter_msec;
  uint64_t m_jitter_calc_ts;
  uint32_t m_jitter_calc_freq_ts;
  bool m_audio_paused;
  bool m_last_add_samples;
  uint64_t m_last_diff;
  // helper routines for buffer fill routines
  bool do_we_need_resync(uint32_t freq_ts, uint32_t &silence_samples);
  void wait_for_callback(void);
  void fill_silence(uint32_t silence_bytes);
  bool check_for_bytes(uint32_t bytes, uint32_t silence_bytes);

  void callback_done(void);
};
  
  
#endif
