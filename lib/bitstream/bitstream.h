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
#include "mpeg4ip.h"

class CBitstream {
 public:
  CBitstream(void) { m_verbose = 0;};
  CBitstream(const uint8_t *buffer, uint32_t bit_len) {
    m_verbose = 0;
    init(buffer, bit_len);
  };
  ~CBitstream (void) {};
  void init(const uint8_t *buffer, uint32_t bit_len);
  void init(const char *buffer, uint32_t bit_len) {
    init((const uint8_t *)buffer, (uint32_t)bit_len);
  };
  void init(const char *buffer, int bit_len) {
    init((const uint8_t *)buffer, (uint32_t)bit_len);
  };
  void init(const uint8_t *buffer, int bit_len) {
    init(buffer, (uint32_t)bit_len);
  };
  void init(const char *buffer, unsigned short bit_len) {
    init((const uint8_t *)buffer, (uint32_t)bit_len);
  };
  void init(const uint8_t *buffer, unsigned short bit_len) {
    init(buffer, (uint32_t)bit_len);
  };
  uint32_t GetBits(uint32_t bits);
  int getbits(uint32_t bits, uint32_t *retvalue) {
    try {
      *retvalue = GetBits(bits);
    } catch (...) {
      return -1;
    }
    return 0;
  }
  int peekbits(uint32_t bits, uint32_t *retvalue) {
    int ret;
    bookmark(1);
    ret = getbits(bits, retvalue);
    bookmark(0);
    return (ret);
  }
  uint32_t PeekBits(uint32_t bits) {
    uint32_t ret;
    bookmark(1);
    ret = GetBits(bits);
    bookmark(0);
    return ret;
  }
  void bookmark(int on);
  int bits_remain (void) {
    return m_chDecBufferSize + m_uNumOfBitsInBuffer;
  };
  int byte_align(void);
  void set_verbose(int verbose) { m_verbose = verbose; };
 private:
  uint32_t m_uNumOfBitsInBuffer;
  const uint8_t *m_chDecBuffer;
  uint8_t m_chDecData, m_chDecData_bookmark;
  uint32_t m_chDecBufferSize;
  int m_bBookmarkOn;
  uint32_t m_uNumOfBitsInBuffer_bookmark;
  const uint8_t *m_chDecBuffer_bookmark;
  uint32_t m_chDecBufferSize_bookmark;
  int m_verbose;
};

#endif
