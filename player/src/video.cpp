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
 * video.cpp - provides codec to video hardware class
 */
#include <string.h>
#include "player_util.h"
#include "video.h"
#include <SDL_syswm.h>

CVideoSync::CVideoSync (void)
{
  char buf[32];
  if (SDL_Init(SDL_INIT_VIDEO) < 0 || !SDL_VideoDriverName(buf, 1)) {
    player_error_message("Could not init SDL video: %s", SDL_GetError());
  }
  m_screen = NULL;
  m_image = NULL;
  m_video_initialized = 0;
  m_config_set = 0;
  m_have_data = 0;
  for (int ix = 0; ix < MAX_VIDEO_BUFFERS; ix++) {
    m_y_buffer[ix] = NULL;
    m_u_buffer[ix] = NULL;
    m_v_buffer[ix] = NULL;
    m_buffer_filled[ix] = 0;
  }
  m_play_index = m_fill_index = 0;
  m_decode_waiting = 0;
  m_dont_fill = 0;
  m_paused = 1;
  m_eof_found = 0;
  m_behind_frames = 0;
  m_total_frames = 0;
  m_behind_time = 0;
  m_behind_time_max = 0;
  m_skipped_render = 0;
  m_video_scale = 2;
}

CVideoSync::~CVideoSync (void)
{
  if (m_image) {
    SDL_FreeYUVOverlay(m_image);
    m_image = NULL;
  }
  if (m_screen) {
    SDL_FreeSurface(m_screen);
    m_screen = NULL;
  }
  for (int ix = 0; ix < MAX_VIDEO_BUFFERS; ix++) {
    if (m_y_buffer[ix]) {
      free(m_y_buffer[ix]);
      m_y_buffer[ix] = NULL;
    }
    if (m_u_buffer[ix]) {
      free(m_u_buffer[ix]);
      m_u_buffer[ix] = NULL;
    }
    if (m_v_buffer[ix]) {
      free(m_v_buffer[ix]);
      m_v_buffer[ix] = NULL;
    }
  }
  player_debug_message("Video Sync Stats:");
  player_debug_message("Displayed-behind frames %d", m_behind_frames);
  player_debug_message("Total frames displayed %d", m_total_frames);
  player_debug_message("Max behind time %llu", m_behind_time_max);
  player_debug_message("Average behind time %llu", m_behind_time / m_behind_frames);
  player_debug_message("Skipped rendering %u", m_skipped_render);
}

/*
 * CVideoSync::config - routine for the codec to call to set up the
 * width, height and frame_rate of the video
 */
void CVideoSync::config (int w, int h, int frame_rate)
{
  m_width = w;
  m_height = h;
  for (int ix = 0; ix < MAX_VIDEO_BUFFERS; ix++) {
    m_y_buffer[ix] = (Uint8 *)malloc(w * h * sizeof(Uint8));
    m_u_buffer[ix] = (Uint8 *)malloc(w/2 * h/2 * sizeof(Uint8));
    m_v_buffer[ix] = (Uint8 *)malloc(w/2 * h/2 * sizeof(Uint8));
    m_buffer_filled[ix] = 0;
  }
  m_msec_per_frame = 1000 / frame_rate;
  m_config_set = 1;
  player_debug_message("Config for %llu msec per video frame", m_msec_per_frame);
}

/*
 * CVideoSync::initialize_video - Called from sync task to initialize
 * the video window
 */  
