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
 * Copyright (C) Cisco Systems Inc. 2001-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter@maersk-moller.net
 */
/*
 * video.cpp - provides codec to video hardware class
 */
#include <string.h>
#include "video_sdl.h"
#include "SDL_error.h"
#include "SDL_syswm.h"
#include "our_config_file.h"
#include "mpeg4ip_utils.h"
#include "player_util.h"

//#define SWAP_UV 1

#ifdef _WIN32
// new hwsurface method doesn't work on windows.
// If you have pre-directX 8.1 - you'll want to define OLD_SURFACE
//#define OLD_SURFACE 1
#define WM_WIN 1
#endif

#if defined (__unix__)
#define WM_X11
#endif

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(video_message, "videosync")
#else
#define video_message(loglevel, fmt...) message(loglevel, "videosync", fmt)
#endif

/*
 * CSDLVideo class is the interface to SDL.  We probably want to 
 * move this to another file, to leave the ring buffer code (what is
 * CSDLVideoSync) alone
 * Having this seperate allows us to keep the video window present between
 * changing sessions.
 */
CSDLVideo::CSDLVideo(int initial_x, int initial_y, uint32_t mask)
{
  char buf[32];
  const SDL_VideoInfo *video_info;
  m_mutex = SDL_CreateMutex();
  m_image = NULL;
  m_screen = NULL;
  m_pos_x = initial_x;
  m_pos_y = initial_y;
  m_mask = mask;
  m_name = NULL;
  m_pixel_width = -1;
  m_pixel_height = -1;
  m_max_width = -1;
  m_max_height = -1;
  // One time initialization stuff
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0 || !SDL_VideoDriverName(buf, 1)) {
    video_message(LOG_CRIT, "Could not init SDL video: %s", SDL_GetError());
  }
  video_info = SDL_GetVideoInfo();
  switch (video_info->vfmt->BitsPerPixel) {
  case 16:
  case 32:
    m_video_bpp = video_info->vfmt->BitsPerPixel;
    break;
  default:
    m_video_bpp = 16;
    break;
  }
  m_old_win_w = m_old_win_h = 0;
  m_old_w = m_old_h = 0;
  SDL_ShowCursor(SDL_DISABLE);

  //Query the current Window system resolution 
  SDL_SysWMinfo         info;
  SDL_VERSION(&info.version);
  if (SDL_GetWMInfo(&info)) {
#if defined(WM_X11)
  if ( info.subsystem == SDL_SYSWM_X11 ) {
        info.info.x11.lock_func();
	m_max_width = DisplayWidth(info.info.x11.display, 
                            DefaultScreen(info.info.x11.display));
	m_max_height = DisplayHeight(info.info.x11.display, 
                            DefaultScreen(info.info.x11.display));
        info.info.x11.unlock_func();
        video_message(LOG_INFO, "Max Window resolution %dx%d", m_max_width, m_max_height);
  } else {
      video_message(LOG_ERR, "Failed to get Max resolution foe window system");
  }
#elif defined (WM_WIN)
      m_max_width=GetSystemMetrics(SM_CXSCREEN);
      m_max_height=GetSystemMetrics(SM_CYSCREEN);
      video_message(LOG_INFO, "Max Window resolution %dx%d", m_max_width, m_max_height);
#endif
  }
}

/*
 * Clean up - free everything
 */
CSDLVideo::~CSDLVideo (void)
{
  CHECK_AND_FREE(m_name);
  if (m_image) {
    SDL_FreeYUVOverlay(m_image);
    m_image = NULL;
  }
  if (m_screen) {
    SDL_FreeSurface(m_screen);
    m_screen = NULL;
  }
  if (m_mutex) {
    SDL_DestroyMutex(m_mutex);
    m_mutex = NULL;
  }
}

void CSDLVideo::set_name (const char *name)
{
  CHECK_AND_FREE(m_name);
  m_name = strdup(name);
}

void CSDLVideo::set_image_size (unsigned int w, unsigned int h, double aspect)
{
  m_image_w = w;
  m_image_h = h;
  m_aspect_ratio = aspect;
  // don't actually do anything until set_screen_size is called
}

/*
 * Set the correct screen size.
 */
