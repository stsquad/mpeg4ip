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

#include "systems.h"
#include "our_bytestream.h"

class COurInByteStreamFile : public COurInByteStream
{
 public:
  COurInByteStreamFile(CPlayerMedia *m, const char *filename);
  ~COurInByteStreamFile();
  int eof(void);
  char get(void);
  char peek(void);
  void bookmark(int bSet); 
  void reset(void);
  int have_no_data(void) { return eof(); };
  uint64_t start_next_frame(void);
  double get_max_playtime (void) { return 0.0; };
  void set_start_time(uint64_t start);
  size_t read(char *buffer, size_t bytes);
  size_t read(unsigned char *buffer, size_t bytes) {
    return (read((char *)buffer, bytes));
  };
	    
  void config_for_file (uint64_t frame_per_sec) {
    m_frame_per_sec = frame_per_sec;
  };
 private:
  void read_frame (void);
  const char *m_filename;
  uint64_t m_frames;
  uint64_t m_frame_per_sec;
  FILE *m_file;
  int m_bookmark_eofstate;
  uint64_t m_total, m_bookmark_total;
  char *m_buffer_on, *m_orig_buffer, *m_bookmark_buffer;
  size_t m_buffer_size, m_bookmark_buffer_size;
  size_t m_bookmark_loaded, m_bookmark_loaded_size;
  size_t m_buffer_size_max;
  size_t m_index, m_bookmark_index;
  int m_bookmark;
};

#endif
