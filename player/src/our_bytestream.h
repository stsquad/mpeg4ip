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

#include "systems.h"
#include <tools/entropy/bytestrm.hpp>

class CPlayerMedia;

class COurInByteStream : public CInByteStreamBase
{
 public:
  COurInByteStream(CPlayerMedia *m) : CInByteStreamBase()
    {m_media = m;};
  virtual ~COurInByteStream() {};
  virtual int eof(void) = 0;
  virtual char get(void) = 0;
  virtual char peek(void) = 0;
  virtual void bookmark(int bSet) = 0;
  virtual void reset(void) = 0;
  virtual int have_no_data (void) {return 0; };
  virtual uint64_t start_next_frame (void) = 0;
  virtual double get_max_playtime (void) = 0;
  virtual void set_start_time(uint64_t start) { m_play_start_time = start; };
  virtual int still_same_ts (void) { return 0; };

 protected:
  uint64_t m_play_start_time;
  CPlayerMedia *m_media;
 private:
  void audio_set_timebase(long frame);
};
#endif