void  CSDLVideo::set_screen_size(bool fullscreen, int video_scale, 
				 int pixel_width, int pixel_height,
				 int max_width, int max_height)
{
  if (pixel_width > -1) m_pixel_width = pixel_width;
  if (pixel_height > -1) m_pixel_height = pixel_height;
  if (max_width > -1) m_max_width = max_width;
  if (max_height > -1) m_max_height = max_height;

  // Check and see if we should use old values.
  if (pixel_width == -1 && m_pixel_width > -1) pixel_width = m_pixel_width;
  if (pixel_height == -1 && m_pixel_height > -1) pixel_height = m_pixel_height;
  if (max_width == -1 && m_max_width > -1) max_width = m_max_width;
  if (max_height == -1 && m_max_height > -1) max_height = m_image_h;

  if (video_scale <= 0) video_scale = m_video_scale;
  int win_w = m_image_w * video_scale / 2;
  int win_h = m_image_h * video_scale / 2;
    
  // It is only when we have positive values larger than zero for
  // pixel sizes that we resize
  if (pixel_width > 0 && pixel_height > 0) {
    // wmay - just use the passed in values.
    win_w = (win_h * pixel_width) / pixel_height;
#if 0
    // Square pixels needs no resize.
    if (pixel_width != pixel_height) {
 
      // enable different handling of full screen versus non full screen
      if (fullscreen == false) {
	if (pixel_width > pixel_height)
	  win_w = win_h * pixel_width / pixel_height;
	else 
	  win_h = win_w * pixel_height / pixel_width;
      } else {
 
	// For now we do the same as in non full screen.
	if (pixel_width > pixel_height)
	  win_w = win_h * pixel_width / pixel_height;
	else 
	  win_h = win_w * pixel_height / pixel_width;
      }
    }
#endif
  } else {
    // this case is when we set it from the aspect ratio defined.
    if (m_aspect_ratio != 0.0 && m_aspect_ratio != 1.0) {
      double win_wf = m_aspect_ratio;
      int win_wtest;
      win_wf *= win_h;
      video_message(LOG_INFO, "aspect ratio %g %g %d %d", 
		    m_aspect_ratio, win_wf, win_w, win_h);
      win_wtest = (int)win_wf;
      if (win_wtest >= win_w) {
	win_w = win_wtest;
      } else {
	video_message(LOG_ERR, "aspect ratio width less than orig");
      }
    }
  }

  if (fullscreen) {
    //Suren, get the max resolution of window system and scale to that
    //--------
    if(m_max_width > 0 ) win_w = m_max_width;
    if (m_max_height > 0 ) win_h = m_max_height;
    video_message(LOG_INFO, "Setting full screen mode %dx%d", win_w, win_h);
    //--------
  }

  m_video_scale = video_scale;

#ifdef OLD_SURFACE
  int mask = SDL_SWSURFACE | SDL_ASYNCBLIT;
#else
  int mask = SDL_HWSURFACE;
#endif
  if (fullscreen) {
    mask |= SDL_FULLSCREEN;
#ifdef _WIN32
    video_scale = 2;
#endif
  }
  mask |= m_mask;

  if (win_w == m_old_win_w &&
      win_h == m_old_win_h &&
      m_image_w == m_old_w &&
      m_image_h == m_old_h &&
      m_fullscreen == fullscreen) {
    return;
  }
  video_message(LOG_DEBUG, "Setting video mode %d %d %x %g", 
		win_w, win_h, mask, m_aspect_ratio);
  m_fullscreen = fullscreen;
  SDL_LockMutex(m_mutex);
  if (m_image) {
    SDL_FreeYUVOverlay(m_image);
    m_image = NULL;
  }

  if (m_screen) {
    SDL_FreeSurface(m_screen);
    m_screen = NULL;
  }

  m_screen = SDL_SetVideoMode(win_w, win_h, m_video_bpp, 
			      mask);
  if (m_screen == NULL) {
    m_screen = SDL_SetVideoMode(win_w, win_h, m_video_bpp, mask);
    if (m_screen == NULL) {
      video_message(LOG_CRIT, "sdl error message is %s", SDL_GetError());
      SDL_Delay(1000);
      abort();
    }
  }
  if (m_pos_x != 0 || m_pos_y != 0) {
    // we only try this the first time the video window gets initialized - 
    // not if we're resize a persistent connection
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    int ret;
    ret = SDL_GetWMInfo(&info);
    if (ret > 0) {
#ifdef unix
      if (info.subsystem == SDL_SYSWM_X11) {
	info.info.x11.lock_func();
#if 0
	XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, 
		    m_pos_x, m_pos_y);
#else
	XWindowChanges xchang;
	memset(&xchang, 0, sizeof(xchang));
	xchang.x = m_pos_x;
	xchang.y = m_pos_y;
	xchang.stack_mode = Above;
	XConfigureWindow(info.info.x11.display,
			 info.info.x11.wmwindow,
			 CWStackMode | CWX | CWY,
			 &xchang);
#endif
	info.info.x11.unlock_func();
      }
#endif
    }
    m_pos_x = 0;
    m_pos_y = 0;
  }

  m_dstrect.x = 0;
  m_dstrect.y = 0;
  m_dstrect.w = m_screen->w;
  m_dstrect.h = m_screen->h;

