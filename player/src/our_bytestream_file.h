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

#include "codec_plugin.h"
#include "our_bytestream.h"

class COurInByteStreamFile : public COurInByteStream
{
 public:
  COurInByteStreamFile(codec_plugin_t *plugin,
		       codec_data_t *plugin_data,
		       double max_time);
  ~COurInByteStreamFile();
  int eof(void);
  void reset(void);
  bool have_frame(void) {
    return !m_plugin->c_raw_file_has_eof(m_plugin_data);
  };
  bool start_next_frame(uint8_t **buffer, uint32_t *buflen,
			frame_timestamp_t *ts, void **userdata);
  void used_bytes_for_frame(uint32_t bytes);
  double get_max_playtime (void) { return m_max_play_time; };
  void play(uint64_t start);
 private:
  codec_plugin_t *m_plugin;
  codec_data_t *m_plugin_data;
  double m_max_play_time;
};

#endif
