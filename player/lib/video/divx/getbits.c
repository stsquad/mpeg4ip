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

unsigned char *decore_stream;
unsigned int decore_length;

/* initialize buffer, call once before first getbits or showbits */

void initbits (get_more_t get, void *ud)
{
  ld->incnt = 0;
  ld->bitcnt = 0;
  ld->rdptr = ld->rdbfr + 2048;
  ld->get = 0;
  ld->get_more = get;
  ld->ud = ud;
#ifdef _DECORE
  ld->rdptr = decore_stream;
#endif
}

void init_frame_bits (unsigned char *buffer,
		      unsigned int buflen)
{
  ld->rdptr = buffer;
  ld->buflen = buflen;
  ld->endptr = buffer + buflen;
  ld->incnt = 0;
  ld->bitcnt = 0;
}

#if 0

void fillbfr ()
{
  int l;

  ld->inbfr[0] = ld->inbfr[8];
  ld->inbfr[1] = ld->inbfr[9];
  ld->inbfr[2] = ld->inbfr[10];
  ld->inbfr[3] = ld->inbfr[11];

  if (ld->rdptr >= ld->rdbfr + 2048)
  {
    l = read (ld->infile, ld->rdbfr, 2048);
    ld->rdptr = ld->rdbfr;
    if (l < 2048)
    {
      if (l < 0)
        l = 0;

      while (l < 2048)          /* Add recognizable sequence end code */
      {
        ld->rdbfr[l++] = 0;
        ld->rdbfr[l++] = 0;
        ld->rdbfr[l++] = (1 << 7) | (31 << 2);
      }
    }
  }
  for (l = 0; l < 8; l++)
    ld->inbfr[l + 4] = ld->rdptr[l];

  ld->rdptr += 8;
  ld->incnt += 64;
}

/* return next n bits (right adjusted) without advancing */

unsigned int showbits (int n)
{
  unsigned char *v;
  unsigned int b;
  int c;

  if (ld->incnt < n)
    fillbfr ();

  v = ld->inbfr + ((96 - ld->incnt) >> 3);
  b = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
  c = ((ld->incnt - 1) & 7) + 25;
  return (b >> (c - n)) & msk[n];
}

/* advance by n bits */

void flushbits (int n)
{
  ld->bitcnt += n;
  ld->incnt -= n;

  if (ld->incnt < 0)
    fillbfr ();
}

#endif

#define _SWAP(a) ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3])

unsigned int showbits (int n)
{
	unsigned char *v;
	int rbit = 32 - ld->bitcnt;
	unsigned int b;
	if ((ld->rdptr + ((ld->bitcnt + n - 1) / 8)) >= ld->endptr) {
	  // get more here...
	  (ld->get_more)(ld->ud, &ld->rdptr,
			 &ld->buflen, ld->incnt, ld->get);
	  ld->endptr = ld->rdptr + ld->buflen;
	  ld->incnt = 0;
	}
	v = ld->rdptr;
	b = _SWAP(v);
	return ((b & msk[rbit]) >> (rbit-n));
}



/* return next bit (could be made faster than getbits(1)) */

unsigned int getbits1 ()
{
  return getbits (1);
}

/* return next n bits (right adjusted) */

unsigned int divx_getbits (unsigned int n)
{
  unsigned int l;

  ld->get = 1;
  l = showbits (n);
  ld->get = 0;
  flushbits (n);

  return l;
}

// Purpose: look nbit forward for an alignement
int __inline bytealigned(int nbit) 
{
	return (((ld->bitcnt + nbit) % 8) == 0);
}

/***/
int __inline nextbits_bytealigned(int nbit)
{
	int code;
	int skipcnt = 0;

	if (bytealigned(skipcnt))
	{
		// stuffing bits
		if (showbits(8) == 127) {
			skipcnt += 8;
		}
	}
	else
	{
	  while (! bytealigned(skipcnt)) {
	    skipcnt += 1;
	  }
	}

	code = showbits(nbit + skipcnt);
 
        return ((code << skipcnt) >> skipcnt);
}


