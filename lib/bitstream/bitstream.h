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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__ 1
#include "systems.h"

class CBitstream {
 public:
  CBitstream(void) {};
  ~CBitstream (void) {};
  void init(const unsigned char *buffer, uint32_t bit_len);
  void init(const char *buffer, uint32_t bit_len) {
    init((const unsigned char *)buffer, (uint32_t)bit_len);
  };
  void init(const char *buffer, int bit_len) {
    init((const unsigned char *)buffer, (uint32_t)bit_len);
  };
  void init(const unsigned char *buffer, int bit_len) {
    init(buffer, (uint32_t)bit_len);
  };
  void init(const char *buffer, unsigned short bit_len) {
    init((const unsigned char *)buffer, (uint32_t)bit_len);
  };
  void init(const unsigned char *buffer, unsigned short bit_len) {
    init(buffer, (uint32_t)bit_len);
  };
  int getbits(uint32_t bits, uint32_t &retvalue);
  int peekbits(uint32_t bits, uint32_t &retvalue) {
    int ret;
    bookmark(1);
    ret = getbits(bits, retvalue);
    bookmark(0);
    return (ret);
  }
  void bookmark(int on);
  int bits_remain (void) {
    return m_chDecBufferSize + m_uNumOfBitsInBuffer;
  };
  int byte_align(void);
 private:
  uint32_t m_uNumOfBitsInBuffer;
  const unsigned char *m_chDecBuffer;
  uint32_t m_chDecBufferSize;
  int m_bBookmarkOn;
  uint32_t m_uNumOfBitsInBuffer_bookmark;
  const unsigned char *m_chDecBuffer_bookmark;
  uint32_t m_chDecBufferSize_bookmark;
};

#endif
