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
 * mpeg4.h - interface to iso reference codec
 */
#ifndef __MPEG4_H__
#define __MPEG4_H__ 1

#include <stdio.h>
#include <stdlib.h>
#include <fstream.h>

#ifdef __PC_COMPILER_
#include <windows.h>
#include <mmsystem.h>
#endif // __PC_COMPILER_

#include <type/typeapi.h>
#include <sys/mode.hpp>
#include <sys/vopses.hpp>
#include <tools/entropy/bitstrm.hpp>
#include <sys/tps_enhcbuf.hpp>
#include <sys/decoder/enhcbufdec.hpp>
#include <sys/decoder/vopsedec.hpp>
#include "codec.h"
#include "player_media.h"
#include "player_rtp_bytestream.h"
#include "video.h"

#define DECODE_STATE_VO_SEARCH 0
#define DECODE_STATE_VOL_SEARCH 1
#define DECODE_STATE_NORMAL 2
#define DECODE_STATE_WAIT_I 3

class CVideoObjectDecoder;

class CMpeg4Codec: public CVideoCodecBase {
 public:
  CMpeg4Codec(CVideoSync *v, 
	      CInByteStreamBase *pbytestrm, 
	      format_list_t *media_fmt,
	      video_info_t *vinfo,
	      const char *userdata = NULL,
	      size_t ud_size = 0);
  ~CMpeg4Codec();
  int decode(uint64_t ts, int fromrtp);
  void do_pause(void);
 private:
  int parse_vovod(const char *config);
  CVideoObjectDecoder *m_pvodec;
  Bool m_main_short_video_header;
  Int m_nFrames;
  Int m_decodeState;
  Bool m_bSpatialScalability;
  Bool m_bCachedRefFrame;
  Bool m_bCachedRefFrameCoded;
  Bool m_dropFrame;
  Bool m_cached_valid;
  uint64_t m_cached_time;
  uint64_t m_last_time;
  size_t m_dropped_b_frames;
  size_t m_num_wait_i;
  size_t m_num_wait_i_frames;
  size_t m_total_frames;
};
  
#endif
/* end file mpeg4.h */
