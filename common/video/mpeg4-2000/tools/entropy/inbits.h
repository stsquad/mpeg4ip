/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: inbits.h,v 1.4 2005/01/07 19:49:41 wmaycisco Exp $
 */

#ifndef __BITS_H__
#define __BITS_H__ 1

#ifdef WIN32
typedef unsigned __int32 uint32_t;
#define close _close
#define open _open
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#ifdef PACKAGE_BUGREPORT
#define TEMP_PACKAGE_BUGREPORT PACKAGE_BUGREPORT
#define TEMP_PACKAGE_NAME PACKAGE_NAME
#define TEMP_PACKAGE_STRING PACKAGE_STRING
#define TEMP_PACKAGE_TARNAME PACKAGE_TARNAME
#define TEMP_PACKAGE_VERSION PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include "mpeg4-2000.h"
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#define PACKAGE_BUGREPORT TEMP_PACKAGE_BUGREPORT
#define PACKAGE_NAME TEMP_PACKAGE_NAME
#define PACKAGE_STRING TEMP_PACKAGE_STRING
#define PACKAGE_TARNAME TEMP_PACKAGE_TARNAME
#define PACKAGE_VERSION TEMP_PACKAGE_VERSION
#else
#include "mpeg4-2000.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

extern unsigned int bit_msk[33];
#define INLINE __inline

class CInBitStream
{
 public:
  CInBitStream(void);
  CInBitStream(int istrm);
  ~CInBitStream();
  void init(void);

  int eof() const { return m_orig_buflen == 0; };

  void set_buffer(unsigned char *bptr, uint32_t blen);

  int get_used_bytes(void) { return m_framebits / 8; };
  int get_used_bits(void) { return m_framebits; } ;
  uint32_t getBits(uint32_t numBits);
  void start_dump(void) { m_output_stuff = 1; };
  void stop_dump(void) { m_output_stuff = 0; };
#define _SWAP(a) ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])
  INLINE uint32_t peekBits(uint32_t numBits)
    {
      uint32_t rbit;
      uint32_t b;
      uint32_t ret_value;

      if (numBits == 0) return 0;
      check_buffer(numBits);
  
      rbit = 32 - m_bitcnt;
      b = _SWAP(m_rdptr);
      if (rbit < numBits) {
	b <<= m_bitcnt;
	b |= m_rdptr[4] >> (8 - m_bitcnt);
	ret_value = (b >> (32 - numBits)) & bit_msk[numBits];
      } else 
	ret_value = ((b & bit_msk[rbit]) >> (rbit-numBits));
      if (m_output_stuff != 0) {
	printf("peek %d %x\n", numBits, ret_value);
      }
      return ret_value;
    };

  int peekOneBit(uint32_t numBits);

  int peekBitsTillByteAlign(int &nBitsToPeek);
  
  int peekBitsFromByteAlign(int nBitsToPeek);
  
  Void flush (int nExtraBits = 0);

  INLINE Void setBookmark (void) {
    assert(m_bookmark == 0);
    m_bookmark_rdptr = m_rdptr;
    m_bookmark_bitcnt = m_bitcnt;
    m_bookmark_framebits = m_framebits;
    m_bookmark = 1;
  };

  INLINE Void gotoBookmark (void) {
    assert(m_bookmark == 1);
    m_rdptr = m_bookmark_rdptr;
    m_bitcnt = m_bookmark_bitcnt;
    m_framebits = m_bookmark_framebits;
    m_bookmark = 0;
  };

  void bookmark (Bool bSet);

 private:
  int m_pistrm;
  unsigned char *m_buffer;
  unsigned char *m_rdptr, *m_bookmark_rdptr;
  int m_bitcnt, m_bookmark_bitcnt;
  int m_framebits, m_bookmark_framebits;
  int m_framebits_max;
  uint32_t m_orig_buflen;
  int m_bookmark;
  int m_output_stuff;
  INLINE void check_buffer (int n) {
    int cmp;
      
    cmp = m_framebits + n;
    if (cmp > m_framebits_max) {
      if (m_pistrm < 0) {
	throw ((int)1);
      } else {
	read_ifstream_buffer();
      }
    }
  };

  INLINE void flushbits(int n)
    {
      m_bitcnt += n;

      if (m_bitcnt >= 8) {
	m_rdptr += (m_bitcnt>>3);
	m_bitcnt &= 7;
      }

      m_framebits += n;
      if (m_output_stuff != 0) {
	printf("Used %d\n", n);
      }
    };
  void read_ifstream_buffer(void);
};
#endif
