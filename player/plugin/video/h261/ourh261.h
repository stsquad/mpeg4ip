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
 * ourh261.h - interface to h261 library
 */
#ifndef __H261_H__
#define __H261_H__ 1
#include "codec_plugin.h"
#include "p64.h"

#define m_vft c.v.video_vft
#define m_ifptr c.ifptr

typedef struct h261_codec_t {
  codec_data_t c;
  P64Decoder *m_decoder;
  int m_h;
  int m_w;
  int m_video_initialized;
  int m_did_pause;
  uint64_t cached_ts;
  int m_initialized;
} mpeg3_codec_t;
  
#endif
/* end file h261.h */
