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

#ifndef __ISMA_RTP_BYTESTREAM_H__
#define __ISMA_RTP_BYTESTREAM_H__ 1
#include "rtp_bytestream.h"
#include "player_sdp.h"
#include "bitstream/bitstream.h"
//#define ISMA_RTP_DUMP_OUTPUT_TO_FILE 1

typedef struct isma_pak_data_t {
  struct isma_pak_data_t *pak_data_next;
  rtp_packet *pak;
  char *frame_ptr;
  size_t frame_len;
  int last_in_pak;
  uint32_t rtp_timestamp;
} isma_pak_data_t;

class CIsmaAudioRtpByteStream : public CRtpByteStreamBase
{
 public:
  CIsmaAudioRtpByteStream(format_list_t *media_fmt,
			  fmtp_parse_t *fmtp,
			  unsigned int rtp_proto,
		    int ondemand,
		    uint64_t tickpersec,
		    rtp_packet **head, 
		    rtp_packet **tail,
		    int rtpinfo_received,
		    uint32_t rtp_rtptime,
		    int rtcp_received,
		    uint32_t ntp_frac,
		    uint32_t ntp_sec,
		    uint32_t rtp_ts);
  ~CIsmaAudioRtpByteStream();
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int Bset);
  void reset(void);
  uint64_t start_next_frame(void);
  size_t read(unsigned char *buffer, size_t bytes);
  size_t read(char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };

 private:
  char *m_frame_ptr;
  size_t m_offset_in_frame;
  size_t m_frame_len;
  size_t m_bookmark_offset_in_frame;
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  isma_pak_data_t *m_pak_data_head;
  isma_pak_data_t *m_pak_data_free;
  size_t m_pak_data_max;
  uint32_t m_rtp_ts_add;
  void process_packet_header(void);
  int insert_pak_data(isma_pak_data_t *pak);
  void remove_packet(rtp_packet *pak);
  isma_pak_data_t *get_pak_data (void) {
    isma_pak_data_t *pak;
    if (m_pak_data_free == NULL) {
      player_debug_message("Mallocing m_pak_data");
      pak = (isma_pak_data_t *)malloc(sizeof(isma_pak_data_t));
      if (pak == NULL) return NULL;
    } else {
      pak = m_pak_data_free;
      m_pak_data_free = pak->pak_data_next;
    }
    pak->pak_data_next = NULL;
    pak->last_in_pak = 0;
    return (pak);
  }
  CBitstream m_header_bitstream;
};

#endif

