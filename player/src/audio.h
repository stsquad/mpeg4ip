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

#include "codec_plugin.h"
#include "mpeg4ip_sdl_includes.h"

#define DECODE_BUFFERS_MAX 32

class CPlayerSession;
// states
class CAudioSync {
 public:
  CAudioSync(CPlayerSession *psptr) { 
    m_psptr = psptr;
    m_eof = 0;
  } ;
  virtual ~CAudioSync(void) {};
  // APIs from  codec
  void clear_eof(void);
  void set_eof(void);
  int get_eof(void) { return m_eof; };
  // APIs from sync task
  virtual int initialize_audio(int have_video);
  virtual int is_audio_ready(uint64_t &disptime);
  virtual uint64_t check_audio_sync(uint64_t current_time, int &have_eof);
  virtual void play_audio(void);

  virtual void flush_sync_buffers(void);
  virtual void flush_decode_buffers(void);
  virtual uint32_t set_config(int freq, int channels, audio_format_t format, uint32_t max_samples) {return 0;};

  // Initialization, other APIs
  virtual void set_wait_sem(SDL_sem *p) {m_audio_waiting = p; } ;
  virtual void set_volume(int volume);
 protected:
  void audio_convert_data(void *from, uint32_t len);
  SDL_sem *m_audio_waiting;
  CPlayerSession *m_psptr;
  int m_eof;
  int m_channels, m_got_channels;
  audio_format_t m_format;
  void *m_convert_buffer;
  int16_t *m_fmt_buffer;
};

CAudioSync *create_audio_sync(CPlayerSession *, int volume);

audio_vft_t *get_audio_vft(void);

int do_we_have_audio(void);

#endif


