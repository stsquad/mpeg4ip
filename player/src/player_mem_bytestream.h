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
 * player_mem_bytestream.h - provides a memory base bytestream.
 * This can be used for initialization, or for a codec that doesn't
 * read raw rtp data.  For that, you can copy from the rtp bytestream
 * to a local memory based bytestream, then have the codec use the local one
 */
#ifndef __PLAYER_MEM_BYTESTREAM_H__
#define __PLAYER_MEM_BYTESTREAM_H__ 1
#include "systems.h"
#include <tools/entropy/bytestrm.hpp>
#include "player_util.h"


class CInByteStreamMem : public CInByteStreamBase
{
 public:
  CInByteStreamMem();
  ~CInByteStreamMem();
  int eof (void) { return m_offset >= m_len; };
  char get(void);
  char peek(void);
  void reset(void) { };
  void bookmark(int Bset);
  void set_memory (const char *membuf, size_t len) {
    m_memptr = membuf;
    m_len = len;
    m_offset = 0;
  };
 public:
  void init(void);
  size_t m_offset, m_len;
  const char *m_memptr, *m_bookmark_memptr;
  int m_bookmark_offset;
  int m_bookmark_set;
};
#endif
