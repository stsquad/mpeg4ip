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
 * rawv.h - interface to rawv library
 */
#ifndef __MPEG4IP_MPEG2DEC_H__
#define __MPEG4IP_MPEG2DEC_H__ 1
#include "codec_plugin.h"
extern "C" {
#include <mpeg2dec/mpeg2.h>
}
#include "mp4av.h"

#define m_vft c.v.video_vft
#define m_ifptr c.ifptr

typedef struct mpeg2dec_codec_t {
  codec_data_t c;
  mpeg2dec_t *m_decoder;
  int m_h;
  int m_w;
  int m_video_initialized;
  int m_did_pause;
  int m_got_i;
  uint64_t cached_ts;
  bool m_cached_ts_invalid;
  mpeg3_pts_to_dts_t pts_convert;
} mpeg2dec_codec_t;
  
#endif
/* end file mpeg2dec.h */
