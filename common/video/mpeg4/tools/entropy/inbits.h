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
 * $Id: inbits.h,v 1.1 2002/01/03 01:09:29 wmaycisco Exp $
 */

#ifndef __BITS_H__
#define __BITS_H__ 1
#include "systems.h"

typedef void (*get_more_bytes_t)(void *, unsigned char **, uint32_t *, uint32_t);

extern unsigned int bit_msk[33];
#define INLINE __inline

class CInBitStream
{
 public:
  CInBitStream(void);
  CInBitStream(int istrm);
  ~CInBitStream();
  void init(void);

  int eof() const { return FALSE; };

  void set_buffer(get_more_bytes_t gb,
		  void *ud, unsigned char *bptr, uint32_t blen);

  int get_used_bytes(void) { return m_framebits / 8; };
  
  uint32_t getBits(uint32_t numBits);

  INLINE uint32_t peekBits(uint32_t numBits)
    {
      int rbit;
      uint32_t b;
  
      check_buffer(numBits);
  
      rbit = 32 - m_bitcnt;
#define _SWAP(a) ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])
      b = _SWAP(m_rdptr);
      return ((b & bit_msk[rbit]) >> (rbit-numBits));
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
  get_more_bytes_t m_get_more_bytes;
  int m_pistrm;
  void *m_ud;
  unsigned char *m_buffer;
  unsigned char *m_rdptr, *m_bookmark_rdptr;
  int m_bitcnt, m_bookmark_bitcnt;
  int m_framebits, m_bookmark_framebits;
  int m_framebits_max;
  uint32_t m_orig_buflen;
  int m_bookmark;

  INLINE void check_buffer (int n) {
    int cmp;
      
    cmp = m_framebits + n;
    if (cmp > m_framebits_max) {
      if (m_pistrm < 0) {
	if (m_bookmark == 0) {
	  (m_get_more_bytes)(m_ud, &m_buffer, &m_orig_buflen,
			     (m_framebits / 8));
	  m_framebits = m_bitcnt;
	  m_rdptr = m_buffer;
	  m_framebits_max = m_orig_buflen * 8;
	}
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
    };
  void read_ifstream_buffer(void);
};
#endif
