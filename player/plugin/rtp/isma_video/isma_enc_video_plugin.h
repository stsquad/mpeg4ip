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
 * Copyright (C) Cisco Systems Inc. 2000-2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *              Alex Vanzella   wmay@cisco.com
 */

#ifndef __ISMA_ENC_VIDEO_RTP_PLUGIN_H__
#define __ISMA_ENC_VIDEO_RTP_PLUGIN_H__ 1
#include "rtp_plugin.h"
#include "mp4util/mpeg4_sdp.h"
#include <ismacryplib.h>

typedef struct isma_enc_video_rtp_data_t {
  rtp_plugin_data_t plug;

#ifdef R2429_RTP_DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  uint8_t *m_buffer;
  uint32_t m_buffer_len;
  uint32_t m_buffer_len_max;

  ismacryp_session_id_t  myEncSID;
  uint32_t               frameCount;
  fmtp_parse_t            *mp4SDP;
  
} isma_enc_video_rtp_data_t;


#define m_vft plug.vft
#define m_ifptr plug.ifptr
#endif

