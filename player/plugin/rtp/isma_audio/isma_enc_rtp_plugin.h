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
 *              Alex Vanzella   alexv@cisco.com
 */
/*
 * player_rtp_bytestream.h - provides an RTP bytestream for the codecs
 * to access
 */

#ifndef __ISMA_ENC_RTP_PLUGIN_H__
#define __ISMA_ENC_RTP_PLUGIN_H__ 1
#include "rtp_plugin.h"
#include "mp4util/mpeg4_sdp.h"
#include "mpeg4ip_bitstream.h"
#include <ismacryplib.h>
#include "mpeg4ip_sdl_includes.h"
// Uncomment this #define to dump rtp packets to file.
//#define ISMA_RTP_DUMP_OUTPUT_TO_FILE 1
// Uncomment this #define for fragment debug info.
//#define DEBUG_ISMA_RTP_FRAGS 1

// fragment information
typedef struct isma_frag_data_t {
  struct isma_frag_data_t *frag_data_next;
  rtp_packet *pak;
  uint8_t *frag_ptr;
  uint32_t frag_len;
} isma_frag_data_t;

typedef struct isma_frame_data_t {
  struct isma_frame_data_t *frame_data_next;
  rtp_packet *pak;
  uint8_t *frame_ptr;
  uint32_t frame_len;
  int last_in_pak;
  uint32_t rtp_timestamp;
  int is_fragment;
  isma_frag_data_t *frag_data;
  uint32_t  IV;
} isma_frame_data_t;

typedef struct isma_enc_rtp_data_t {
  rtp_plugin_data_t plug;

#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  isma_frame_data_t *m_frame_data_head;
  isma_frame_data_t *m_frame_data_on;
  isma_frame_data_t *m_frame_data_free;
  uint32_t m_frame_data_max;
  uint32_t m_rtp_ts_add;
  CBitstream m_header_bitstream;
  fmtp_parse_t *m_fmtp;
  int m_min_first_header_bits;
  int m_min_header_bits;
  uint8_t *m_frag_reass_buffer;
  uint32_t m_frag_reass_size;
  uint32_t m_frag_reass_size_max;
  SDL_mutex *m_rtp_packet_mutex;
  uint64_t m_ts;
  ismacryp_session_id_t  myEncSID;
} isma_enc_rtp_data_t;

#define m_vft plug.vft
#define m_ifptr plug.ifptr
#endif

