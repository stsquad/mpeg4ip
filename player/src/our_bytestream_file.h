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

typedef struct frame_file_pos_t
{
  struct frame_file_pos_t *next;
  uint64_t timestamp;
  long file_position;
  uint64_t frames;
} frame_file_pos_t;
  
#define THROW_PAST_EOF ((int)1)
  
class COurInByteStreamFile : public COurInByteStream
{
 public:
  COurInByteStreamFile(const char *filename);
  ~COurInByteStreamFile();
  int eof(void);
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int bSet); 
  void reset(void);
  int have_no_data(void) { return eof(); };
  uint64_t start_next_frame(void);
  double get_max_playtime (void) { return m_max_play_time; };
  void set_start_time(uint64_t start);
  ssize_t read(unsigned char *buffer, size_t bytes);
  ssize_t read(char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };
  const char *get_throw_error(int error);
  int throw_error_minor(int error);
  void config_for_file (uint64_t frame_per_sec, double max_time = 0.0) {
    m_frame_per_sec = frame_per_sec;
    m_max_play_time = max_time;
  };
 private:
  void read_frame (void);
  const char *m_filename;
  uint64_t m_frames;
  uint64_t m_frame_per_sec;
  FILE *m_file;
  int m_bookmark_eofstate;
  uint64_t m_total, m_bookmark_total;
  unsigned char *m_buffer_on, *m_orig_buffer, *m_bookmark_buffer;
  long m_buffer_position, m_bookmark_loaded_position;
  uint32_t m_buffer_size, m_bookmark_buffer_size;
  uint32_t m_bookmark_loaded, m_bookmark_loaded_size;
  uint32_t m_buffer_size_max;
  uint32_t m_index, m_bookmark_index;
  int m_bookmark;
  frame_file_pos_t *m_file_pos_head, *m_file_pos_tail;
  double m_max_play_time;
};

#endif
