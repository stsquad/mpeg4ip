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
#ifndef __VIDEO_SDL_H__
#define __VIDEO_SDL_H__ 1

#include "mpeg4ip.h"
#include "mpeg4ip_sdl_includes.h"

// if we wanted to offer an alternative to SDL, we could do so
// by creating a base class here.
class CSDLVideo {
 public:
  CSDLVideo(int initial_x = 0, int initial_y = 0, uint32_t mask = 0);
  virtual ~CSDLVideo(void);
  void set_name(const char *name);
  void set_image_size(unsigned int w, unsigned int h,
		      double aspect_ratio);
  void set_screen_size(bool fullscreen, int scale = 0, 
		       int pixel_width = -1, int pixel_height = -1,
		       int max_width = -1, int max_height = -1);

  virtual void display_image(const uint8_t *y, const uint8_t *u, const uint8_t *v, 
		     uint32_t yStride = 0, uint32_t uvStride = 0);
  void blank_image(void);
  void set_cursor(bool setit);
  uint get_width (void) { return m_image_w; };
  uint get_height (void) { return m_image_h; };
  bool get_fullscreen(void) { return m_fullscreen; };
 protected:
  int m_pos_x, m_pos_y;
  uint32_t m_mask;
  SDL_Surface *m_screen;
  SDL_Overlay *m_image;
  SDL_Rect m_dstrect;
  SDL_mutex *m_mutex;
  int m_video_bpp;
  const char *m_name;
  unsigned int m_image_w, m_image_h, m_old_w, m_old_h;
  int m_old_win_w, m_old_win_h;
  double m_aspect_ratio;
  int m_pixel_width;
  int m_pixel_height;
  int m_max_width;
  int m_max_height;
  bool m_fullscreen;
  int m_video_scale;
};
    
  
/* frame doublers */
#ifdef USE_MMX
extern "C" void FrameDoublerMmx(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight);
#else
extern void FrameDoubler(u_int8_t* pSrcPlane, u_int8_t* pDstPlane, 
	u_int32_t srcWidth, u_int32_t srcHeight, u_int32_t destWidth);
#endif

void DestroyVideoPersistence(void *persist);

#endif
