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
 * aa.h - class definition for AAC codec.
 */

#ifndef __AA_H__
#define __AA_H__ 1
#include "systems.h"
extern "C" {
#include <faad/faad.h>
}

#include "codec.h"
#include "player_rtp_bytestream.h"
#include "player_mem_bytestream.h"
#include "audio.h"

class CAACodec : public CAudioCodecBase {
 public:
  CAACodec(CAudioSync *a,
	   CInByteStreamBase *pbytestrm,
	   format_list_t *media_desc,
	   audio_info_t *audio,
	   const unsigned char *userdata = NULL,
	   size_t userdata_size = 0);
  ~CAACodec();
  int decode(uint64_t rtptime, int fromrtp);
  void do_pause(void);
  unsigned char read_byte(FILE_STREAM *fs, int *err);
  void reset(void);
  int peek(void *data, int len);
 private:
  FILE_STREAM *m_fs;
  faadAACInfo m_fInfo;
  CInByteStreamMem *m_local_bytestream;
  CInByteStreamBase *m_orig_bytestream;
  int m_resync_with_header;
  int m_record_sync_time;
  uint64_t m_first_time_offset;
  uint64_t m_current_time;
  uint64_t m_last_rtp_ts;
  size_t m_current_frame;
  SDL_AudioSpec m_obtained;
  int m_audio_inited;
  size_t m_local_buffersize;
  char *m_local_buffer;
  int m_freq;  // frequency
#define DUMP_OUTPUT_TO_FILE 1
#if DUMP_OUTPUT_TO_FILE
  FILE *m_outfile;
#endif
};

#endif