#ifdef OLD_SURFACE
  if (video_scale == 4) {
    m_image = SDL_CreateYUVOverlay(m_image_w << 1, 
				   m_image_h << 1,
				   SDL_YV12_OVERLAY, 
				   m_screen);
  } else 
#endif
    {
    m_image = SDL_CreateYUVOverlay(m_image_w, 
				   m_image_h,
				   SDL_YV12_OVERLAY, 
				   m_screen);
    }
  if (m_image_w > m_image->pitches[0]) {
    video_message(LOG_ERR, "Window returned width of %u %u requested", 
		  m_image->pitches[0], m_image_w);
  }
  if (m_name && strlen(m_name) != 0) {
    SDL_WM_SetCaption(m_name, NULL);
  }
  SDL_UnlockMutex(m_mutex);
  m_old_win_w = win_w;
  m_old_win_h = win_h;
  m_old_w = m_image_w;
  m_old_h = m_image_h;
}

/*
 * Display the image - copy the yuv buffers to the SDL overlay
 * height and width are set via set_image_size call
 */
void CSDLVideo::display_image (const uint8_t *y, const uint8_t *u, 
			       const uint8_t *v,
			       uint32_t yStride, uint32_t uvStride)
{
  unsigned int ix;
  uint8_t *to;
  const uint8_t *from;
  if (yStride == 0) yStride = m_image_w;
  if (uvStride == 0)  uvStride = m_image_w / 2;

  SDL_LockMutex(m_mutex);
  if (SDL_LockYUVOverlay(m_image)) {
    video_message(LOG_ERR, "Failed to lock image");
    SDL_UnlockMutex(m_mutex);
    return;
  } 

  // Must always copy the buffer to memory.  This creates 2 copies of this
  // data (probably a total of 6 - libsock -> rtp -> decoder -> our ring ->
  // sdl -> hardware)
#ifdef OLD_SURFACE
  if (m_fullscreen == false && m_video_scale == 4) {
    // when scaling to 200%, don't use SDL stretch blit
    // use a smoothing (averaging) blit instead
    // we only do this for maybe windows - otherwise, let SDL do it.
#if 0
    // sorry - too many problems with this...
    FrameDoublerMmx(y, m_image->pixels[0], 
		    m_image_w, m_image_h);
    FrameDoublerMmx(v, m_image->pixels[1], 
		    m_image_w >> 1, m_image_h >> 1);
    FrameDoublerMmx(u, m_image->pixels[2], 
		    m_image_w >> 1, m_image_h >> 1);
#else
    FrameDoubler(y, m_image->pixels[0], 
		 m_image_w, m_image_h, m_image->pitches[0]);
    FrameDoubler(v, m_image->pixels[1], 
		 m_image_w >> 1, m_image_h >> 1, m_image->pitches[1]);
    FrameDoubler(u, m_image->pixels[2], 
		 m_image_w >> 1, m_image_h >> 1, m_image->pitches[2]);
#endif
  } else 
#endif
    {
      // let SDL blit, either 1:1 for 100% or decimating by 2:1 for 50%
      uint32_t bufsize = m_image_w * m_image_h * sizeof(uint8_t);
      unsigned int width = m_image_w, height = m_image_h;
      if (yStride != m_image->pitches[0]) {
	// The width is not equal to the size in the SDL buffers - 
	// we need to copy a row at a time
	to = (uint8_t *)m_image->pixels[0];
	from = y;
	for (ix = 0; ix < height; ix++) {
	  memcpy(to, from, MIN(width, m_image->pitches[0]));
	  to += m_image->pitches[0];
	  from += yStride;
	}
      } else {
	// Copy entire Y frame
	memcpy(m_image->pixels[0], 
	       y,
	       bufsize);
      }
      // We reduce the sizes for U and V planes (they are 1/4 the size)
      bufsize /= 4;
      width /= 2;
      height /= 2;
#ifdef SWAP_UV
#define V 2
#define U 1
#else
#define V 1
#define U 2
#endif
      // Copy the U and V - same comments as above
      if (uvStride != m_image->pitches[V]) {
	to = (uint8_t *)m_image->pixels[V];
	from = v;
	for (ix = 0; ix < height; ix++) {
	  memcpy(to, from, MIN(m_image->pitches[V], width));
	  to += m_image->pitches[V];
	  from += uvStride;
	}
      } else {
	memcpy(m_image->pixels[V], 
	       v,
	       bufsize);
      }
      if (uvStride != m_image->pitches[U]) {
	to = (uint8_t *)m_image->pixels[U];
	from = u;
	for (ix = 0; ix < height; ix++) {
	  memcpy(to, from, MIN(width, m_image->pitches[U]));
	  to += m_image->pitches[U];
	  from += uvStride;
	}
      } else {
	memcpy(m_image->pixels[U], 
	       u,
	       bufsize);
      }
      
    }

  // unlock, then display
  SDL_UnlockYUVOverlay(m_image);
  // Actually display the video
#ifndef darwin
  int rval = 
#endif
    SDL_DisplayYUVOverlay(m_image, &m_dstrect);
  SDL_UnlockMutex(m_mutex);
#ifndef darwin
#define CORRECT_RETURN 0
  if (rval != CORRECT_RETURN) {
    video_message(LOG_ERR, "Return from display is %d", rval);
  }
#else
#define CORRECT_RETURN 1
#endif
}

