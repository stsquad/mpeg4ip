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
//#define FILE_DEBUG

COurInByteStreamFile::COurInByteStreamFile (codec_plugin_t *plugin,
					    codec_data_t *plugin_data,
					    double max_time)
: COurInByteStream("File")
{
  m_plugin = plugin;
  m_plugin_data = plugin_data;
  m_max_play_time = max_time;
}

COurInByteStreamFile::~COurInByteStreamFile(void)
{
}

void COurInByteStreamFile::play (uint64_t start) 
{
  if (m_plugin->c_raw_file_seek_to != NULL)
  m_plugin->c_raw_file_seek_to(m_plugin_data, start);
}

bool COurInByteStreamFile::start_next_frame (uint8_t **buffer, 
					     uint32_t *buflen,
					     frame_timestamp_t *ts,
					     void **userdata) 
{
  *buflen = m_plugin->c_raw_file_next_frame(m_plugin_data,
					    buffer, 
					    ts);

  return *buflen != 0;
}
					    
void COurInByteStreamFile::used_bytes_for_frame (uint32_t bytes)
{

  if (m_plugin->c_raw_file_used_for_frame != NULL) {
    m_plugin->c_raw_file_used_for_frame(m_plugin_data, bytes);
  }
}

int COurInByteStreamFile::eof (void) 
{ 
  return (!have_frame());
}

void COurInByteStreamFile::reset (void)
{
  if (m_plugin->c_raw_file_seek_to != NULL) 
    m_plugin->c_raw_file_seek_to(m_plugin_data, 0);
}

/* end file our_bytestream_file.cpp */





