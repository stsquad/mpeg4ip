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

#include "bitstream.h"

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

static unsigned int msk[33] =
{
  0x00000000, 0x00000001, 0x00000003, 0x00000007,
  0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
  0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
  0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
  0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
  0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
  0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
  0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
  0xffffffff
};

void CBitstream::init (const unsigned char *buffer, 
		       size_t bit_len)
{
  m_chDecBuffer = buffer;
  m_chDecBufferSize = bit_len;
  m_bBookmarkOn = 0;
  m_uNumOfBitsInBuffer = 0;
}

void CBitstream::bookmark (int bSet)
{
  if (bSet) {
    m_uNumOfBitsInBuffer_bookmark = m_uNumOfBitsInBuffer;
    m_chDecBuffer_bookmark = m_chDecBuffer;
    m_chDecBufferSize_bookmark = m_chDecBufferSize_bookmark;
    m_bBookmarkOn = 1;
  } else {
    m_uNumOfBitsInBuffer = m_uNumOfBitsInBuffer_bookmark;
    m_chDecBuffer = m_chDecBuffer_bookmark;
    m_chDecBufferSize = m_chDecBufferSize_bookmark;
    m_bBookmarkOn = 0;
  }
}

int CBitstream::getbits (size_t numBits, uint32_t &retData)
{
  if (numBits > 32) {
    return -1;
  }

  retData = 0;
  if (numBits == 0) {
    return 0;
  }
  if (m_uNumOfBitsInBuffer >= numBits) {
    m_uNumOfBitsInBuffer -= numBits;
    retData = *m_chDecBuffer >> m_uNumOfBitsInBuffer;
    retData &= msk[numBits];
  } else {
    size_t nbits;
    nbits = numBits - m_uNumOfBitsInBuffer;
    if (nbits == 32)
      retData = 0;
    else
      retData = *m_chDecBuffer << nbits;
    switch ((nbits - 1) / 8) {
    case 3:
      nbits -= 8;
      if (m_chDecBufferSize < 8) {
	return -1;
      }
      retData |= (*m_chDecBuffer++) << nbits;
      m_chDecBufferSize -= 8;
      // fall through
    case 2:
      nbits -= 8;
      if (m_chDecBufferSize < 8) {
	return -1;
      }
      retData |= (*m_chDecBuffer++) << nbits;
      m_chDecBufferSize -= 8;
    case 1:
      nbits -= 8;
      if (m_chDecBufferSize < 8) {
	return -1;
      }
      retData |= (*m_chDecBuffer++) << nbits;
      m_chDecBufferSize -= 8;
    case 0:
      break;
    }
    if (m_chDecBufferSize < nbits) {
      return (-1);
    }
    if (m_chDecBufferSize >= 8) {
      m_uNumOfBitsInBuffer = 8 - nbits;
    } else
      m_uNumOfBitsInBuffer = m_chDecBufferSize - nbits;

    retData |= (*m_chDecBuffer >> m_uNumOfBitsInBuffer) & msk[nbits];
  }
  if (m_uNumOfBitsInBuffer == 0) {
    m_chDecBuffer++;
    m_chDecBufferSize -= MIN(m_chDecBufferSize,8);
  }
  retData &= msk[numBits];
  return 0;
}

int CBitstream::byte_align(void) 
{
  int temp = 0;
  if (m_uNumOfBitsInBuffer != 0) {
    temp = m_uNumOfBitsInBuffer;
    m_uNumOfBitsInBuffer = 0;
    m_chDecBuffer++;
    m_chDecBufferSize -= MIN(m_chDecBufferSize,8);
  }
  return (temp);
}
