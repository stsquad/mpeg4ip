/* 
 *
 *	exponents.c Copyright (C) Aaron Holtzman - May 1999
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
 
 
#define MIN(x, y) ((x) > (y) ? (y) : (x))

#include "mpeg3audio.h"
#include <stdio.h>

/* Exponent defines */
#define UNPACK_FBW  1
#define UNPACK_CPL  2
#define UNPACK_LFE  4

static inline int mpeg3audio_ac3_exp_unpack_ch(unsigned int type,
		unsigned int expstr,
		unsigned int ngrps,
		unsigned int initial_exp, 
		unsigned short exps[], 
		unsigned short *dest)
{
	int i, j;
	int exp_acc;
	int exp_1, exp_2, exp_3;

	if(expstr == MPEG3_EXP_REUSE)
		return 0;

/* Handle the initial absolute exponent */
	exp_acc = initial_exp;
	j = 0;

/* In the case of a fbw channel then the initial absolute value is 
 * also an exponent */
	if(type != UNPACK_CPL)
		dest[j++] = exp_acc;

/* Loop through the groups and fill the dest array appropriately */
	for(i = 0; i < ngrps; i++)
	{
		if(exps[i] > 124)
		{
//			fprintf(stderr, "mpeg3audio_ac3_exp_unpack_ch: Invalid exponent %d\n", exps[i]);
			return 1;
		}

		exp_1 = exps[i] / 25;
		exp_2 = (exps[i] % 25) / 5;
		exp_3 = (exps[i] % 25) % 5;

		exp_acc += (exp_1 - 2);

		switch(expstr)
		{
			case MPEG3_EXP_D45:
				j = MIN(255, j);
				dest[j++] = exp_acc;
				j = MIN(255, j);
				dest[j++] = exp_acc;
			case MPEG3_EXP_D25:
				j = MIN(255, j);
				dest[j++] = exp_acc;
			case MPEG3_EXP_D15:
				j = MIN(255, j);
				dest[j++] = exp_acc;
		}

		exp_acc += (exp_2 - 2);

		switch(expstr)
		{
			case MPEG3_EXP_D45:
				j = MIN(255, j);
				dest[j++] = exp_acc;
				j = MIN(255, j);
				dest[j++] = exp_acc;
			case MPEG3_EXP_D25:
				j = MIN(255, j);
				dest[j++] = exp_acc;
			case MPEG3_EXP_D15:
				j = MIN(255, j);
				dest[j++] = exp_acc;
		}

		exp_acc += (exp_3 - 2);

		switch(expstr)
		{
			case MPEG3_EXP_D45:
				j = MIN(255, j);
				dest[j++] = exp_acc;
				j = MIN(255, j);
				dest[j++] = exp_acc;
			case MPEG3_EXP_D25:
				j = MIN(255, j);
				dest[j++] = exp_acc;
			case MPEG3_EXP_D15:
				j = MIN(255, j);
				dest[j++] = exp_acc;
		}
	}
	return 0;
}

int mpeg3audio_ac3_exponent_unpack(mpeg3audio_t *audio,
		mpeg3_ac3bsi_t *bsi,
		mpeg3_ac3audblk_t *audblk)
{
	int i, result = 0;

	for(i = 0; i < bsi->nfchans; i++)
		result |= mpeg3audio_ac3_exp_unpack_ch(UNPACK_FBW, 
			audblk->chexpstr[i], 
			audblk->nchgrps[i], 
			audblk->exps[i][0], 
			&audblk->exps[i][1], 
			audblk->fbw_exp[i]);

	if(audblk->cplinu && !result)
		result |= mpeg3audio_ac3_exp_unpack_ch(UNPACK_CPL, 
			audblk->cplexpstr, 
			audblk->ncplgrps, 
			audblk->cplabsexp << 1,	
			audblk->cplexps, 
			&audblk->cpl_exp[audblk->cplstrtmant]);

	if(bsi->lfeon && !result)
		result |= mpeg3audio_ac3_exp_unpack_ch(UNPACK_LFE, 
			audblk->lfeexpstr, 
			2, 
			audblk->lfeexps[0], 
			&audblk->lfeexps[1], 
			audblk->lfe_exp);
	return result;
}
