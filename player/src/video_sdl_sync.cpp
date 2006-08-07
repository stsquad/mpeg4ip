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
#include "player_session.h"
#include "video_sdl_sync.h"
#include "video_sdl.h"
#include "player_util.h"
#include "SDL_error.h"
#include "SDL_syswm.h"
#include "our_config_file.h"

//#define VIDEO_SYNC_FILL 1

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(video_message, "videosync")
#else
#define video_message(loglevel, fmt...) message(loglevel, "videosync", fmt)
#endif

/*
 * CSDLVideoSync - actually a ring buffer for YUV frames - probably
 * can pull it out if you want to replace SDL
 */

CSDLVideoSync::CSDLVideoSync (CPlayerSession *psptr,
			      void *video_persistence,
			      int screen_pos_x, 
			      int screen_pos_y) : 
  CVideoSync(psptr, video_persistence, screen_pos_x, screen_pos_y)
{
  m_sdl_video = NULL;
  for (int ix = 0; ix < MAX_VIDEO_BUFFERS; ix++) {
    m_y_buffer[ix] = NULL;
    m_u_buffer[ix] = NULL;
    m_v_buffer[ix] = NULL;
  }

  m_video_scale = 2;
  m_msec_per_frame = 100;
  m_fullscreen = false;
  m_filled_frames = 0;
  video_message(LOG_DEBUG, "persistence is %p", video_persistence);
#ifdef WRITE_YUV
  m_outfile = fopen("raw.yuv", FOPEN_WRITE_BINARY);
#endif
  initialize_indexes(MAX_VIDEO_BUFFERS);
}

CSDLVideoSync::~CSDLVideoSync (void)
{
  if ((m_grabbed_video_persistence == 0) &&
      (m_sdl_video != NULL)){
    if (m_fullscreen) {
      m_sdl_video->set_screen_size(0, 2);
    }
    video_message(LOG_ERR, "deleteing video sdl");
    delete m_sdl_video;
  } else {
    if (m_sdl_video != NULL) 
      m_sdl_video->blank_image();
  }
  for (uint ix = 0; ix < m_max_buffers; ix++) {
    if (m_y_buffer[ix] != NULL) {
      free(m_y_buffer[ix]);
      m_y_buffer[ix] = NULL;
    }
    if (m_u_buffer[ix] != NULL) {
      free(m_u_buffer[ix]);
      m_u_buffer[ix] = NULL;
    }
    if (m_v_buffer[ix] != NULL) {
      free(m_v_buffer[ix]);
      m_v_buffer[ix] = NULL;
    }
  }
#ifdef WRITE_YUV
  if (m_outfile != NULL) {
    fclose(m_outfile);
  }
#endif

}

/*
 * CSDLVideoSync::config - routine for the codec to call to set up the
 * width, height and frame_rate of the video
 */
void CSDLVideoSync::configure (int w, int h, double aspect_ratio)
{
  m_width = w;
  m_height = h;
  m_aspect_ratio = aspect_ratio;
  for (int ix = 0; ix < MAX_VIDEO_BUFFERS; ix++) {
    m_y_buffer[ix] = (uint8_t *)malloc(w * h * sizeof(uint8_t));
    m_u_buffer[ix] = (uint8_t *)malloc(w/2 * h/2 * sizeof(uint8_t));
    m_v_buffer[ix] = (uint8_t *)malloc(w/2 * h/2 * sizeof(uint8_t));
  }
  m_config_set = true;
  video_message(LOG_DEBUG, "video configured");
}

/*
 * CSDLVideoSync::initialize_video - Called from sync task to initialize
 * the video window
 */  
int CSDLVideoSync::initialize (const char *name)
{
  if (m_initialized == false) {
    if (m_config_set) {
      if (m_sdl_video == NULL) {
	if (m_video_persistence != NULL) {
	  video_message(LOG_DEBUG, "initializing with video perist of %p", 
			m_video_persistence);
	  m_sdl_video = (CSDLVideo *)m_video_persistence;
	} else {
	  m_sdl_video = new CSDLVideo(m_screen_pos_x, m_screen_pos_y);
	  m_video_persistence = m_sdl_video;
	}
      }
      m_sdl_video->set_name(name);
      m_sdl_video->set_image_size(m_width, m_height, m_aspect_ratio);
      m_sdl_video->set_screen_size(m_fullscreen, m_video_scale);
      m_initialized = true;
      return (1);
    } else {
      return (0);
    }
  }
  return (1);
}

/*
 * CSDLVideoSync::is_video_ready - called from sync task to determine if
 * we have sufficient number of buffers stored up to start displaying
 */
bool CSDLVideoSync::is_ready (uint64_t &disptime)
{
  disptime = m_play_this_at[m_play_index];
  if (m_dont_fill) {
    return false;
  }
  if (config.GetBoolValue(CONFIG_SHORT_VIDEO_RENDER)) {
    return (m_buffer_filled[m_play_index]);
  }
  return (m_buffer_filled[(m_play_index + 2*m_max_buffers/3) % m_max_buffers]);
}



