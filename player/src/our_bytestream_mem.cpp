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

#include "our_bytestream_mem.h"
#include "player_util.h"
#include <SDL.h>

COurInByteStreamMem::COurInByteStreamMem (const unsigned char *membuf, 
					  size_t len) :
  COurInByteStream()
{
  m_frames = 0;
  m_memptr = membuf;
  m_len = len;
  m_offset = 0;
  m_total = 0;
  m_bookmark_set = 0;
}

COurInByteStreamMem::~COurInByteStreamMem(void)
{
  player_debug_message("bytes %u, frames %llu, fps %llu",
		       m_total, m_frames, m_frame_per_sec);
  if (m_frames > 0)
    player_debug_message("bits per sec %llu", 
			 (m_total * 8 * m_frame_per_sec) / m_frames);
}

int COurInByteStreamMem::eof (void)
{
  return (m_offset >= m_len);
}

uint64_t COurInByteStreamMem::start_next_frame (void) 
{
  uint64_t ret;
  ret = (m_frames * 1000) / m_frame_per_sec;
  m_frames++;
  ret += m_play_start_time;
  return (ret);
}


unsigned char COurInByteStreamMem::get (void) 
{
  if ((m_offset >= m_len) && m_bookmark_set == 0) {
    throw "PAST END";
  }
    
  unsigned char ret = m_memptr[m_offset];
  m_offset++;
  m_total++;
  return (ret);
}

unsigned char COurInByteStreamMem::peek (void)
{
  return (m_memptr[m_offset]);
}

void COurInByteStreamMem::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark_set = 1;
    m_bookmark_offset = m_offset;
    m_bookmark_total = m_total;
  } else {
    m_bookmark_set = 0;
    m_offset = m_bookmark_offset;
    m_total = m_bookmark_total;
  }
}

size_t COurInByteStreamMem::read (unsigned char *buffer, size_t bytes)
{
  size_t diff = m_len - m_offset;
  if (diff < bytes) {
    bytes = diff;
  }
  memcpy(buffer, m_memptr, bytes);
  m_offset += bytes;
  m_total++;
  return (bytes);
}

void COurInByteStreamMem::reset (void)
{
  m_total = 0;
  m_offset = 0;
  m_bookmark_set = 0;
  m_frames = 0;
}

COurInByteStreamWav::COurInByteStreamWav (const unsigned char *membuf, 
					  size_t len) :
  COurInByteStreamMem(membuf, len) 
{
}
COurInByteStreamWav::~COurInByteStreamWav(void)
{
  SDL_FreeWAV((Uint8 *)m_memptr); 
};
