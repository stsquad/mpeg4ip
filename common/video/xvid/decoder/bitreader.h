#ifndef _DECODER_BITREADER_H_
#define _DECODER_BITREADER_H_

#include "../portab.h"
#include "decoder.h"


// vop coding types 
// intra, prediction, backward, sprite, not_coded
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4

typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t pos;
	uint32_t * tail;
	uint32_t * start;
	uint32_t length;
} 
BITREADER;



// header stuff
int bs_headers(BITREADER * bs, DECODER * dec, uint32_t * rounding, uint32_t * quant, uint32_t * fcode, uint32_t * intra_dc_threshold);

static void __inline bs_init(BITREADER * const bs, void * const bitstream, uint32_t length)
{
	uint32_t tmp;

	bs->start = (uint32_t*)bitstream;

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

	bs->pos = 0;
	bs->tail = ((uint32_t *)bitstream + 2);

	bs->length = length;
}


// return the ammount of bytes eaten

static uint32_t __inline bs_length(BITREADER * const bs)
{
	return 4*(bs->tail - bs->start) - 4 - ((32 - bs->pos) / 8);
}


static uint32_t __inline bs_show(BITREADER * const bs, const uint32_t bits)
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



static __inline void bs_skip(BITREADER * const bs, const uint32_t bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) 
	{
		uint32_t tmp;

		bs->bufa = bs->bufb;
		tmp = *(uint32_t *)bs->tail;
#ifndef ARCH_IS_BIG_ENDIAN
		BSWAP(tmp);
#endif
		bs->bufb = tmp;
		bs->tail++;
		bs->pos -= 32;
	}
}



static __inline void bs_bytealign(BITREADER * const bs)
{
	uint32_t remainder = bs->pos % 8;
	if (remainder)
	{
		bs_skip(bs, 8 - remainder);
	}
}



static uint32_t __inline bs_get(BITREADER * const bs, const uint32_t n)
{
	uint32_t ret = bs_show(bs, n);
	bs_skip(bs, n);
	return ret;
}


static uint32_t __inline bs_get1(BITREADER * const bs)
{
	return bs_get(bs, 1);
}


#endif /* _DECODER_BITREADER_H_ */
