/* 
 *
 *	uncouple.c Copyright (C) Aaron Holtzman - May 1999
 *
 *  This file is part of libmpeg3
 *	
 *  libmpeg3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  libmpeg3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "../bitstream.h"
#include "mpeg3audio.h"

static unsigned char mpeg3_first_bit_lut[256] = 
{
	0, 8, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

/* Converts an unsigned exponent in the range of 0-24 and a 16 bit mantissa
 * to an IEEE single precision floating point value */
static inline void mpeg3audio_ac3_convert_to_float(unsigned short exp, 
	unsigned short mantissa, 
	u_int32_t *dest)
{
	int num;
	short exponent;
	int i;

/* If the mantissa is zero we can simply return zero */
	if(mantissa == 0)
	{
		*dest = 0;
		return;
	}

/* Exponent is offset by 127 in IEEE format minus the shift to
 * align the mantissa to 1.f (subtracted in the final result) */
	exponent = 127 - exp;

/* Take care of the one asymmetric negative number */
	if(mantissa == 0x8000)
		mantissa++;

/* Extract the sign bit, invert the mantissa if it's negative, and 
   shift out the sign bit */
	if(mantissa & 0x8000)
	{
		mantissa *= -1;
		num = 0x80000000 + (exponent << 23);
	}
	else
	{
		mantissa *= 1;
		num = exponent << 23;
	}

/* Find the index of the most significant one bit */
	i = mpeg3_first_bit_lut[mantissa >> 8];

	if(i == 0)
		i = mpeg3_first_bit_lut[mantissa & 0xff] + 8;

	*dest = num - (i << 23) + (mantissa << (7 + i));
	return;
}


int mpeg3audio_ac3_uncouple(mpeg3audio_t *audio, 
		mpeg3_ac3bsi_t *bsi,
		mpeg3_ac3audblk_t *audblk,
		mpeg3_stream_coeffs_t *coeffs)
{
	int i, j;

	for(i = 0; i < bsi->nfchans; i++)
	{
		for(j = 0; j < audblk->endmant[i]; j++)
			 mpeg3audio_ac3_convert_to_float(audblk->fbw_exp[i][j], 
			 		audblk->chmant[i][j],
					(u_int32_t*)&coeffs->fbw[i][j]);
	}

	if(audblk->cplinu)
	{
		for(i = 0; i < bsi->nfchans; i++)
		{
			if(audblk->chincpl[i])
			{
				mpeg3audio_ac3_uncouple_channel(audio, 
					coeffs,
					audblk,
					i);
			}
		}

	}

	if(bsi->lfeon)
	{
/* There are always 7 mantissas for lfe */
		for(j = 0; j < 7 ; j++)
			 mpeg3audio_ac3_convert_to_float(audblk->lfe_exp[j], 
			 		audblk->lfemant[j],
					 (u_int32_t*)&coeffs->lfe[j]);

	}
	return 0;
}
