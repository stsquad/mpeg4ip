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
 * video.h - contains the interface class between the codec and the video
 * display hardware.
 */
#ifndef __VIDEO_SDL_H__
#define __VIDEO_SDL_H__ 1

#include "video.h"

#define MAX_VIDEO_BUFFERS 16

class CSDLVideoSync : public CVideoSync {
 public:
  CSDLVideoSync(CPlayerSession *psptr);
  ~CSDLVideoSync(void);
  int initialize_video(const char *name, int x, int y);  // from sync task
  int is_video_ready(uint64_t &disptime);  // from sync task
  int64_t play_video_at(uint64_t current_time, // from sync task
			 int &have_eof);
  int get_video_buffer(uint8_t **y,
		       uint8_t **u,
		       uint8_t **v);
  int filled_video_buffers(uint64_t time);
  int set_video_frame(const uint8_t *y,      // from codec
		      const uint8_t *u,
		      const uint8_t *v,
		      int m_pixelw_y,
		      int m_pixelw_uv,
		      uint64_t time);
  void config (int w, int h); // from codec
  void set_wait_sem (SDL_sem *p) { m_decode_sem = p; };  // from set up
  void flush_decode_buffers(void);    // from decoder task in response to stop
  void flush_sync_buffers(void);      // from sync task in response to stop
  void play_video(void);
  void set_screen_size(int scaletimes2); // 1 gets 50%, 2, normal, 4, 2 times
  void set_fullscreen(int fullscreen);
  int get_fullscreen (void) { return m_fullscreen; };
  void do_video_resize(void); // from sync
 private:
  int m_video_bpp;
  int m_video_scale;
  int m_fullscreen;
  unsigned int m_width, m_height;
  int m_video_initialized;
  int m_config_set;
  int m_paused;
  volatile int m_have_data;
  SDL_Surface *m_screen;
  SDL_Overlay *m_image;
  SDL_Rect m_dstrect;
  uint32_t m_fill_index, m_play_index;
  int m_decode_waiting;
  volatile int m_buffer_filled[MAX_VIDEO_BUFFERS];
  uint8_t *m_y_buffer[MAX_VIDEO_BUFFERS];
  uint8_t *m_u_buffer[MAX_VIDEO_BUFFERS];
  uint8_t *m_v_buffer[MAX_VIDEO_BUFFERS];
  uint64_t m_play_this_at[MAX_VIDEO_BUFFERS];
  int m_dont_fill;
};

/* frame doublers */
#ifdef USE_MMX
extern "C" void FrameDoublerMmx(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight);
#else
extern void FrameDoubler(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight, u_int32_t destWidth);
#endif

#endif
