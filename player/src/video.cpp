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
#include "player_session.h"
#include "video.h"
#include "player_util.h"
#include <SDL_syswm.h>
//#define VIDEO_SYNC_PLAY 2
//#define VIDEO_SYNC_FILL 1
//#define SHORT_VIDEO 1
#ifdef _WIN32
// new hwsurface method doesn't work on windows.
#define OLD_SURFACE 1
#endif

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(video_message, "videosync")
#else
#define video_message(loglevel, fmt...) message(loglevel, "videosync", fmt)
#endif

CVideoSync::CVideoSync (CPlayerSession *psptr)
{
  char buf[32];
  m_psptr = psptr;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0 || !SDL_VideoDriverName(buf, 1)) {
    video_message(LOG_CRIT, "Could not init SDL video: %s", SDL_GetError());
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
  m_msec_per_frame = 0;
  m_first_frame_count = 0;
  m_calculated_frame_rate = 0;
  m_consec_skipped = 0;
  m_current_time = 0;
  m_fullscreen = 0;
}

CVideoSync::~CVideoSync (void)
{
  if (m_fullscreen != 0) {
    m_fullscreen = 0;
    do_video_resize();
  }
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
  video_message(LOG_NOTICE, "Video Sync Stats:");
  video_message(LOG_NOTICE, "Displayed-behind frames %d", m_behind_frames);
  video_message(LOG_NOTICE, "Total frames displayed %d", m_total_frames);
  video_message(LOG_NOTICE, "Max behind time " LLU, m_behind_time_max);
  if (m_behind_frames > 0) 
    video_message(LOG_NOTICE, "Average behind time "LLU, m_behind_time / m_behind_frames);
  video_message(LOG_NOTICE, "Skipped rendering %u", m_skipped_render);
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
  if (frame_rate > 0 && frame_rate <= 60) {
    // this means that the value is probably bad
    m_msec_per_frame = 1000 / frame_rate;
  }
  m_config_set = 1;
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

      SDL_ShowCursor(SDL_DISABLE);
      SDL_SysWMinfo info;
      SDL_VERSION(&info.version);
      int ret;
      ret = SDL_GetWMInfo(&info);
      // Oooh... fun... To scale the video, just pass the width and height
      // to this routine. (ie: m_width *2, m_height * 2).
      int w = m_width * m_video_scale / 2;
      int h = m_height * m_video_scale / 2;
#ifdef OLD_SURFACE
      int mask = SDL_SWSURFACE | SDL_ASYNCBLIT | SDL_RESIZABLE;
#else
      int mask = SDL_HWSURFACE | SDL_RESIZABLE;
#endif
      if (m_fullscreen != 0) {
	mask |= SDL_FULLSCREEN;
      }
      m_screen = SDL_SetVideoMode(w,
				  h,
				  m_video_bpp,
				  mask);
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
#ifdef OLD_SURFACE
	if (m_video_scale == 4) {
      m_image = SDL_CreateYUVOverlay(m_width << 1, 
				     m_height << 1,
				     SDL_YV12_OVERLAY, 
				     m_screen);
	} else {
#else
	  {
#endif
      m_image = SDL_CreateYUVOverlay(m_width, 
				     m_height,
				     SDL_YV12_OVERLAY, 
				     m_screen);
	}
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
#ifdef SHORT_VIDEO
  return (m_buffer_filled[m_play_index] == 1);
#else
  return (m_buffer_filled[(m_play_index + 2*MAX_VIDEO_BUFFERS/3) % MAX_VIDEO_BUFFERS] == 1);
#endif
}

void CVideoSync::play_video (void) 
{

}

/*
 * CVideoSync::play_video_at - called from sync task to show the next
 * video frame (if the timing is right
 */
int64_t CVideoSync::play_video_at (uint64_t current_time, 
				   int &have_eof)
{
  uint64_t play_this_at;
  unsigned int ix;
  Uint8 *to, *from;
  m_current_time = current_time;

  /*
   * If the next buffer is not filled, indicate that, as well
   */
  if (m_buffer_filled[m_play_index] == 0) {
    /*
     * If we have end of file, indicate it
     */
    if (m_eof_found != 0) {
      have_eof = 1;
      return (-1);
    }
    if (current_time < m_next_time) {
      //      player_debug_message("nf %llu", m_next_time - current_time);
      return (m_next_time - current_time);
    } else {
      // Don't have a frame, and we're past the last time - delay 10 msec
      //player_debug_message("nf 10");
      return (10);
    }
  }
  
  /*
   * we have a buffer.  If it is in the future, don't play it now
   */
  play_this_at = m_play_this_at[m_play_index];
  if (play_this_at > current_time) {
#if 0
    video_message(LOG_DEBUG, 
		  "checking "LLU" at "LLU, 
		  play_this_at, current_time);
#endif
    return (play_this_at - current_time);
  }
#if VIDEO_SYNC_PLAY
  video_message(LOG_DEBUG, "play "LLU" at "LLU " %d", play_this_at, current_time,
		       m_play_index);
#endif

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
      video_message(LOG_DEBUG, "behind "LLU" avg "LLU" max "LLU,
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
#define CHECK_SYNC_TIME 1
#ifdef CHECK_SYNC_TIME
  int showed = 0;
  if ((m_msec_per_frame == 0) || 
      ((play_this_at + m_msec_per_frame) > current_time) ||
      (m_consec_skipped > 10)) {
#else
    {
#endif
      showed = 1;
      m_consec_skipped = 0;
    if (SDL_LockYUVOverlay(m_image)) {
      video_message(LOG_ERR, "Failed to lock image");
    }
    // Must always copy the buffer to memory.  This creates 2 copies of this
    // data (probably a total of 6 - libsock -> rtp -> decoder -> our ring ->
    // sdl -> hardware)
#ifdef OLD_SURFACE
    if (m_video_scale == 4) {
      // when scaling to 200%, don't use SDL stretch blit
      // use a smoothing (averaging) blit instead
#ifdef USE_MMX
      FrameDoublerMmx(m_y_buffer[m_play_index], m_image->pixels[0], 
		      m_width, m_height);
      FrameDoublerMmx(m_v_buffer[m_play_index], m_image->pixels[1], 
		      m_width >> 1, m_height >> 1);
      FrameDoublerMmx(m_u_buffer[m_play_index], m_image->pixels[2], 
		      m_width >> 1, m_height >> 1);
#else
      FrameDoubler(m_y_buffer[m_play_index], m_image->pixels[0], 
		   m_width, m_height);
      FrameDoubler(m_v_buffer[m_play_index], m_image->pixels[1], 
		   m_width >> 1, m_height >> 1);
      FrameDoubler(m_u_buffer[m_play_index], m_image->pixels[2], 
		   m_width >> 1, m_height >> 1);
#endif
    } else 
#endif
      {
	// let SDL blit, either 1:1 for 100% or decimating by 2:1 for 50%
	uint32_t bufsize = m_width * m_height * sizeof(Uint8);
	unsigned int width = m_width;

	if (width != m_image->pitches[0]) {
	  to = m_image->pixels[0];
	  from = m_y_buffer[m_play_index];
	  for (ix = 0; ix < m_height; ix++) {
	    memcpy(to, from, width);
	    to += m_image->pitches[0];
	    from += width;
	  }
	} else {
	  memcpy(m_image->pixels[0], 
		 m_y_buffer[m_play_index], 
		 bufsize);
	}
	bufsize /= 4;
	width /= 2;
	if (width != m_image->pitches[1]) {
	    to = m_image->pixels[1];
	    from = m_v_buffer[m_play_index];
	  for (ix = 0; ix < m_height; ix++) {
	    memcpy(to, from, width);
	    to += m_image->pitches[1];
	    from += width;
	  }
	} else {
	  memcpy(m_image->pixels[1], 
		 m_v_buffer[m_play_index], 
		 bufsize);
	}
	if (width != m_image->pitches[2]) {
	    to = m_image->pixels[2];
	    from = m_u_buffer[m_play_index];
	  for (ix = 0; ix < m_height; ix++) {
	    memcpy(to, from, width);
	    to += m_image->pitches[2];
	    from += width;
	  }
	} else {
	  memcpy(m_image->pixels[2], 
		 m_u_buffer[m_play_index], 
		 bufsize);
	}

      }

    int rval = SDL_DisplayYUVOverlay(m_image, &m_dstrect);
    if (rval != 0) {
      video_message(LOG_ERR, "Return from display is %d", rval);
    }
    SDL_UnlockYUVOverlay(m_image);
  } 
#ifdef CHECK_SYNC_TIME
else {
#if 0
    video_message(LOG_DEBUG, "Video lagging current time "LLU" "LLU" "LLU, 
			 play_this_at, current_time, m_msec_per_frame);
#endif
    /*
     * Else - we're lagging - just skip and hope we catch up...
     */
    m_skipped_render++;
    m_consec_skipped++;
  }
#endif
  /*
   * Advance the buffer
   */
  m_buffer_filled[m_play_index] = 0;
  m_play_index++;
  m_play_index %= MAX_VIDEO_BUFFERS;
  m_total_frames++;
  // 
  // okay - we need to signal the decode task to continue...
  if (m_decode_waiting) {
    m_decode_waiting = 0;
    SDL_SemPost(m_decode_sem);
  }

  if (m_buffer_filled[m_play_index] == 1) {
    m_next_time = m_play_this_at[m_play_index];
    if (m_next_time < m_current_time) 
      return (0);
    else 
      return (m_next_time - m_current_time);
  }
  m_next_time = play_this_at + m_msec_per_frame;
  return (m_msec_per_frame);
}

int CVideoSync::get_video_buffer(unsigned char **y,
				 unsigned char **u,
				 unsigned char **v)
{
  
  if (m_dont_fill != 0) 
    return (0);

  if (m_buffer_filled[m_fill_index] != 0) {
    m_decode_waiting = 1;
    SDL_SemWait(m_decode_sem);
    //    current_time = m_current_time;
    if (m_dont_fill != 0)
      return 0;
    if (m_buffer_filled[m_fill_index] != 0)
      return 0;
  }
  *y = m_y_buffer[m_fill_index];
  *u = m_u_buffer[m_fill_index];
  *v = m_v_buffer[m_fill_index];
  return (1);
}

int CVideoSync::filled_video_buffers(uint64_t time, uint64_t &current_time)
{
  int ix;
  if (m_dont_fill == 1)
    return 0;
  current_time = m_current_time;
  m_play_this_at[m_fill_index] = time;
  m_buffer_filled[m_fill_index] = 1;
  ix = m_fill_index;
  m_fill_index++;
  m_fill_index %= MAX_VIDEO_BUFFERS;
  
  m_psptr->wake_sync_thread();
#ifdef VIDEO_SYNC_FILL
  video_message(LOG_DEBUG, "Filled %llu %d", time, ix);
#endif
  return (1);
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
  unsigned int ix;

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
  if (m_msec_per_frame == 0) {
    // Ugly - we need to calculate the frame rate.
    if (m_first_frame_count == 0) {
      m_first_frame_time = time;
      m_first_frame_count = 1;
    } else {
      m_msec_per_frame = time - m_first_frame_time;
      m_calculated_frame_rate = 1;
      video_message(LOG_INFO, "Calculate frame rate is "LLU, m_msec_per_frame);
    }
  }
  src = y;
  dst = m_y_buffer[m_fill_index];
  for (ix = 0; ix < m_height; ix++) {
    memcpy(dst, src, m_width);
    dst += m_width;
    src += pixelw_y;
  }
  src = u;
  dst = m_u_buffer[m_fill_index];
  unsigned int uvheight = m_height/2;
  unsigned int uvwidth = m_width/2;
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
  ix = m_fill_index;
  m_fill_index++;
  m_fill_index %= MAX_VIDEO_BUFFERS;
  m_psptr->wake_sync_thread();
#ifdef VIDEO_SYNC_FILL
  video_message(LOG_DEBUG, "filled %llu %d", time, ix);
#endif
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

void CVideoSync::set_fullscreen (int fullscreen)
{
  m_fullscreen = fullscreen;
}

void CVideoSync::do_video_resize (void)
{
  int w = m_width * m_video_scale / 2;
  int h = m_height * m_video_scale / 2;
#ifdef OLD_SURFACE
  int mask = SDL_SWSURFACE | SDL_ASYNCBLIT | SDL_RESIZABLE;
#else
  int mask = SDL_HWSURFACE | SDL_RESIZABLE;
#endif

  if (m_fullscreen != 0) {
    mask |= SDL_FULLSCREEN;
  }
    
  m_screen = SDL_SetVideoMode(w, h, m_video_bpp, 
			      mask);
  m_dstrect.x = 0;
  m_dstrect.y = 0;
  m_dstrect.w = m_screen->w;
  m_dstrect.h = m_screen->h;

  SDL_FreeYUVOverlay(m_image);
#ifdef OLD_SURFACE
  if (m_video_scale == 4) {
    m_image = SDL_CreateYUVOverlay(m_width << 1, 
				 m_height << 1,
				 SDL_YV12_OVERLAY, 
				 m_screen);
  } else {
#else 
    {
#endif
    m_image = SDL_CreateYUVOverlay(m_width, 
				 m_height,
				 SDL_YV12_OVERLAY, 
				 m_screen);
  }
}

/* end file video.cpp */

