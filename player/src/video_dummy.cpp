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
#include "video_dummy.h"
#include "player_util.h"
#include <SDL_error.h>
#include <SDL_syswm.h>

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(video_message, "videosync")
#else
#define video_message(loglevel, fmt...) message(loglevel, "videosync", fmt)
#endif

/*
 * CDummyVideoSync::config - routine for the codec to call to set up the
 * width, height and frame_rate of the video
 */
void CDummyVideoSync::config (int w, int h, double aspect_ratio)
{
  m_width = w;
  m_height = h;
  m_config_set = 1;
}

int CDummyVideoSync::get_video_buffer(uint8_t **y,
				      uint8_t **u,
				      uint8_t **v)
{
  
  if (m_y == NULL) {
    m_y = (uint8_t *)malloc(m_width * m_height * sizeof(uint8_t));
    m_u = (uint8_t *)malloc(m_width/2 * m_height/2 * sizeof(uint8_t));
    m_v = (uint8_t *)malloc(m_width/2 * m_height/2 * sizeof(uint8_t));
  }
  *y = m_y;
  *u = m_u;
  *v = m_v;
  return (1);
}

int CDummyVideoSync::filled_video_buffers (uint64_t time)
{
  video_message(LOG_DEBUG, "Filled "U64, time);
  return (1);
}

/*
 * CDummyVideoSync::set_video_frame - called from codec to indicate a new
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
int CDummyVideoSync::set_video_frame(const Uint8 *y, 
				   const Uint8 *u, 
				   const Uint8 *v,
				   int pixelw_y, 
				   int pixelw_uv, 
				   uint64_t time)
{
  video_message(LOG_DEBUG, "set_video_frame" U64, time);
  return (1);
}

void CDummyVideoSync::double_width (void)
{
}

static void c_video_configure (void *ifptr,
			      int w,
			      int h,
			      int format,
			       double aspect_ratio)
{
  // asdf - ignore format for now
  ((CDummyVideoSync *)ifptr)->config(w, h, aspect_ratio);
}

static int c_video_get_buffer (void *ifptr, 
			       uint8_t **y,
			       uint8_t **u,
			       uint8_t **v)
{
  return (((CDummyVideoSync *)ifptr)->get_video_buffer(y, u, v));
}

static int c_video_filled_buffer(void *ifptr, uint64_t time)
{
  return (((CDummyVideoSync *)ifptr)->filled_video_buffers(time));
}

static int c_video_have_frame (void *ifptr,
			       const uint8_t *y,
			       const uint8_t *u,
			       const uint8_t *v,
			       int m_pixelw_y,
			       int m_pixelw_uv,
			       uint64_t time)
{
  CDummyVideoSync *foo = (CDummyVideoSync *)ifptr;

  return (foo->set_video_frame(y, 
			       u, 
			       v, 
			       m_pixelw_y,
			       m_pixelw_uv,
			       time));
}

static video_vft_t video_vft = 
{
  message,
  c_video_configure,
  c_video_get_buffer,
  c_video_filled_buffer,
  c_video_have_frame,
};

video_vft_t *get_video_vft (void)
{
  return (&video_vft);
}

CVideoSync *create_video_sync (CPlayerSession *psptr) 
{
  return new CDummyVideoSync(psptr);
}
/* end file video.cpp */

