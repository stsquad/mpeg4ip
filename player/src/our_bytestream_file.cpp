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
#include <fstream.h>
#include "player_util.h"

COurInByteStreamFile::COurInByteStreamFile (CPlayerMedia *m,
					    const char *filename) :
  COurInByteStream(m)
{
  m_filename = strdup(filename);
  m_pInStream = new ifstream(m_filename, ios::bin | ios::in);
  m_frames = 0;
  m_total = 0;
}

COurInByteStreamFile::~COurInByteStreamFile(void)
{
  if (m_pInStream) {
    delete m_pInStream;
    m_pInStream = NULL;
  }
  if (m_filename) {
    free((void *)m_filename);
    m_filename = NULL;
  }
  player_debug_message("bytes %llu, frames %llu, fps %llu",
		       m_total, m_frames, m_frame_per_sec);
  player_debug_message("bits per sec %llu", 
		       (m_total * 8 * m_frame_per_sec) / m_frames);
}

uint64_t COurInByteStreamFile::start_next_frame (void) 
{
  uint64_t ret;
  ret = (m_frames * 1000) / m_frame_per_sec;
  m_frames++;
  ret += m_play_start_time;
  return (ret);
}
