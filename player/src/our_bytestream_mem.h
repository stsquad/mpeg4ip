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

#define THROW_MEM_PAST_END ((int) 1)
class COurInByteStreamMem : public COurInByteStream
{
 public:
  COurInByteStreamMem(const unsigned char *membuf, uint32_t len) :
    COurInByteStream("memory") {
    init(membuf, len);
  };
  COurInByteStreamMem(const char *membuf, uint32_t len) :
    COurInByteStream("memory") {
    init((const unsigned char *)membuf, len);
  };
  ~COurInByteStreamMem();
  int eof(void);
  void reset(void);
  int have_no_data(void) { return eof(); };
  void config_frame_per_sec (uint64_t sample_per_buffersize) {
    m_frame_per_sec = sample_per_buffersize;
  };
  uint64_t start_next_frame(unsigned char **buffer, uint32_t *buflen);
  void used_bytes_for_frame(uint32_t bytes);
  void get_more_bytes (unsigned char **buffer,
		       uint32_t *buflen,
		       uint32_t used,
		       int get) {
    m_offset += used;
    throw THROW_MEM_PAST_END;
  };
  double get_max_playtime (void) { return 0.0; };
  const char *get_throw_error(int error);
  // A minor error is one where in video, you don't need to skip to the
  // next I frame.
  int throw_error_minor(int error);
 protected:
  const unsigned char *m_memptr;
 private:
  void init(const unsigned char *membuf, uint32_t len);
  uint32_t m_offset, m_len, m_total;
  uint64_t m_frames;
  uint64_t m_frame_per_sec;
};

class COurInByteStreamWav : public COurInByteStreamMem
{
public:
  COurInByteStreamWav(const unsigned char *membuf, uint32_t len);
  ~COurInByteStreamWav();
};

#endif
