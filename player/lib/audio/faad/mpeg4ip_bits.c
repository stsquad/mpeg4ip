#include <assert.h>
#include <stdio.h>
#include "bits.h"
/* to mask the n least significant bits of an integer */

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

/* initialize buffer, call once before first getbits or showbits */

void faad_initbits (bitfile *ld, char *buffer)
{
  ld->m_total += ld->m_processed_bits;
  ld->m_processed_bits = 0;
}

void faad_init_bytestream (bitfile *ld,
			   get_t get,
			   bookmark_t book,
			   void *userdata)
{
  ld->bs_get_routine = get;
  ld->bs_bookmark = book;
  ld->bs_userdata = userdata;
  ld->m_iBitPosition=0;
  ld->m_iBuffer=0;
  ld->m_lCounter=0;
  ld->m_bBookmarkOn = 0;
  ld->m_chDecBuffer = 0;
  ld->m_uNumOfBitsInBuffer = 0;
  ld->m_total = 0;
}

void faad_bookmark (bitfile *ld, int bSet)
{

  if(bSet) {
    (ld->bs_bookmark)(ld->bs_userdata, bSet);
    ld->m_uNumOfBitsInBuffer_bookmark = ld->m_uNumOfBitsInBuffer;
    ld->m_chDecBuffer_bookmark = ld->m_chDecBuffer;
    ld->m_lCounter_bookmark = ld->m_lCounter;
    ld->m_processed_bits_bookmark = ld->m_processed_bits;
    ld->m_bBookmarkOn = 1;
  }
  else {
    (ld->bs_bookmark)(ld->bs_userdata, bSet);
    ld->m_uNumOfBitsInBuffer = ld->m_uNumOfBitsInBuffer_bookmark; 
    ld->m_chDecBuffer = ld->m_chDecBuffer_bookmark; 
    ld->m_lCounter = ld->m_lCounter_bookmark;
    ld->m_processed_bits = ld->m_processed_bits_bookmark;
    ld->m_bBookmarkOn = 0;
  }
}

/* return next n bits (right adjusted) without advancing */

unsigned int faad_showbits (bitfile *ld, int n)
{
  unsigned int retData;
  faad_bookmark(ld, 1);
  retData = faad_getbits(ld, n);
  faad_bookmark(ld, 0);
  return (retData);
}

/* advance by n bits */

void faad_flushbits (bitfile *ld, int n)
{
  faad_getbits(ld, n);
}

/* return next n bits (right adjusted) */

unsigned int faad_getbits (bitfile *ld, unsigned int numBits)
{
  unsigned int retData;
  assert (numBits <= 32);
  if (numBits == 0) return 0;
	
  if (ld->m_uNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
    ld->m_uNumOfBitsInBuffer -= numBits;
    retData = ld->m_chDecBuffer >> ld->m_uNumOfBitsInBuffer;
    // wmay - this gets done below...retData &= msk[numBits];
  } else {
    int nbits;
    nbits = numBits - ld->m_uNumOfBitsInBuffer;
    if (nbits == 32)
      retData = 0;
    else
      retData = ld->m_chDecBuffer << nbits;
    switch ((nbits - 1) / 8) {
    case 3:
      nbits -= 8;
      retData |= (ld->bs_get_routine)(ld->bs_userdata) << nbits;
      // fall through
    case 2:
      nbits -= 8;
      retData |= (ld->bs_get_routine)(ld->bs_userdata) << nbits;
    case 1:
      nbits -= 8;
      retData |= (ld->bs_get_routine)(ld->bs_userdata) << nbits;
    case 0:
      break;
    }
    ld->m_chDecBuffer = (ld->bs_get_routine)(ld->bs_userdata);
    ld->m_uNumOfBitsInBuffer = 8 - nbits;
    retData |= (ld->m_chDecBuffer >> ld->m_uNumOfBitsInBuffer) & msk[nbits];
  }
  ld->m_processed_bits += numBits;
  return retData & msk[numBits];
}

// Purpose: look nbit forward for an alignment
int faad_byte_align (bitfile *ld) 
{
  int temp;
  temp = ld->m_uNumOfBitsInBuffer % 8;
  faad_getbits(ld, temp);
  return (temp);
}

int faad_get_processed_bits (bitfile *ld)
{
  return (ld->m_processed_bits);
}

void faad_bitdump (bitfile *ld)
{
  #if 0
  printf("processed %d %d bits left - %d\n",
	 ld->m_total / 8,
	 ld->m_total % 8,
	 ld->m_uNumOfBitsInBuffer);
  #endif
}
