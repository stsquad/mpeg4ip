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
 * aa.h - class definition for AAC codec.
 */

#ifndef __OURWAV_H__
#define __OURWAV_H__ 1
#include "systems.h"
#include "codec.h"
#include "audio.h"

class CWavCodec : public CAudioCodecBase {
 public:
  CWavCodec(CAudioSync *a,
	    CInByteStreamBase *pbytestrm,
	    format_list_t *media_desc,
	    audio_info_t *audio,
	    const unsigned char *userdata = NULL,
	    uint32_t userdata_size = 0);
  ~CWavCodec();
  int decode(uint64_t rtptime, int fromrtp);
  int skip_frame(uint64_t rtptime);
  void do_pause(void);
 private:
  SDL_AudioSpec *m_sdl_config;
  uint32_t m_bytes_per_sample;
};

#endif
