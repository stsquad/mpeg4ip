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

static uint32_t msk[33] =
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

void CBitstream::init (const uint8_t *buffer, 
		       uint32_t bit_len)
{
  m_chDecBuffer = buffer;
  m_chDecBufferSize = bit_len;
  m_bBookmarkOn = 0;
  m_uNumOfBitsInBuffer = 0;
}

void CBitstream::bookmark (int bSet)
{
  if (m_verbose) {
    printf("bookmark\n");
  }
  if (bSet) {
    m_uNumOfBitsInBuffer_bookmark = m_uNumOfBitsInBuffer;
    m_chDecBuffer_bookmark = m_chDecBuffer;
    m_chDecBufferSize_bookmark = m_chDecBufferSize;
    m_bBookmarkOn = 1;
    m_chDecData_bookmark = m_chDecData;
  } else {
    m_uNumOfBitsInBuffer = m_uNumOfBitsInBuffer_bookmark;
    m_chDecBuffer = m_chDecBuffer_bookmark;
    m_chDecBufferSize = m_chDecBufferSize_bookmark;
    m_chDecData = m_chDecData_bookmark;
    m_bBookmarkOn = 0;
  }
}

uint32_t CBitstream::GetBits (uint32_t numBits)
{
  uint32_t retData;

  if (numBits > 32) {
    throw BITSTREAM_TOO_MANY_BITS;
  }
  
  if (numBits == 0) {
    return 0;
  }
	
  if (m_uNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
    m_uNumOfBitsInBuffer -= numBits;
    retData = m_chDecData >> m_uNumOfBitsInBuffer;
    // wmay - this gets done below...retData &= msk[numBits];
  } else {
    uint32_t nbits;
    nbits = numBits - m_uNumOfBitsInBuffer;
    if (nbits == 32)
      retData = 0;
    else
      retData = m_chDecData << nbits;
    switch ((nbits - 1) / 8) {
    case 3:
      nbits -= 8;
      if (m_chDecBufferSize < 8) throw BITSTREAM_PAST_END;
      retData |= *m_chDecBuffer++ << nbits;
      m_chDecBufferSize -= 8;
      // fall through
    case 2:
      nbits -= 8;
      if (m_chDecBufferSize < 8) throw BITSTREAM_PAST_END;
      retData |= *m_chDecBuffer++ << nbits;
      m_chDecBufferSize -= 8;
    case 1:
      nbits -= 8;
      if (m_chDecBufferSize < 8) throw BITSTREAM_PAST_END;
      retData |= *m_chDecBuffer++ << nbits;
      m_chDecBufferSize -= 8;
    case 0:
      break;
    }
    if (m_chDecBufferSize < nbits) {
      throw BITSTREAM_PAST_END;
    }
    m_chDecData = *m_chDecBuffer++;
    m_uNumOfBitsInBuffer = MIN(8, m_chDecBufferSize) - nbits;
    m_chDecBufferSize -= MIN(8, m_chDecBufferSize);
    retData |= (m_chDecData >> m_uNumOfBitsInBuffer) & msk[nbits];
  }
  if (m_verbose)
    printf("bits %d value %x\n", numBits, retData&msk[numBits]);
  return (retData & msk[numBits]);
}

int CBitstream::byte_align(void) 
{
  int temp = 0;
  if (m_uNumOfBitsInBuffer != 0) {
    temp = GetBits(m_uNumOfBitsInBuffer);
#if 0
    temp = m_uNumOfBitsInBuffer;
    m_uNumOfBitsInBuffer = 0;
    m_chDecBuffer++;
    m_chDecBufferSize -= MIN(m_chDecBufferSize,8);
#endif
  } else {
    // if we are byte aligned, check for 0x7f value - this will indicate
    // we need to skip those bits
    uint8_t readval;
    readval = PeekBits(8);
    if (readval == 0x7f) {
      readval = GetBits(8);
    }
  }
  return (temp);
}
