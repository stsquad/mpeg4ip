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
 * video.h - contains the interface class between the codec and the video
 * display hardware.
 */
#ifndef __VIDEO_DUMMY_H__
#define __VIDEO_DUMMY_H__ 1

#include "systems.h"
//#include <type/typeapi.h>
#include "mpeg4ip_sdl_includes.h"
#include "codec_plugin.h"
#include "video.h"

#define MAX_VIDEO_BUFFERS 16

class CPlayerSession;

class CDummyVideoSync : public CVideoSync {
 public:
  CDummyVideoSync(CPlayerSession *ptptr) : CVideoSync(ptptr) {
    m_y = m_u = m_v = NULL;
  };
  int get_video_buffer(uint8_t **y,
		       uint8_t **u,
		       uint8_t **v);
  int filled_video_buffers(uint64_t time);
  int set_video_frame(const uin8_t *y,      // from codec
		      const uint8_t *u,
		      const uint8_t *v,
		      int m_pixelw_y,
		      int m_pixelw_uv,
		      uint64_t time);
  void config (int w, int h, double aspect_ratio); // from codec
  void double_width(void);
 protected:
  int m_width;
  int m_height;
  uint8_t *m_y, *m_u, *m_v;
  int m_config_set;
};

#endif
