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

#ifndef __RFC3119_BYTESTREAM_H__
#define __RFC3119_BYTESTREAM_H__ 1
#include "mpeg4ip.h"
#include "rtp_bytestream.h"
#include <sdp/sdp.h>
#include <mp4av/mp4av.h>

typedef struct adu_data_t {
  struct adu_data_t *next_adu;
  rtp_packet *pak;
  uint8_t *frame_ptr;
  int aduDataSize;
  uint64_t timestamp;
  uint32_t freq_timestamp;
  char first_in_pak;
  char last_in_pak;
  char freeframe;
  //isma_frag_data_t *frag_data;

  int headerSize;
  int sideInfoSize;
  int backpointer;
  int framesize;
  MP4AV_Mp3Header mp3hdr;
  
  int interleave_idx;
  int cyc_ct;
} adu_data_t;

class CRfc3119RtpByteStream : public CRtpByteStreamBase
{
 public:
  CRfc3119RtpByteStream(unsigned int rtp_pt,
			format_list_t *fmt,
			int ondemand,
			uint64_t tickpersec,
			rtp_packet **head, 
			rtp_packet **tail,
			int rtp_seq_set,
			uint16_t rtp_base_seq,
			int rtp_ts_set,
			uint32_t rtp_base_ts,
			int rtcp_received,
			uint32_t ntp_frac,
			uint32_t ntp_sec,
			uint32_t rtp_ts);
  ~CRfc3119RtpByteStream();
  void reset(void);
  bool have_frame(void);
  bool start_next_frame(uint8_t **buffer, 
			uint32_t *buflen, 
			frame_timestamp_t *ts,
			void **ud);
  void used_bytes_for_frame(uint32_t byte);
  void flush_rtp_packets(void);
 protected:
  bool check_rtp_frame_complete_for_payload_type(void) {
    return m_head != NULL;
  };

 private:
#ifdef ISMA_RTP_DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
  adu_data_t *m_adu_data_free;

  adu_data_t *m_deinterleave_list;
  adu_data_t *m_ordered_adu_list;
  adu_data_t *m_pending_adu_list;

  uint32_t m_rtp_ts_add;
  uint32_t m_rtp_freq_ts_add;
  uint32_t m_freq;
  int m_recvd_first_pak;
  int m_got_next_idx;
  int m_have_interleave;

  uint8_t *m_mp3_frame;
  uint32_t m_mp3_frame_size;
  
  int insert_frame_data(adu_data_t *pak);
  adu_data_t *get_adu_data (void) {
    adu_data_t *pak;
    if (m_adu_data_free == NULL) {
      pak = MALLOC_STRUCTURE(adu_data_t);
      if (pak == NULL) return NULL;
    } else {
      pak = m_adu_data_free;
      m_adu_data_free = pak->next_adu;
    }
    pak->next_adu = NULL;
    pak->last_in_pak = 0;
    return (pak);
  }
  void free_adu(adu_data_t *adu);
  void dump_adu_list (adu_data_t *lptr) {
    adu_data_t *p;
    while (lptr != NULL) {
      p = lptr->next_adu;
      free_adu(lptr);
      lptr = p;
    }
  }
  void insert_interleave_adu(adu_data_t *adu);
  int insert_processed_adu(adu_data_t *adu);
  void process_packet(void);
  int needToGetAnADU(void);
  void add_and_insertDummyADUsIfNecessary(void);
};

#endif

