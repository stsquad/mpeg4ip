/************************************************************************
 *
 *  getbits.c, bit level routines for tmndecode (H.263 decoder)
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *
 *  Contacts:
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *
 *  Copyright (C) 1997  University of BC, Canada
 *  Modified by: Michael Gallant <mikeg@ee.ubc.ca>
 *               Guy Cote <guyc@ee.ubc.ca>
 *               Berna Erol <bernae@ee.ubc.ca>
 *
 *  Contacts:
 *  Michael Gallant                   <mikeg@ee.ubc.ca>
 *
 *  UBC Image Processing Laboratory   http://www.ee.ubc.ca/image
 *  2356 Main Mall                    tel.: +1 604 822 4051
 *  Vancouver BC Canada V6T1Z4        fax.: +1 604 822 5949
 *
 ************************************************************************/

/* Disclaimer of Warranty
 * 
 * These software programs are available to the user without any license fee
 * or royalty on an "as is" basis. The University of British Columbia
 * disclaims any and all warranties, whether express, implied, or
 * statuary, including any implied warranties or merchantability or of
 * fitness for a particular purpose.  In no event shall the
 * copyright-holder be liable for any incidental, punitive, or
 * consequential damages of any kind whatsoever arising from the use of
 * these programs.
 * 
 * This disclaimer of warranty extends to the user of these programs and
 * user's customers, employees, agents, transferees, successors, and
 * assigns.
 * 
 * The University of British Columbia does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any
 * third-party patents.
 * 
 * Commercial implementations of H.263, including shareware, are subject to
 * royalty fees to patent holders.  Many of these patents are general
 * enough such that they are unavoidable regardless of implementation
 * design.
 * 
 */


/* based on mpeg2decode, (C) 1994, MPEG Software Simulation Group and
 * mpeg2play, (C) 1994 Stefan Eckart <stefan@lis.e-technik.tu-muenchen.de>
 * 
 */

/**
*  Copyright (C) 2001 - Project Mayo
 *
 * adapted by Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/

#include <stdlib.h>

#include "mp4_decoder.h"
#include "global.h"
#ifdef WIN32
#include <io.h>
#endif
#include <assert.h>
#ifdef LINUX
#define SE_CODE 31
#endif
void *bs_userdata;
get_t bs_get_routine;
bookmark_t bs_bookmark;

int m_lCounter;
int m_iBitPosition;
unsigned int m_iBuffer;
unsigned int m_uNumOfBitsInBuffer;
unsigned char m_chDecBuffer;
int m_bBookmarkOn;
unsigned int m_uNumOfBitsInBuffer_bookmark;
unsigned char m_chDecBuffer_bookmark;
int m_lCounter_bookmark;
int m_iBitPosition_bookmark;
unsigned int m_iBuffer_bookmark;
int m_bookmark_eofstate;

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

#ifdef _DECORE
char *decore_stream;
unsigned int decore_length;
#endif

/* initialize buffer, call once before first getbits or showbits */

void initbits (get_t get, bookmark_t book, void *userdata)
{
  bs_get_routine = get;
  bs_bookmark = book;
  bs_userdata = userdata;
  m_iBitPosition=0;
  m_iBuffer=0;
  m_lCounter=0;
  m_bBookmarkOn = 0;
  m_chDecBuffer = 0;
  m_uNumOfBitsInBuffer = 0;
}

static void bookmark (int bSet)
{

  if(bSet) {
    (bs_bookmark)(bs_userdata, bSet);
    m_uNumOfBitsInBuffer_bookmark = m_uNumOfBitsInBuffer;
    m_chDecBuffer_bookmark = m_chDecBuffer;
    m_lCounter_bookmark = m_lCounter;
    m_bBookmarkOn = 1;
  }
  else {
    (bs_bookmark)(bs_userdata, bSet);
    m_uNumOfBitsInBuffer = m_uNumOfBitsInBuffer_bookmark; 
    m_chDecBuffer = m_chDecBuffer_bookmark; 
    m_lCounter = m_lCounter_bookmark; 
    m_bBookmarkOn = 0;
  }
}

/* return next n bits (right adjusted) without advancing */

unsigned int showbits (int n)
{
  unsigned int retData;
  bookmark(1);
  retData = divx_getbits(n);
  bookmark(0);
  return (retData);
}

/* advance by n bits */

void flushbits (int n)
{
  divx_getbits(n);
}

/* return next bit (could be made faster than getbits(1)) */

unsigned int getbits1 ()
{
  return divx_getbits (1);
}

/* return next n bits (right adjusted) */

unsigned int divx_getbits (unsigned int numBits)
{
  unsigned int retData;
  assert (numBits <= 32);
  if (numBits == 0) return 0;
	
  if (m_uNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
    m_uNumOfBitsInBuffer -= numBits;
    retData = m_chDecBuffer >> m_uNumOfBitsInBuffer;
    retData &= msk[numBits];
  } else {
    int nbits;
    nbits = numBits - m_uNumOfBitsInBuffer;
    if (nbits == 32)
      retData = 0;
    else
      retData = m_chDecBuffer << nbits;
    switch ((nbits - 1) / 8) {
    case 3:
      nbits -= 8;
      retData |= (bs_get_routine)(bs_userdata) << nbits;
      // fall through
    case 2:
      nbits -= 8;
      retData |= (bs_get_routine)(bs_userdata) << nbits;
    case 1:
      nbits -= 8;
      retData |= (bs_get_routine)(bs_userdata) << nbits;
    case 0:
      break;
    }
    m_chDecBuffer = (bs_get_routine)(bs_userdata);
    m_uNumOfBitsInBuffer = 8 - nbits;
    retData |= (m_chDecBuffer >> m_uNumOfBitsInBuffer) & msk[nbits];
  }
  return retData & msk[numBits];
}

// Purpose: look nbit forward for an alignement
int __inline bytealigned(int nbit) 
{
  return (((m_uNumOfBitsInBuffer + nbit) % 8) == 0);
}

/***/
int __inline nextbits_bytealigned(int nbit)
{
  unsigned int ret;
  unsigned int skip;

  skip = m_uNumOfBitsInBuffer;
  
  if (m_uNumOfBitsInBuffer == 0) {
    bookmark(1);
    ret = divx_getbits(8);
    bookmark(0);
    if (ret == 127)
      skip += 8;
  }

  bookmark(1);
  divx_getbits(skip);
  ret = divx_getbits(nbit);
  bookmark(0);
  return (ret);
}

