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
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter@maersk-moller.net
 */
/*
 * video.h - contains the interface class between the codec and the video
 * display hardware.
 */
#ifndef __VIDEO_H__
#define __VIDEO_H__ 1

#include <SDL.h>
#include "codec_plugin.h"

#define MAX_VIDEO_BUFFERS 16

class CPlayerSession;

class CVideoSync {
 public:
  CVideoSync(CPlayerSession *ptptr);
  virtual ~CVideoSync(void);

  void set_wait_sem (SDL_sem *p) { m_decode_sem = p; };  // from set up

  virtual void flush_decode_buffers(void);    
                              // from decoder task in response to stop
  void set_eof(void) { m_eof_found = 1; };
  int get_eof(void) { return m_eof_found; };
  virtual void set_screen_size(int scaletimes2); // 1 gets 50%, 2, normal, 4, 2 times
  virtual void set_fullscreen(int fullscreen);
  virtual int get_fullscreen (void) { return 0;};
  virtual int initialize_video(const char *name, int x, int y);  // from sync task
  virtual int is_video_ready(uint64_t &disptime);  // from sync task
  virtual int64_t play_video_at(uint64_t current_time, // from sync task
			 int &have_eof);
  virtual void do_video_resize(int m_pixel_width = -1, int m_pixel_height = -1, int m_max_width = -1, int m_max_height = -1);
  virtual void flush_sync_buffers(void);  // from sync task in response to stop
  virtual void double_width(void);
 protected:
  CPlayerSession *m_psptr;
  SDL_sem *m_decode_sem;
  int m_eof_found;
  uint32_t m_consec_skipped;
  uint32_t m_behind_frames;
  uint32_t m_total_frames;
  uint32_t m_filled_frames;
  uint64_t m_behind_time;
  uint64_t m_behind_time_max;
  uint32_t m_skipped_render;
  uint64_t m_msec_per_frame;
  uint64_t m_last_filled_time;
#ifdef ADD_ASPECT_RATIO
  int m_pixel_width;
  int m_pixel_height;
  int m_max_width;
  int m_max_height;
#endif
};

/* frame doublers */
#ifdef USE_MMX
extern "C" void FrameDoublerMmx(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight);
#else
extern void FrameDoubler(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight, u_int32_t destWidth);
#endif

video_vft_t *get_video_vft(void);

CVideoSync *create_video_sync(CPlayerSession *psptr);
#endif
