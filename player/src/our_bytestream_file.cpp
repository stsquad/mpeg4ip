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

#include "our_bytestream_file.h"
#include "player_util.h"

COurInByteStreamFile::COurInByteStreamFile (const char *filename) :
  COurInByteStream()
{
  m_filename = strdup(filename);
  m_file = fopen(m_filename, "rb");
  m_frames = 0;
  m_total = 0;
  m_bookmark_loaded = 0;
  m_bookmark = 0;
  m_buffer_size_max = 4096;
  m_buffer_size = 1;
  m_orig_buffer = (unsigned char *)malloc(m_buffer_size_max);
  m_bookmark_buffer = (unsigned char *)malloc(m_buffer_size_max);
  read_frame();
}

COurInByteStreamFile::~COurInByteStreamFile(void)
{
  if (m_file) {
    fclose(m_file);
    m_file = NULL;
  }
  if (m_filename) {
    free((void *)m_filename);
    m_filename = NULL;
  }
  player_debug_message("File bytes " LLU ", frames "LLU", fps "LLU,
		       m_total, m_frames, m_frame_per_sec);
  if (m_frames > 0)
    player_debug_message("bits per sec "LLU, 
			 (m_total * 8 * m_frame_per_sec) / m_frames);
}

void COurInByteStreamFile::set_start_time (uint64_t start) 
{
  m_frames = 0;
  m_play_start_time = start;
}

uint64_t COurInByteStreamFile::start_next_frame (void) 
{
  uint64_t ret;
  ret = (m_frames * 1000) / m_frame_per_sec;
  m_frames++;
  ret += m_play_start_time;
  return (ret);
}

void COurInByteStreamFile::read_frame (void)
{
  if (m_bookmark) {
    if (m_bookmark_loaded) {
      m_buffer_on = m_bookmark_buffer;
      m_buffer_size = m_bookmark_loaded_size;
      m_index = 0;
      return;
    }
	
    m_bookmark_loaded_size = m_buffer_size = 
		fread(m_bookmark_buffer, 1, m_buffer_size_max, m_file);
    if (m_bookmark_loaded_size > 0) {
      m_index = 0;
      m_bookmark_loaded = 1;
      m_buffer_on = m_bookmark_buffer;
    }
    return;
  }
  if (m_bookmark_loaded) {
    m_bookmark_loaded = 0;
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_buffer = m_orig_buffer;
    m_orig_buffer = m_buffer_on;
    m_buffer_size = m_bookmark_loaded_size;
    m_index = 0;
    return;
  }
  m_buffer_size = fread(m_orig_buffer, 1, m_buffer_size_max, m_file);
  m_buffer_on = m_orig_buffer;
  m_index = 0;
}

int COurInByteStreamFile::eof (void) 
{ 
  return (m_buffer_size == 0);
}

unsigned char COurInByteStreamFile::get (void) 
{
  unsigned char ret = m_buffer_on[m_index];
  m_index++;
  m_total++;
  if (m_index >= m_buffer_size) {
    read_frame();
  }
  return (ret);
}

unsigned char COurInByteStreamFile::peek (void)
{
  return (m_buffer_on[m_index]);
}

void COurInByteStreamFile::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark = 1;
    m_bookmark_buffer_size = m_buffer_size;
    m_bookmark_index = m_index;
    m_bookmark_total = m_total;
  } else {
    m_bookmark = 0;
    m_buffer_size = m_bookmark_buffer_size;
    m_index = m_bookmark_index;
    m_buffer_on = m_orig_buffer;
    m_total = m_bookmark_total;
  }
}

void COurInByteStreamFile::reset (void)
{
  fseek(m_file, 0, SEEK_SET); 
  m_bookmark_loaded = 0;
  m_bookmark = 0;
  m_frames = 0;
  read_frame();
}

size_t COurInByteStreamFile::read (unsigned char *buffer, size_t bytestoread) 
{
  if (m_bookmark && bytestoread > m_buffer_size_max) {
    // Can't do this...
    return 0;
  }
  size_t inbuffer, readbytes = 0;
  do {
    inbuffer = m_buffer_size - m_index;
    if (bytestoread < inbuffer)
      inbuffer = bytestoread;
    memcpy(buffer, &m_buffer_on[m_index], inbuffer);
    readbytes += inbuffer;
    buffer += inbuffer;
    bytestoread -= inbuffer;
    m_index += inbuffer;
    if (m_index >= m_buffer_size) {
      read_frame();
    }
    m_total += inbuffer;
  } while (bytestoread > 0 && m_buffer_size > 0);

  return (readbytes);
}
