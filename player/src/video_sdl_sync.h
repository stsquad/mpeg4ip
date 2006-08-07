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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter@maersk-moller.net
 */
/*
 * video.h - contains the interface class between the codec and the video
 * display hardware.
 */
#ifndef __VIDEO_SDL_SYNC_H__
#define __VIDEO_SDL_SYNC_H__ 1

#include "video.h"
#include "video_sdl.h"
#define MAX_VIDEO_BUFFERS 16

class CSDLVideoSync : public CVideoSync {
 public:
  CSDLVideoSync(CPlayerSession *psptr, void *video_persistence,
		int screen_pos_x, int screen_pos_y);
  ~CSDLVideoSync(void);
  int initialize(const char *name);  // from sync task
  bool is_ready(uint64_t &disptime);  // from sync task

  int get_video_buffer(uint8_t **y,
		       uint8_t **u,
		       uint8_t **v);
  void filled_video_buffers(uint64_t time);
  void set_video_frame(const uint8_t *y,      // from codec
		       const uint8_t *u,
		       const uint8_t *v,
		       int m_pixelw_y,
		       int m_pixelw_uv,
		       uint64_t time);
  void configure (int w, int h, double aspect_ratio); // from codec

  void set_screen_size(int scaletimes2); // 1 gets 50%, 2, normal, 4, 2 times
  void set_fullscreen(bool fullscreen);
  bool get_fullscreen (void) { return m_fullscreen; };
  void do_video_resize(int pixel_width = -1, int pixel_height = -1, int max_width = -1, int max_height = -1);
  void set_cursor (bool setit) {
    if (m_sdl_video != NULL) m_sdl_video->set_cursor(setit);
  }
 protected:
  void render(uint32_t play_index);
 private:
  CSDLVideo *m_sdl_video;
  int m_video_scale;
  bool m_fullscreen;
  unsigned int m_width, m_height;
  double m_aspect_ratio;

  uint8_t *m_y_buffer[MAX_VIDEO_BUFFERS];
  uint8_t *m_u_buffer[MAX_VIDEO_BUFFERS];
  uint8_t *m_v_buffer[MAX_VIDEO_BUFFERS];
  int m_pixel_width;
  int m_pixel_height;
  int m_max_width;
  int m_max_height;
  //#define WRITE_YUV 1
#ifdef WRITE_YUV
  FILE *m_outfile;
#endif
#if 0
  uint64_t *m_play_this_at;
  volatile bool *m_buffer_filled;
  bool m_initialized;
  bool m_config_set;
  bool m_decode_waiting; 
  bool m_dont_fill;
  uint32_t m_fill_index, m_play_index, m_max_buffers;

  void initialize_indexes (uint32_t num_buffers) {
    void *temp = malloc(sizeof(bool) * num_buffers);
    memset(temp, false, sizeof(bool) * num_buffers);
    m_buffer_filled = (volatile bool *)temp;
    temp = malloc(sizeof(uint64_t) * num_buffers);
    m_play_this_at = (uint64_t *)temp;
    m_max_buffers = num_buffers;
  };

  void increment_fill_index (void) {
    m_fill_index++;
    m_fill_index %= m_max_buffers;
    m_filled_frames++;
  };
  void increment_play_index (void) {
    m_buffer_filled[m_play_index] = false;
    m_play_index++;
    m_play_index %= m_max_buffers;
    m_total_frames++;
  };
  void notify_decode_thread (void) {
    if (m_decode_waiting) {
      // If the decode thread is waiting, signal it.
      m_decode_waiting = false;
      SDL_SemPost(m_decode_sem);
    }
  };
  bool dont_fill (void) { return m_dont_fill; };
  bool have_buffer_to_fill (void) {
    if (dont_fill()) return false;

    if (m_buffer_filled[m_fill_index]) {
      m_decode_waiting = true;
      SDL_SemWait(m_decode_sem);
      if (dont_fill()) return false;
      if (m_buffer_filled[m_fill_index]) {
#ifdef VIDEO_SYNC_FILL
	video_message(LOG_DEBUG, "Wait but filled %d", m_fill_index);
#endif
	return false;
      }
    }  
    return true;
  };
  void save_last_filled_time (uint64_t ts) {
    if (ts > m_last_filled_time) {
      uint64_t temp;
      temp = ts - m_last_filled_time;
      if (temp < m_msec_per_frame) {
	m_msec_per_frame = temp;
      }
    }
    m_last_filled_time = ts;
  };
#endif
};


#endif
