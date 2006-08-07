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

CVideoApi::CVideoApi (CPlayerSession *psptr,
			void *video_persistence,
			int screen_pos_x,
			int screen_pos_y)
{
  // set up the persistence information.  If we have a value, we
  // consider it grabbed - and it's the responsibility of the grabber
  // to free it.
  m_video_persistence = video_persistence;
  m_grabbed_video_persistence = m_video_persistence != NULL ? 1 : 0;
  m_screen_pos_x = screen_pos_x;
  m_screen_pos_y = screen_pos_y;
}

CVideoApi::~CVideoApi (void)
{
}

/*
 * CVideoApi::initialize_video - Called from sync task to initialize
 * the video window
 */  
int CVideoApi::initialize (const char *name)
{
  return (1);
}

void CVideoApi::set_screen_size (int scaletimes2)
{
}

void CVideoApi::set_fullscreen (bool fullscreen)
{
}

void CVideoApi::do_video_resize(int m_pixel_width, int m_pixel_height, int m_max_width, int m_max_height)
{
}

/* end file video.cpp */

