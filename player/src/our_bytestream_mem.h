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

class COurInByteStreamMem : public COurInByteStream
{
 public:
  COurInByteStreamMem(const unsigned char *membuf, size_t len);
  ~COurInByteStreamMem();
  int eof(void);
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int bSet); 
  void reset(void);
  int have_no_data(void) { return eof(); };
  void config_frame_per_sec (uint64_t sample_per_buffersize) {
    m_frame_per_sec = sample_per_buffersize;
  };
  uint64_t start_next_frame(void);
  double get_max_playtime (void) { return 0.0; };
  size_t read(unsigned char *buffer, size_t read);
  size_t read(char *buffer, size_t readbytes) {
    return (read((unsigned char *)buffer, readbytes));
  }
 protected:
  const unsigned char *m_memptr;
 private:
  size_t m_offset, m_len, m_total, m_bookmark_total, m_bookmark_offset;
  uint64_t m_frames;
  uint64_t m_frame_per_sec;
  int m_bookmark_set;

};

class COurInByteStreamWav : public COurInByteStreamMem
{
public:
  COurInByteStreamWav(const unsigned char *membuf, size_t len);
  ~COurInByteStreamWav();
};

#endif