/*
 * get_video_buffer - give the decoder direct access to the YUV
 * ring buffers, so it can write into that memory
 */
int CSDLVideoSync::get_video_buffer(uint8_t **y,
				    uint8_t **u,
				    uint8_t **v)
{
  if (have_buffer_to_fill() == false) {
    return (0);
  }

  *y = m_y_buffer[m_fill_index];
  *u = m_u_buffer[m_fill_index];
  *v = m_v_buffer[m_fill_index];
  return (1);
}

/*
 * filled_video_buffers - API routine called when decoder has filled a 
 * buffer after it called get_video_buffer
 */
void CSDLVideoSync::filled_video_buffers (uint64_t time)
{
  int ix;
  if (dont_fill())
    return;
  m_play_this_at[m_fill_index] = time;
  m_buffer_filled[m_fill_index] = true;
  ix = m_fill_index;
#ifdef WRITE_YUV
  fwrite(m_y_buffer[ix], m_width * m_height, 1, m_outfile);
  fwrite(m_u_buffer[ix], (m_width * m_height) / 4, 1, m_outfile);
  fwrite(m_v_buffer[ix], (m_width * m_height) / 4, 1, m_outfile);
#endif
  increment_fill_index();

  save_last_filled_time(time);
  m_psptr->wake_sync_thread();
#ifdef VIDEO_SYNC_FILL
  video_message(LOG_DEBUG, "Filled "U64" %d", time, ix);
#endif
}

/*
 * CSDLVideoSync::set_video_frame - called from decoder to indicate a new
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
void CSDLVideoSync::set_video_frame(const uint8_t *y, 
				    const uint8_t*u, 
				    const uint8_t *v,
				    int pixelw_y, 
				    int pixelw_uv, 
				    uint64_t time)
{
  uint8_t *dst;
  const uint8_t *src;
  unsigned int ix;

  if (have_buffer_to_fill() == false) {
    return;
  }  

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
  m_buffer_filled[m_fill_index] = true;
  ix = m_fill_index;
#ifdef WRITE_YUV
  fwrite(m_y_buffer[ix], m_width * m_height, 1, m_outfile);
  fwrite(m_u_buffer[ix], (m_width * m_height) / 4, 1, m_outfile);
  fwrite(m_v_buffer[ix], (m_width * m_height) / 4, 1, m_outfile);
#endif
  increment_fill_index();
  save_last_filled_time(time);
  m_psptr->wake_sync_thread();
#ifdef VIDEO_SYNC_FILL
  video_message(LOG_DEBUG, "filled "U64" %d", time, ix);
#endif
  return;
}


void CSDLVideoSync::render (uint32_t play_index)
{
  m_sdl_video->display_image(m_y_buffer[play_index],
			     m_u_buffer[play_index],
			     m_v_buffer[play_index]);
}

void CSDLVideoSync::set_screen_size (int scaletimes2)
{
  m_video_scale = scaletimes2;
}

void CSDLVideoSync::set_fullscreen (bool fullscreen)
{
  m_fullscreen = fullscreen;
}

void CSDLVideoSync::do_video_resize (int pixel_width, 
				     int pixel_height, 
				     int max_width, 
				     int max_height)
{

  if (m_sdl_video != NULL) {
    m_sdl_video->set_screen_size(m_fullscreen, m_video_scale, 
				 pixel_width, pixel_height,
				 max_width, max_height);
  }
}

static void c_video_configure (void *ifptr,
			       int w,
			       int h,
			       int format,
			       double aspect_ratio)
{
  // asdf - ignore format for now
  ((CSDLVideoSync *)ifptr)->configure(w, h, aspect_ratio);
}

static int c_video_get_buffer (void *ifptr, 
			       uint8_t **y,
			       uint8_t **u,
			       uint8_t **v)
{
  return (((CSDLVideoSync *)ifptr)->get_video_buffer(y, u, v));
}

static void c_video_filled_buffer(void *ifptr, uint64_t time)
{
  ((CSDLVideoSync *)ifptr)->filled_video_buffers(time);
}

static void c_video_have_frame (void *ifptr,
			       const uint8_t *y,
			       const uint8_t *u,
			       const uint8_t *v,
			       int m_pixelw_y,
			       int m_pixelw_uv,
			       uint64_t time)
{
  CSDLVideoSync *foo = (CSDLVideoSync *)ifptr;

  foo->set_video_frame(y, 
		       u, 
		       v, 
		       m_pixelw_y,
		       m_pixelw_uv,
		       time);
}

static video_vft_t video_vft = 
{
  message,
  c_video_configure,
  c_video_get_buffer,
  c_video_filled_buffer,
  c_video_have_frame,
  NULL,
};

video_vft_t *get_video_vft (void)
{
  video_vft.pConfig = &config;
  return (&video_vft);
}

CVideoSync *create_video_sync (CPlayerSession *psptr) 
{
  return new CSDLVideoSync(psptr, psptr->get_video_persistence(),
			   psptr->m_screen_pos_x, 
			   psptr->m_screen_pos_y);
}



