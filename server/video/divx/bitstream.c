
/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  mom_bitstream.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

#include "bitstream.h"

/* to mask the n least significant bits of an integer */

static unsigned int mask[33] =
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


/* static data for pointers and counters */

/* byte pointer to the output bitstream */
static unsigned char *byteptr;
/* counter of how many bytes got written to the bitstream */
static int bytecnt; 

/* a one byte temporary buffer */
static unsigned char outbfr;
/* counter of how many unused bits left in the byte buffer */
static int outcnt;

void Bitstream_Init(void *buffer)
{
	byteptr = (unsigned char *)buffer;
	bytecnt = 0;
	outbfr = 0;
	outcnt = 8;
}

void Bitstream_PutBits(int n, unsigned int val)
{
	int diff;

	while ((diff = n - outcnt) >= 0) { /* input is longer than what is left in the buffer */
		outbfr |= (unsigned char)(val >> diff);
		/* note that the first byte of the integer is the least significant byte */
		n = diff;
		val &= mask[n];

		*(byteptr ++) = outbfr;
		bytecnt++;
		outbfr = 0;
		outcnt = 8;
	}

	if (n > 0) { /* input is short enough to fit in what is left in the buffer */
		outbfr |= (unsigned char)(val << (-diff));
		outcnt -= n;
	}
}


int Bitstream_Close()
{
	while (outcnt != 8) Bitstream_PutBits(1, 1);
	return bytecnt;
}


int Bitstream_NextStartCode()
{
	int count = outcnt;

	Bitstream_PutBits(1,0);
	while (outcnt != 8) Bitstream_PutBits(1, 1);

	return (count);
}


int Bitstream_GetLength()
{
	return bytecnt;
}


