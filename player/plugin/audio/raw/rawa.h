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
 * rawa.h - class definition for raw audio codec
 */

#ifndef __RAWA_H__
#define __RAWA_H__ 1
#include "codec_plugin.h"

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr

typedef struct rawa_codec_t {
  codec_data_t c;
  int m_freq;  // frequency
  int m_chans; // channels
  int m_bitsperchan;
  uint32_t m_bytesperchan;
  int m_output_frame_size;
  int m_initialized;
  uint8_t *m_temp_buff;
  uint32_t m_temp_buffsize;
  uint64_t m_ts;
  uint32_t m_freq_ts;
  uint64_t m_bytes;
  int m_convert_bytes;
  uint16_t m_small_buffer;
  bool m_in_small_buffer;
} rawa_codec_t;

#endif