int CVideoSync::initialize_video (const char *name, int x, int y)
{
  if (m_video_initialized == 0) {
    if (m_config_set) {
      const SDL_VideoInfo *video_info;
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

      SDL_SysWMinfo info;
      SDL_VERSION(&info.version);
      int ret;
      ret = SDL_GetWMInfo(&info);
      // Oooh... fun... To scale the video, just pass the width and height
      // to this routine. (ie: m_width *2, m_height * 2).
      int w, h;
      if (m_video_scale == 1) {
	w = m_width / 2;
	h = m_height / 2;
      } else {
	w = m_width * m_video_scale / 2;
	h = m_height * m_video_scale / 2;
      }
      m_screen = SDL_SetVideoMode(w,
				  h,
				  m_video_bpp,
				  SDL_SWSURFACE | SDL_ASYNCBLIT | SDL_RESIZABLE);
      if (ret > 0) {
#ifdef unix
	//	player_debug_message("Trying");
	if (info.subsystem == SDL_SYSWM_X11) {
	  info.info.x11.lock_func();
	  XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, x, y);
	  info.info.x11.unlock_func();
	}
#endif
      }
      SDL_WM_SetCaption(name, NULL);
      m_dstrect.x = 0;
      m_dstrect.y = 0;
      m_dstrect.w = m_screen->w;
      m_dstrect.h = m_screen->h;
#if 0
      player_debug_message("Created mscreen %p hxw %d %d", m_screen, m_height,
			   m_width);
#endif
      m_image = SDL_CreateYUVOverlay(m_width, 
				     m_height,
				     SDL_YV12_OVERLAY, 
				     m_screen);
      m_video_initialized = 1;
      return (1);
    } else {
      return (0);
    }
  }
  return (1);
}

/*
 * CVideoSync::is_video_ready - called from sync task to determine if
 * we have sufficient number of buffers stored up to start displaying
 */
int CVideoSync::is_video_ready (uint64_t &disptime)
{
  disptime = m_play_this_at[m_play_index];
  if (m_dont_fill) {
    return 0;
  }
  return (m_buffer_filled[(m_play_index + 2*MAX_VIDEO_BUFFERS/3) % MAX_VIDEO_BUFFERS] == 1);
}

void CVideoSync::play_video (void) 
{
  player_debug_message("video start");
}

/*
 * CVideoSync::play_video_at - called from sync task to show the next
 * video frame (if the timing is right
 */
int CVideoSync::play_video_at (uint64_t current_time, 
			       uint64_t &play_this_at,
			       int &have_eof)
{
  m_current_time = current_time;
  /*
   * If we have end of file, indicate it
   */
  if (m_eof_found != 0) {
    have_eof = 1;
    return (0);
  }

  /*
   * If the next buffer is not filled, indicate that, as well
   */
  if (m_buffer_filled[m_play_index] == 0) {
    return 0;
  }
  
  /*
   * we have a buffer.  If it is in the future, don't play it now
   */
  play_this_at = m_play_this_at[m_play_index];
  if (play_this_at > current_time) {
    return (1);
  }

  /*
   * If we're behind - see if we should just skip it
   */
  if (play_this_at < current_time) {
    m_behind_frames++;
    uint64_t behind = current_time - play_this_at;
    m_behind_time += behind;
    if (behind > m_behind_time_max) m_behind_time_max = behind;
#if 0
    if ((m_behind_frames % 64) == 0) {
      player_debug_message("behind %llu avg %llu max %llu",
			   behind, m_behind_time / m_behind_frames,
			   m_behind_time_max);
    }
#endif
  }
  m_paused = 0;
  /*
   * If we're within 1/2 of the frame time, go ahead and display
   * this frame
   */
  if ((play_this_at + m_msec_per_frame) > current_time) {

    if (SDL_LockYUVOverlay(m_image)) {
      player_debug_message("Failed to lock image");
    }
    // Must always copy the buffer to memory.  This creates 2 copies of this
    // data (probably a total of 6 - libsock -> rtp -> decoder -> our ring ->
    // sdl -> hardware)
    size_t bufsize = m_width * m_height * sizeof(Uint8);
    memcpy(m_image->pixels[0], 
	   m_y_buffer[m_play_index], 
	   bufsize);
    bufsize /= 4;
    memcpy(m_image->pixels[1],
	   m_v_buffer[m_play_index],
	   bufsize);
    memcpy(m_image->pixels[2],
	   m_u_buffer[m_play_index],
	   bufsize);

    int rval = SDL_DisplayYUVOverlay(m_image, &m_dstrect);
    if (rval != 0) {
      player_error_message("Return from display is %d", rval);
    }
    SDL_UnlockYUVOverlay(m_image);
  } else {
    //  player_debug_message("Video lagging current time %llu %llu", 
    //		 play_this_at, current_time);
    /*
     * Else - we're lagging - just skip and hope we catch up...
     */
    m_skipped_render++;
  }
  /*
   * Advance the buffer
   */
  m_buffer_filled[m_play_index] = 0;
  m_play_index++;
  m_play_index %= MAX_VIDEO_BUFFERS;
  m_current_time = current_time;
  m_total_frames++;
  // okay - we need to signal the decode task to continue...
  if (m_decode_waiting) {
    m_decode_waiting = 0;
    SDL_SemPost(m_decode_sem);
  }
  return (0);
}

