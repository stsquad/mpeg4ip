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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * a52dec.h - class definition for raw audio codec
 */

#ifndef __A52DEC_H__
#define __A52DEC_H__ 1
#include "codec_plugin.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <a52dec/a52.h>
#ifdef __cplusplus
}
#endif

#define m_vft c.v.audio_vft
#define m_ifptr c.ifptr

typedef struct a52dec_codec_t {
  codec_data_t c;
  int m_initialized;
  int m_resync;
  int m_chans;
  uint64_t m_last_ts;
  uint32_t m_frames_at_ts;
  a52_state_t *m_state;
  //#define OUTPUT_TO_FILE 1
#ifdef OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
} a52dec_codec_t;

#endif
