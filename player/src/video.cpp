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

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(video_message, "videosync")
#else
#define video_message(loglevel, fmt...) message(loglevel, "videosync", fmt)
#endif

CVideoSync::CVideoSync (CPlayerSession *psptr)
{
  m_psptr = psptr;
  m_decode_sem = NULL;
  m_consec_skipped = 0;
  m_behind_frames = 0;
  m_total_frames = 0;
  m_filled_frames = 0;
  m_behind_time = 0;
  m_behind_time_max = 0;
  m_skipped_render = 0;
  m_msec_per_frame = 0;
  m_last_filled_time = 0;
  m_eof_found = 0;
}

CVideoSync::~CVideoSync (void)
{
  video_message(LOG_NOTICE, "Video Sync Stats:");
  video_message(LOG_NOTICE, "Displayed-behind frames %d", m_behind_frames);
  video_message(LOG_NOTICE, "Total frames displayed %d", m_total_frames);
  video_message(LOG_NOTICE, "Max behind time " LLU, m_behind_time_max);
  if (m_behind_frames > 0) 
    video_message(LOG_NOTICE, "Average behind time "LLU, m_behind_time / m_behind_frames);
  video_message(LOG_NOTICE, "Skipped rendering %u", m_skipped_render);
  video_message(LOG_NOTICE, "Filled frames %u", m_filled_frames);
}

/*
 * CVideoSync::config - routine for the codec to call to set up the
 * width, height and frame_rate of the video
 */
void CVideoSync::config (int w, int h)
{
}

/*
 * CVideoSync::initialize_video - Called from sync task to initialize
 * the video window
 */  
int CVideoSync::initialize_video (const char *name, int x, int y)
{
  return (1);
}

/*
 * CVideoSync::is_video_ready - called from sync task to determine if
 * we have sufficient number of buffers stored up to start displaying
 */
int CVideoSync::is_video_ready (uint64_t &disptime)
{
  return 0;
}

/*
 * CVideoSync::play_video_at - called from sync task to show the next
 * video frame (if the timing is right
 */
int64_t CVideoSync::play_video_at (uint64_t current_time, 
				   int &have_eof)
{
  return (10);
}

int CVideoSync::get_video_buffer(unsigned char **y,
				 unsigned char **u,
				 unsigned char **v)
{
  return (0);
}

int CVideoSync::filled_video_buffers (uint64_t time)
{
  return 0;
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
 */
int CVideoSync::set_video_frame(const Uint8 *y, 
				const Uint8 *u, 
				const Uint8 *v,
				int pixelw_y, 
				int pixelw_uv, 
				uint64_t time)
{
  return (-1);
}

// called from sync thread.  Don't call on play, or m_dont_fill race
// condition may occur.
void CVideoSync::flush_sync_buffers (void)
{
}

// called from decode thread on both stop/start.
void CVideoSync::flush_decode_buffers (void)
{
}

void CVideoSync::set_screen_size (int scaletimes2)
{
}

void CVideoSync::set_fullscreen (int fullscreen)
{
}

void CVideoSync::do_video_resize (void)
{
}

/* end file video.cpp */