/*
 * CVideoSync::set_video_frame - called from codec to indicate a new
 * frame is ready.
 * Inputs - y - pointer to y buffer - should point to first byte to copy
 *          u - pointer to u buffer
 *          v - pointer to v buffer
 *          pixelw_y - width of row in y buffer (may be larger than width
 *                   set up above.
 *          pixelw_uv - width of row in u or v buffer.
 *          time - time to display
 *          current_time - current time we're displaying - this allows the
 *            codec to intelligently drop frames if it's falling behind.
 */
int CVideoSync::set_video_frame(const Uint8 *y, 
				     const Uint8 *u, 
				     const Uint8 *v,
				     int pixelw_y, 
				     int pixelw_uv, 
				     uint64_t time,
				     uint64_t &current_time)
{
  Uint8 *dst;
  const Uint8 *src;
  uint ix;

  if (m_dont_fill != 0) 
    return (m_paused);

  /*
   * Do we have a buffer ?  If not, indicate that we're waiting, and wait
   */
  if (m_buffer_filled[m_fill_index] != 0) {
    m_decode_waiting = 1;
    SDL_SemWait(m_decode_sem);
    current_time = m_current_time;
    if (m_buffer_filled[m_fill_index] == 0)
      return m_paused;
  }  
  current_time = m_current_time;

  /*
   * copy the relevant data to the local buffers
   */
  m_play_this_at[m_fill_index] = time;
  src = y;
  dst = m_y_buffer[m_fill_index];
  for (ix = 0; ix < m_height; ix++) {
    memcpy(dst, src, m_width);
    dst += m_width;
    src += pixelw_y;
  }
  src = u;
  dst = m_u_buffer[m_fill_index];
  uint uvheight = m_height/2;
  uint uvwidth = m_width/2;
  for (ix = 0; ix < uvheight; ix++) {
    memcpy(dst, src, uvwidth);
    dst += uvwidth;
    src += pixelw_uv;
  }

  src = v;
  dst = m_v_buffer[m_fill_index];
  for (ix = 0; ix < uvheight; ix++) {
    memcpy(dst, src, uvwidth);
    dst += uvwidth;
    src += pixelw_uv;
  }
  /*
   * advance the buffer, and post to the sync task
   */
  m_buffer_filled[m_fill_index] = 1;
  m_fill_index++;
  m_fill_index %= MAX_VIDEO_BUFFERS;
  SDL_SemPost(m_sync_sem);
  return (m_paused);
}

// called from sync thread.  Don't call on play, or m_dont_fill race
// condition may occur.
void CVideoSync::flush_sync_buffers (void)
{
  // Just restart decode thread if waiting...
  m_dont_fill = 1;
  m_eof_found = 0;
  m_paused = 1;
  if (m_decode_waiting) {
    // player_debug_message("decode thread waiting - posted to it");
    SDL_SemPost(m_decode_sem);
    // start debug
  }
}

// called from decode thread on both stop/start.
void CVideoSync::flush_decode_buffers (void)
{
  for (int ix = 0; ix < MAX_VIDEO_BUFFERS; ix++) {
    m_buffer_filled[ix] = 0;
  }

  m_fill_index = m_play_index = 0;
  m_dont_fill = 0;
}

void CVideoSync::set_screen_size (int scaletimes2)
{
  m_video_scale = scaletimes2;
}

void CVideoSync::do_video_resize (void)
{
  int w, h;
  if (m_video_scale == 1) {
    w = m_width / 2;
    h = m_height / 2;
  } else {
    w = m_width * m_video_scale / 2;
    h = m_height * m_video_scale / 2;
  }
  m_screen = SDL_SetVideoMode(w, h, m_video_bpp, 
			      SDL_SWSURFACE | SDL_ASYNCBLIT | SDL_RESIZABLE);
  m_dstrect.x = 0;
  m_dstrect.y = 0;
  m_dstrect.w = m_screen->w;
  m_dstrect.h = m_screen->h;
}
/* end file video.cpp */    
