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
 * our_bytestream.h - base class for access to various "getbyte" facilities
 * such as rtp, file, quicktime file, memory.
 *
 * virtual routines here will allow any type of byte stream to be read
 * by the codecs.
 */
#ifndef __OUR_BYTESTREAM_H__
#define __OUR_BYTESTREAM_H__ 1
#include <assert.h>
#include "mpeg4ip.h"
#include "codec_plugin.h"

class COurInByteStream
{
 public:
  // name should be name for displays - not freed at close
  COurInByteStream(const char *name) {
    m_name = name;
  };
  virtual ~COurInByteStream() {};
  // eof - True if we're got end of file
  virtual int eof(void) = 0;
  // reset vector
  virtual void reset(void) = 0;
  // have_frame - return TRUE if we have a frame ready
  virtual bool have_frame (void) { return true; };
  /*
   * start_next_frame - return timestamp value, pointer to buffer start, length
   * in buffer
   */
  virtual bool start_next_frame(uint8_t **buffer, 
				uint32_t *buflen,
				frame_timestamp_t *ts,
				void **userdata) = 0;
  /*
   * used_bytes_for_frame - called after we process a frame with the number
   * of bytes used during decoding
   */
  virtual void used_bytes_for_frame(uint32_t bytes) = 0;
  /*
   * can_skip_frame - return 1 if bytestream can skip a frame
   */
  virtual int can_skip_frame (void) { return 0; };
  /*
   * skip_next_frame - actually do it...
   */
  virtual bool skip_next_frame(frame_timestamp_t *ts, int *hasSyncFrame, uint8_t **buffer, uint32_t *buflen, void **ud) { assert(0 == 1);return false; };
  virtual double get_max_playtime (void) = 0;
  virtual void pause (void) {};
  virtual void play (uint64_t start) { m_play_start_time = start; };
 protected:
  uint64_t m_play_start_time;
  const char *m_name;
};

#endif

