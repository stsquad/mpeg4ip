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
 * our_bytestream_file.h - provides a raw file access as a bytestream
 */
#ifndef __OUR_BYTESTREAM_FILE_H__
#define __OUR_BYTESTREAM_FILE_H__ 1

#include <stdint.h>
#include "our_bytestream.h"

class COurInByteStreamFile : public COurInByteStream
{
 public:
  COurInByteStreamFile(CPlayerMedia *m, const char *filename);
  ~COurInByteStreamFile();
  int eof(void) { return (m_pInStream->eof()); };
  Char get(void){ m_total++; return (m_pInStream->get()); };
  Char peek(void) { return (m_pInStream->peek()); };
  void bookmark(Bool bSet){
    if (bSet) {
      m_bookmark_strmpos = m_pInStream->tellg();
      m_bookmark_eofstate = m_pInStream->eof();
      m_total_bookmark = m_total;
    } else {
      m_pInStream->seekg(m_bookmark_strmpos);
      if (m_bookmark_eofstate == 0)
	m_pInStream->clear();
      m_total = m_total_bookmark;
    }
  };
  void reset(void) {
    m_pInStream->clear();
    m_pInStream->seekg(0);
    m_frames = 0;
    m_total = 0;
  };
  int have_no_data(void) { return eof(); };
  uint64_t start_next_frame(void);
  void config_for_file (uint64_t frame_per_sec) {
    m_frame_per_sec = frame_per_sec;
  };
  double get_max_playtime (void) { return 0.0; };
 private:
  const char *m_filename;
  uint64_t m_frames;
  uint64_t m_frame_per_sec;
  istream *m_pInStream;
  streampos m_bookmark_strmpos;
  int m_bookmark_eofstate;
  uint64_t m_total, m_total_bookmark;
};

#endif
