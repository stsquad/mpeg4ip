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
 * divx.h - interface to divx library
 */
#ifndef __DIVX_H__
#define __DIVX_H__ 1

#include <stdio.h>
#include <stdlib.h>

#include "codec.h"
#include "player_media.h"
#include "player_rtp_bytestream.h"
#include "video.h"

#define DIVX_STATE_VO_SEARCH 0
#define DIVX_STATE_NORMAL 1
#define DIVX_STATE_WAIT_I 2

class CDivxCodec: public CVideoCodecBase {
 public:
  CDivxCodec(CVideoSync *v, 
	     CInByteStreamBase *pbytestrm, 
	     format_list_t *media_fmt,
	     video_info_t *vinfo,
	     const unsigned char *userdata = NULL,
	     size_t ud_size = 0);
  ~CDivxCodec();
  int decode(uint64_t ts, int fromrtp);
  void do_pause(void);
  unsigned char get (void) { return m_bytestream->get(); };
  void bookmark (int val) { m_bytestream->bookmark(val); } ;
 private:
  int parse_vovod(const char *config, int ascii, size_t len);
  int m_nFrames;
  int m_decodeState;
  int m_dropFrame;
  uint64_t m_last_time;
  size_t m_dropped_b_frames;
  size_t m_num_wait_i;
  size_t m_num_wait_i_frames;
  size_t m_total_frames;
};
  
#endif
/* end file divx.h */
