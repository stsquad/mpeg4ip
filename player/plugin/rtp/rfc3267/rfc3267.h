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
 * player_rtp_bytestream.h - provides an RTP bytestream for the codecs
 * to access
 */

#ifndef __RFC3267_PLUGIN_H__
#define __RFC3267_PLUGIN_H__ 1
#include "rtp_plugin.h"
//#include <mp4av/mp4av.h>

typedef struct rfc3267_data_t {
  rtp_plugin_data_t plug;

#ifdef RFC3267_DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  rtp_packet *m_pak_on;
  uint32_t m_pak_frame_on; // frame in the TOC that we're looking at
  uint32_t m_pak_frame_offset; // offset in rtp packet for frame
  uint32_t m_rtp_ts_add; 
  bool m_amr_is_wb;
  uint64_t m_ts;
  uint8_t m_frame[61 * 2];
} rfc3267_data_t;

#define m_vft plug.vft
#define m_ifptr plug.ifptr
#endif

