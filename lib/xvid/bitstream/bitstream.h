 /******************************************************************************
  *                                                                            *
  *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
  *                                                                            *
  *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
  *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
  *  software module in hardware or software products are advised that its     *
  *  use may infringe existing patents or copyrights, and any such use         *
  *  would be at such party's own risk.  The original developer of this        *
  *  software module and his/her company, and subsequent editors and their     *
  *  companies, will have no liability for use of this software or             *
  *  modifications or derivatives thereof.                                     *
  *                                                                            *
  *  XviD is free software; you can redistribute it and/or modify it           *
  *  under the terms of the GNU General Public License as published by         *
  *  the Free Software Foundation; either version 2 of the License, or         *
  *  (at your option) any later version.                                       *
  *                                                                            *
  *  XviD is distributed in the hope that it will be useful, but               *
  *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
  *  GNU General Public License for more details.                              *
  *                                                                            *
  *  You should have received a copy of the GNU General Public License         *
  *  along with this program; if not, write to the Free Software               *
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  bitstream.h                                                               *
  *                                                                            *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *																			   *	
  *  Revision history:                                                         *
  *                                                                            *
  *  26.03.2002 interlacing support - modified putvol/vopheaders paramters
  *  04.03.2002 putbits speedup (Isibaar)                                      *
  *  03.03.2002 merged BITREADER and BITWRITER (Isibaar)                       *
  *	 16.12.2001	inital version                                           	   *
  *																			   *
  ******************************************************************************/

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include "../portab.h"
#include "../decoder.h"
#include "../encoder.h"

// comment any #defs we dont use

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
//#define VIDSESERR_ERROR_CODE	0x000001b4
#define VISOBJ_START_CODE		0x000001b5
//#define SLICE_START_CODE		0x000001b7
//#define EXT_START_CODE		0x000001b8


#define VISOBJ_TYPE_VIDEO				1
//#define VISOBJ_TYPE_STILLTEXTURE		2
//#define VISOBJ_TYPE_MESH				3
//#define VISOBJ_TYPE_FBA				4
//#define VISOBJ_TYPE_3DMESH			5


#define VIDOBJLAY_TYPE_SIMPLE			1
//#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE	2
#define VIDOBJLAY_TYPE_CORE				3
#define VIDOBJLAY_TYPE_MAIN				4


//#define VIDOBJLAY_AR_SQUARE			1
//#define VIDOBJLAY_AR_625TYPE_43		2
//#define VIDOBJLAY_AR_525TYPE_43		3
//#define VIDOBJLAY_AR_625TYPE_169		8
//#define VIDOBJLAY_AR_525TYPE_169		9
#define VIDOBJLAY_AR_EXTPAR				15


#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3

#define VO_START_CODE	0x8
#define VOL_START_CODE	0x12
#define VOP_START_CODE	0x1b6

#define READ_MARKER()	BitstreamSkip(bs, 1)
#define WRITE_MARKER()	BitstreamPutBit(bs, 1)

// vop coding types 
// intra, prediction, backward, sprite, not_coded
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4


// header stuff
int BitstreamReadHeaders(Bitstream * bs, DECODER * dec, uint32_t * rounding,
						 uint32_t * quant, uint32_t * fcode, uint32_t * intra_dc_threshold,
			 int findvol);

#ifdef MPEG4IP
void BitstreamWriteVoshHeader(Bitstream * const bs);
#endif

void BitstreamWriteVolHeader(Bitstream * const bs,
						const MBParam * pParam);


void BitstreamWriteVopHeader(Bitstream * const bs,
						const MBParam * pParam);


/* initialise bitstream structure */

static void __inline BitstreamInit(Bitstream * const bs,
								   void * const bitstream,
								   uint32_t length)
{
	uint32_t tmp;

	bs->start = bs->tail = (uint32_t*)bitstream;

	tmp = *(uint32_t *)bitstream;
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufa = tmp;

	tmp = *((uint32_t *)bitstream + 1);
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufb = tmp;

	bs->buf = 0;
	bs->pos = 0;
	bs->length = length;
}


/* reset bitstream state */