#define BLANK_Y (0)
#define BLANK_U (0x80)
#define BLANK_V (0x80)
void CSDLVideo::blank_image (void)
{
  unsigned int ix;
  uint8_t *to;
  if (SDL_LockYUVOverlay(m_image)) {
    video_message(LOG_ERR, "Failed to lock image");
    return;
  } 
  uint32_t bufsize = m_image_w * m_image_h * sizeof(uint8_t);
  unsigned int width = m_image_w, height = m_image_h;
  if (width != m_image->pitches[0]) {
    // The width is not equal to the size in the SDL buffers - 
    // we need to copy a row at a time
    to = (uint8_t *)m_image->pixels[0];
    for (ix = 0; ix < height; ix++) {
      memset(to, BLANK_Y, MIN(width, m_image->pitches[0]));
      to += m_image->pitches[0];
    }
  } else {
    // Copy entire Y frame
    memset(m_image->pixels[0], 
	   BLANK_Y,
	   bufsize);
  }
      
  // We reduce the sizes for U and V planes (they are 1/4 the size)
  bufsize /= 4;
  width /= 2;
  height /= 2;
#ifdef SWAP_UV
#define V 2
#define U 1
#else
#define V 1
#define U 2
#endif
  // Copy the U and V - same comments as above
  if (width != m_image->pitches[V]) {
    to = (uint8_t *)m_image->pixels[V];
    for (ix = 0; ix < height; ix++) {
      memset(to, BLANK_U, MIN(width, m_image->pitches[V]));
      to += m_image->pitches[V];
    }
  } else {
    memset(m_image->pixels[V], 
	   BLANK_U,
	   bufsize);
  }
  if (width != m_image->pitches[U]) {
    to = (uint8_t *)m_image->pixels[U];
    for (ix = 0; ix < height; ix++) {
      memset(to, BLANK_V, MIN(m_image->pitches[U],width));
      to += m_image->pitches[U];
    }
  } else {
    memset(m_image->pixels[U], 
	   BLANK_V,
	   bufsize);
  }
      
  SDL_UnlockYUVOverlay(m_image);
  SDL_DisplayYUVOverlay(m_image, &m_dstrect);
}

void CSDLVideo::set_cursor (bool setit)
{
  SDL_ShowCursor(setit ? SDL_ENABLE : SDL_DISABLE);
}

void DestroyVideoPersistence (void *persist)
{
  CSDLVideo *temp = (CSDLVideo *)persist;
  delete temp;
  SDL_Quit();
}
