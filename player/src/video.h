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
#ifndef __VIDEO_H__
#define __VIDEO_H__ 1

#include "mpeg4ip_sdl_includes.h"
#include "sync.h"

#define MAX_VIDEO_BUFFERS 16

class CPlayerSession;

class CVideoApi {
 public:
  CVideoApi(CPlayerSession *ptptr, void *video_persistence = NULL,
		  int screen_pos_x = 0, int screen_pos_y = 0);
  virtual ~CVideoApi(void);

  virtual void set_screen_size(int scaletimes2); // 1 gets 50%, 2, normal, 4, 2 times
  virtual void set_fullscreen(bool fullscreen);
  virtual bool get_fullscreen (void) { return false;};

  virtual int initialize(const char *name);  // from sync task
  virtual void do_video_resize(int pixel_width = -1, int pixel_height = -1, int max_width = -1, int max_height = -1);
  virtual void set_cursor(bool val) {};

  void *get_video_persistence (void) {
    return m_video_persistence;
  };
  void set_video_persistence (void *per) {
    m_video_persistence = per;
  };
  void *grab_video_persistence (void) {
    m_grabbed_video_persistence = 1;
    return get_video_persistence();
  };
  int grabbed_video_persistence (void) {
    return m_grabbed_video_persistence;
  }
 protected:
  void *m_video_persistence;
  int m_grabbed_video_persistence;
  int m_screen_pos_x, m_screen_pos_y;

};

class CVideoSync : public CTimedSync, public CVideoApi
{
 public:
  CVideoSync(CPlayerSession *psptr, void *video_persist, 
	     int screen_pos_x, int screen_pos_y) : 
    CTimedSync("video", psptr),
    CVideoApi(psptr, video_persist, screen_pos_x, screen_pos_y)
    { m_next_video = NULL;};
  void SetNextVideo(CVideoSync *n) { m_next_video = n; } ;
  CVideoSync *GetNextVideo(void) { return m_next_video; };
  session_sync_types_t get_sync_type (void) { return VIDEO_SYNC; } ;
 protected:
  CVideoSync *m_next_video;
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