static void __inline BitstreamReset(Bitstream * const bs)
{
	uint32_t tmp;

	bs->tail = bs->start;

	tmp = *bs->start;
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufa = tmp;

	tmp = *(bs->start + 1);
#ifndef ARCH_IS_BIG_ENDIAN
	BSWAP(tmp);
#endif
	bs->bufb = tmp;

	bs->buf = 0;
    bs->pos = 0;
}


/* reads n bits from bitstream without changing the stream pos */

static uint32_t __inline BitstreamShowBits(Bitstream * const bs,
										   const uint32_t bits)
{
	int nbit = (bits + bs->pos) - 32;
	if (nbit > 0) 
	{
		return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) |
				(bs->bufb >> (32 - nbit));
	}
	else 
	{
		return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
	}
}


/* skip n bits forward in bitstream */

static __inline void BitstreamSkip(Bitstream * const bs, const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) 
	{
		uint32_t tmp;

		bs->bufa = bs->bufb;
		tmp = *((uint32_t *)bs->tail + 2);
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(tmp);
#endif
		bs->bufb = tmp;
		bs->tail++;
		bs->pos -= 32;
	}
}


/* move forward to the next byte boundary */

static __inline void BitstreamByteAlign(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;
	if (remainder)
	{
		BitstreamSkip(bs, 8 - remainder);
	}
}


/* bitstream length (unit bits) */

static uint32_t __inline BitstreamPos(const Bitstream * const bs)
{
    return 8 * ((uint32_t)bs->tail - (uint32_t)bs->start) + bs->pos;
}


/*	flush the bitstream & return length (unit bytes)
	NOTE: assumes no futher bitstream functions will be called.
 */

static uint32_t __inline BitstreamLength(Bitstream * const bs)
{
	uint32_t len = (uint32_t) bs->tail - (uint32_t) bs->start;

    if (bs->pos)
    {
		uint32_t b = bs->buf;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail = b;

		len += (bs->pos + 7) / 8;
    }

	return len;
}


/* move bitstream position forward by n bits and write out buffer if needed */

static void __inline BitstreamForward(Bitstream * const bs, const uint32_t bits)
{
    bs->pos += bits;

    if (bs->pos >= 32)
    {
		uint32_t b = bs->buf;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(b);
#endif
		*bs->tail++ = b;
		bs->buf = 0;
		bs->pos -= 32;
    }
}


#ifndef MPEG4IP
/* pad bitstream to the next byte boundary */

static void __inline BitstreamPad(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;

    if (remainder)
    {
		BitstreamForward(bs, 8 - remainder);
    }
}
#endif


/* read n bits from bitstream */

static uint32_t __inline BitstreamGetBits(Bitstream * const bs,
										  const uint32_t n)
{
	uint32_t ret = BitstreamShowBits(bs, n);
	BitstreamSkip(bs, n);
	return ret;
}


/* read single bit from bitstream */

static uint32_t __inline BitstreamGetBit(Bitstream * const bs)
{
	return BitstreamGetBits(bs, 1);
}


/* write single bit to bitstream */

static void __inline BitstreamPutBit(Bitstream * const bs, 
									const uint32_t bit)
{
    if (bit)
		bs->buf |= (0x80000000 >> bs->pos);

    BitstreamForward(bs, 1);
}


/* write n bits to bitstream */

static void __inline BitstreamPutBits(Bitstream * const bs, 
									const uint32_t value,
									const uint32_t size)
{
	uint32_t shift = 32 - bs->pos - size;

	if (shift <= 32) {
		bs->buf |= value << shift;
		BitstreamForward(bs, size);
	} else {
		uint32_t remainder;

		shift = size - (32 - bs->pos);
		bs->buf |= value >> shift;
		BitstreamForward(bs, size - shift);
		remainder = shift;

		shift = 32 - shift;
		
		bs->buf |= value << shift;
		BitstreamForward(bs, remainder);
	}
}

#ifdef MPEG4IP
static void __inline BitstreamPad(Bitstream * const bs)
{
	uint32_t remainder = bs->pos % 8;

    if (remainder)
    {
		// padding rule is a 0 bit 
		// followed by 1's until the byte boundary
		uint32_t pad = 8 - remainder;
		BitstreamPutBit(bs, 0);
		pad--;
		if (pad > 0) {
			BitstreamPutBits(bs, (1 << pad) - 1, pad);
		}
    }
}
#endif

#endif /* _BITSTREAM_H_ */
