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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * audio.h - provides a class that interfaces between the codec and
 * the SDL audio application.  Will provide for volume, buffering,
 * syncronization
 */

#ifndef __SYNC_H__
#define __SYNC_H__ 1

#include "codec_plugin.h"
#include "mpeg4ip_sdl_includes.h"
#include "player_session.h"
#include "player_util.h"

#define DECODE_BUFFERS_MAX 32

class CPlayerSession;
// states
class CSync {
 public:
  CSync (CPlayerSession *psptr) { 
    m_psptr = psptr;
    m_decode_sem = NULL;
    m_eof = false;
  } ;
  virtual ~CSync (void) {
  };
  // APIs from  codec
  void clear_eof(void) { m_eof = false; };
  void set_eof(void) { 
    m_eof = true; 
    //    player_debug_message("eof set");
  };
  bool get_eof(void) { return m_eof; };

  void set_wait_sem (SDL_sem *p) { m_decode_sem = p; };  // from set up


  virtual void flush_sync_buffers(void) = 0;
  virtual void flush_decode_buffers(void) = 0;
  virtual void display_status(void) {};
 protected:
  CPlayerSession *m_psptr;
  SDL_sem *m_decode_sem; // pointer to player media decode thread semaphore
 private:
  bool m_eof;
};


class CTimedSync : public CSync 
{
 public:
  CTimedSync (const char *name, CPlayerSession *psptr);
  ~CTimedSync(void);
  bool play_at(uint64_t current_time,
	       bool have_audio_resync,
	       uint64_t &next_time,
	       bool &have_eof);
  void flush_sync_buffers(void);
  void flush_decode_buffers(void);
  CTimedSync *GetNext(void) { return m_next; };
  void SetNext(CTimedSync *next) { m_next = next; } ;
  virtual session_sync_types_t get_sync_type(void) = 0;
  virtual int initialize (const char *name) = 0;
  virtual bool is_ready(uint64_t &disptime) = 0;
  virtual void flush(void) {} ;
  virtual bool active_at_start(void) { return true; };
  bool is_initialized (void) { return m_initialized; };
  const char *GetName (void) { return m_name; };
 protected:
  virtual void render(uint32_t play_index) = 0;
  const char *m_name;
  uint64_t *m_play_this_at;
  volatile bool *m_buffer_filled;
  bool m_initialized;
  bool m_config_set;
  bool m_decode_waiting; 
  bool m_dont_fill;
  uint32_t m_fill_index, m_play_index, m_max_buffers;

  void initialize_indexes(uint32_t num_buffers);

  void increment_fill_index(void);
  void increment_play_index(void);
  void notify_decode_thread(void);
  bool dont_fill (void) { return m_dont_fill; };
  bool have_buffer_to_fill(void);
  void save_last_filled_time(uint64_t ts);
  
  uint32_t m_consec_skipped;
  uint32_t m_behind_frames;
  uint32_t m_total_frames;
  uint32_t m_filled_frames;
  uint64_t m_behind_time;
  uint64_t m_behind_time_max;
  uint32_t m_skipped_render;
  uint64_t m_msec_per_frame;
  uint64_t m_last_filled_time;

  CTimedSync *m_next;
};

#endif


