/* 
 *
 *	Copyright (C) Aaron Holtzman - May 1999
 *
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

#include "mpeg3audio.h"
#include <string.h>

/* Bit allocation tables */

static short mpeg3_slowdec[]  = { 0x0f,  0x11,  0x13,  0x15  };
static short mpeg3_fastdec[]  = { 0x3f,  0x53,  0x67,  0x7b  };
static short mpeg3_slowgain[] = { 0x540, 0x4d8, 0x478, 0x410 };
static short mpeg3_dbpbtab[]  = { 0x000, 0x700, 0x900, 0xb00 };

static unsigned short mpeg3_floortab[] = { 0x2f0, 0x2b0, 0x270, 0x230, 0x1f0, 0x170, 0x0f0, 0xf800 };
static short mpeg3_fastgain[] = { 0x080, 0x100, 0x180, 0x200, 0x280, 0x300, 0x380, 0x400  };


static short mpeg3_bndtab[] = 
{  
	0,  1,  2,   3,   4,   5,   6,   7,   8,   9, 
	10, 11, 12,  13,  14,  15,  16,  17,  18,  19,
	20, 21, 22,  23,  24,  25,  26,  27,  28,  31,
	34, 37, 40,  43,  46,  49,  55,  61,  67,  73,
	79, 85, 97, 109, 121, 133, 157, 181, 205, 229 
};

static short mpeg3_bndsz[]  = 
{ 
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  3,  3,
	3,  3,  3,  3,  3,  6,  6,  6,  6,  6,
	6, 12, 12, 12, 12, 24, 24, 24, 24, 24 
};

static short mpeg3_masktab[] = 
{ 
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 28, 28, 29,
	29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34,
	34, 35, 35, 35, 35, 35, 35, 36, 36, 36, 36, 36, 36, 37, 37, 37,
	37, 37, 37, 38, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39, 39, 40,
	40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
	41, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 43, 43, 43,
	43, 43, 43, 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 44, 44,
	44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
	45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 46, 46, 46,
	46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
	46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 48, 48, 48,
	48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
	48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
	49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,  0,  0,  0 
};


static short mpeg3_latab[] = 
{ 
	0x0040, 0x003f, 0x003e, 0x003d, 0x003c, 0x003b, 0x003a, 0x0039,
	0x0038, 0x0037, 0x0036, 0x0035, 0x0034, 0x0034, 0x0033, 0x0032,
	0x0031, 0x0030, 0x002f, 0x002f, 0x002e, 0x002d, 0x002c, 0x002c,
	0x002b, 0x002a, 0x0029, 0x0029, 0x0028, 0x0027, 0x0026, 0x0026,
	0x0025, 0x0024, 0x0024, 0x0023, 0x0023, 0x0022, 0x0021, 0x0021,
	0x0020, 0x0020, 0x001f, 0x001e, 0x001e, 0x001d, 0x001d, 0x001c,
	0x001c, 0x001b, 0x001b, 0x001a, 0x001a, 0x0019, 0x0019, 0x0018,
	0x0018, 0x0017, 0x0017, 0x0016, 0x0016, 0x0015, 0x0015, 0x0015,
	0x0014, 0x0014, 0x0013, 0x0013, 0x0013, 0x0012, 0x0012, 0x0012,
	0x0011, 0x0011, 0x0011, 0x0010, 0x0010, 0x0010, 0x000f, 0x000f,
	0x000f, 0x000e, 0x000e, 0x000e, 0x000d, 0x000d, 0x000d, 0x000d,
	0x000c, 0x000c, 0x000c, 0x000c, 0x000b, 0x000b, 0x000b, 0x000b,
	0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x0009, 0x0009, 0x0009,
	0x0009, 0x0009, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
	0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0006, 0x0006,
	0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0005, 0x0005,
	0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0004, 0x0004,
	0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
	0x0004, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0002,
	0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
	0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
	0x0002, 0x0002, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000
};

static short mpeg3_hth[][50] = 
{
	{ 
		0x04d0, 0x04d0, 0x0440, 0x0400, 0x03e0, 0x03c0, 0x03b0, 0x03b0,  
		0x03a0, 0x03a0, 0x03a0, 0x03a0, 0x03a0, 0x0390, 0x0390, 0x0390,  
		0x0380, 0x0380, 0x0370, 0x0370, 0x0360, 0x0360, 0x0350, 0x0350,  
		0x0340, 0x0340, 0x0330, 0x0320, 0x0310, 0x0300, 0x02f0, 0x02f0,
		0x02f0, 0x02f0, 0x0300, 0x0310, 0x0340, 0x0390, 0x03e0, 0x0420,
		0x0460, 0x0490, 0x04a0, 0x0460, 0x0440, 0x0440, 0x0520, 0x0800,
		0x0840, 0x0840 
	},

	{ 
		0x04f0, 0x04f0, 0x0460, 0x0410, 0x03e0, 0x03d0, 0x03c0, 0x03b0, 
		0x03b0, 0x03a0, 0x03a0, 0x03a0, 0x03a0, 0x03a0, 0x0390, 0x0390, 
		0x0390, 0x0380, 0x0380, 0x0380, 0x0370, 0x0370, 0x0360, 0x0360, 
		0x0350, 0x0350, 0x0340, 0x0340, 0x0320, 0x0310, 0x0300, 0x02f0, 
		0x02f0, 0x02f0, 0x02f0, 0x0300, 0x0320, 0x0350, 0x0390, 0x03e0, 
		0x0420, 0x0450, 0x04a0, 0x0490, 0x0460, 0x0440, 0x0480, 0x0630, 
		0x0840, 0x0840 
	},

	{ 
		0x0580, 0x0580, 0x04b0, 0x0450, 0x0420, 0x03f0, 0x03e0, 0x03d0, 
		0x03c0, 0x03b0, 0x03b0, 0x03b0, 0x03a0, 0x03a0, 0x03a0, 0x03a0, 
		0x03a0, 0x03a0, 0x03a0, 0x03a0, 0x0390, 0x0390, 0x0390, 0x0390, 
		0x0380, 0x0380, 0x0380, 0x0370, 0x0360, 0x0350, 0x0340, 0x0330, 
		0x0320, 0x0310, 0x0300, 0x02f0, 0x02f0, 0x02f0, 0x0300, 0x0310, 
		0x0330, 0x0350, 0x03c0, 0x0410, 0x0470, 0x04a0, 0x0460, 0x0440, 
		0x0450, 0x04e0 
	}
};


static short mpeg3_baptab[] = 
{ 
	0,  1,  1,  1,  1,  1,  2,  2,  3,  3,  3,  4,  4,  5,  5,  6,
	6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 
	10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14,
	14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15 
};


static int logadd(int a, int b)
{ 
	int c;
	int address;

	c = a - b; 
	address = mpeg3_min((abs(c) >> 1), 255); 
	
	if(c >= 0) 
		return(a + mpeg3_latab[address]); 
	else 
		return(b + mpeg3_latab[address]); 
}


int mpeg3audio_ac3_calc_lowcomp(int a, int b0, int b1, int bin)
{ 
	if(bin < 7) 
	{ 
		if((b0 + 256) == b1)
			a = 384; 
	 	else 
		if(b0 > b1) 
			a = mpeg3_max(0, a - 64); 
	} 
	else if(bin < 20) 
	{ 
		if((b0 + 256) == b1) 
			a = 320; 
		else if(b0 > b1) 
			a = mpeg3_max(0, a - 64) ; 
	}
	else  
		a = mpeg3_max(0, a - 128); 
	
	return(a);
}

void mpeg3audio_ac3_ba_compute_psd(int start, 
		int end, 
		short exps[], 
		short psd[], 
		short bndpsd[])
{
	int bin,i,j,k;
	int lastbin = 0;
	
/* Map the exponents into dBs */
	for (bin = start; bin < end; bin++) 
	{ 
		psd[bin] = (3072 - (exps[bin] << 7)); 
	}

/* Integrate the psd function over each bit allocation band */
	j = start; 
	k = mpeg3_masktab[start]; 
	
	do 
	{ 
		lastbin = mpeg3_min(mpeg3_bndtab[k] + mpeg3_bndsz[k], end); 
		bndpsd[k] = psd[j]; 
		j++; 

		for(i = j; i < lastbin; i++) 
		{ 
			bndpsd[k] = logadd(bndpsd[k], psd[j]);
			j++; 
		}

		k++; 
	}while(end > lastbin);
}

void mpeg3audio_ac3_ba_compute_excitation(mpeg3audio_t *audio, 
		int start, 
		int end,
		int fgain,
		int fastleak, 
		int slowleak, 
		int is_lfe, 
		short bndpsd[],
		short excite[])
{
	int bin;
	int bndstrt;
	int bndend;
	int lowcomp = 0;
	int begin = 0;

/* Compute excitation function */
	bndstrt = mpeg3_masktab[start]; 
	bndend = mpeg3_masktab[end - 1] + 1; 
	
	if(bndstrt == 0) /* For fbw and lfe channels */ 
	{ 
		lowcomp = mpeg3audio_ac3_calc_lowcomp(lowcomp, bndpsd[0], bndpsd[1], 0); 
		excite[0] = bndpsd[0] - fgain - lowcomp; 
		lowcomp = mpeg3audio_ac3_calc_lowcomp(lowcomp, bndpsd[1], bndpsd[2], 1);
		excite[1] = bndpsd[1] - fgain - lowcomp; 
		begin = 7 ; 
		
/* Note: Do not call mpeg3audio_ac3_calc_lowcomp() for the last band of the lfe channel, (bin = 6) */ 
		for (bin = 2; bin < 7; bin++) 
		{ 
			if(!(is_lfe && (bin == 6)))
				lowcomp = mpeg3audio_ac3_calc_lowcomp(lowcomp, bndpsd[bin], bndpsd[bin+1], bin); 
			fastleak = bndpsd[bin] - fgain; 
			slowleak = bndpsd[bin] - audio->ac3_bit_allocation.sgain; 
			excite[bin] = fastleak - lowcomp; 
			
			if(!(is_lfe && (bin == 6)))
			{
				if(bndpsd[bin] <= bndpsd[bin+1]) 
				{
					begin = bin + 1 ; 
					break; 
				} 
			}
		} 
		
		for (bin = begin; bin < mpeg3_min(bndend, 22); bin++) 
		{ 
			if (!(is_lfe && (bin == 6)))
				lowcomp = mpeg3audio_ac3_calc_lowcomp(lowcomp, bndpsd[bin], bndpsd[bin+1], bin); 
			fastleak -= audio->ac3_bit_allocation.fdecay; 
			fastleak = mpeg3_max(fastleak, bndpsd[bin] - fgain); 
			slowleak -= audio->ac3_bit_allocation.sdecay; 
			slowleak = mpeg3_max(slowleak, bndpsd[bin] - audio->ac3_bit_allocation.sgain); 
			excite[bin] = mpeg3_max(fastleak - lowcomp, slowleak); 
		} 
		begin = 22; 
	} 
	else /* For coupling channel */ 
	{ 
		begin = bndstrt; 
	} 

	for (bin = begin; bin < bndend; bin++) 
	{ 
		fastleak -= audio->ac3_bit_allocation.fdecay; 
		fastleak = mpeg3_max(fastleak, bndpsd[bin] - fgain); 
		slowleak -= audio->ac3_bit_allocation.sdecay; 
		slowleak = mpeg3_max(slowleak, bndpsd[bin] - audio->ac3_bit_allocation.sgain); 
		excite[bin] = mpeg3_max(fastleak, slowleak) ; 
	} 
}

void mpeg3audio_ac3_ba_compute_mask(mpeg3audio_t *audio, 
		int start, 
		int end, 
		int fscod,
		int deltbae, 
		int deltnseg, 
		short deltoffst[], 
		short deltba[],
		short deltlen[], 
		short excite[], 
		short mask[])
{
	int bin, k;
	int bndstrt;
	int bndend;
	int delta;

	bndstrt = mpeg3_masktab[start]; 
	bndend = mpeg3_masktab[end - 1] + 1; 

/* Compute the masking curve */

	for (bin = bndstrt; bin < bndend; bin++) 
	{ 
		if (audio->ac3_bit_allocation.bndpsd[bin] < audio->ac3_bit_allocation.dbknee) 
		{ 
			excite[bin] += ((audio->ac3_bit_allocation.dbknee - audio->ac3_bit_allocation.bndpsd[bin]) >> 2); 
		} 
		mask[bin] = mpeg3_max(excite[bin], mpeg3_hth[fscod][bin]);
	}
	
/* Perform delta bit modulation if necessary */
	if ((deltbae == DELTA_BIT_REUSE) || (deltbae == DELTA_BIT_NEW)) 
	{ 
		int band = 0; 
		int seg = 0; 
		
		for (seg = 0; seg < deltnseg + 1; seg++) 
		{ 
			band += deltoffst[seg]; 
			if (deltba[seg] >= 4) 
			{ 
				delta = (deltba[seg] - 3) << 7;
			} 
			else 
			{ 
				delta = (deltba[seg] - 4) << 7;
			} 
			
			for (k = 0; k < deltlen[seg]; k++) 
			{ 
				mask[band] += delta; 
				band++; 
			} 
		} 
	}
}

void mpeg3audio_ac3_ba_compute_bap(mpeg3audio_t *audio, 
		int start, 
		int end, 
		int snroffset,
		short psd[], 
		short mask[], 
		short bap[])
{
	int i, j, k;
	short lastbin = 0;
	short address = 0;

/* Compute the bit allocation pointer for each bin */
	i = start; 
	j = mpeg3_masktab[start]; 

	do
	{
		lastbin = mpeg3_min(mpeg3_bndtab[j] + mpeg3_bndsz[j], end); 
		mask[j] -= snroffset; 
		mask[j] -= audio->ac3_bit_allocation.floor; 

		if(mask[j] < 0) 
			mask[j] = 0; 

		mask[j] &= 0x1fe0;
		mask[j] += audio->ac3_bit_allocation.floor; 
		for(k = i; k < lastbin; k++)
		{
			address = (psd[i] - mask[j]) >> 5; 
			address = mpeg3_min(63, mpeg3_max(0, address)); 
			bap[i] = mpeg3_baptab[address]; 
			i++; 
		}
		j++; 
	}while (end > lastbin);
}

int mpeg3audio_ac3_bit_allocate(mpeg3audio_t *audio, 
		unsigned int fscod, 
		mpeg3_ac3bsi_t *bsi, 
		mpeg3_ac3audblk_t *audblk)
{
	int result = 0;
	int i;
	int fgain;
	int snroffset;
	int start;
	int end;
	int fastleak;
	int slowleak;

/*printf("mpeg3audio_ac3_bit_allocate %d %d %d %d %d\n", audblk->sdcycod, audblk->fdcycod, audblk->sgaincod, audblk->dbpbcod, audblk->floorcod); */
/* Only perform bit_allocation if the exponents have changed or we
 * have new sideband information */
	if(audblk->chexpstr[0]  == 0 && audblk->chexpstr[1] == 0 &&
			audblk->chexpstr[2]  == 0 && audblk->chexpstr[3] == 0 &&
			audblk->chexpstr[4]  == 0 && audblk->cplexpstr   == 0 &&
			audblk->lfeexpstr    == 0 && audblk->baie        == 0 &&
			audblk->snroffste    == 0 && audblk->deltbaie    == 0)
		return 0;

/* Do some setup before we do the bit alloc */
	audio->ac3_bit_allocation.sdecay = mpeg3_slowdec[audblk->sdcycod]; 
	audio->ac3_bit_allocation.fdecay = mpeg3_fastdec[audblk->fdcycod];
	audio->ac3_bit_allocation.sgain = mpeg3_slowgain[audblk->sgaincod]; 
	audio->ac3_bit_allocation.dbknee = mpeg3_dbpbtab[audblk->dbpbcod]; 
	audio->ac3_bit_allocation.floor = mpeg3_floortab[audblk->floorcod]; 

/* if all the SNR offset constants are zero then the whole block is zero */
	if(!audblk->csnroffst    && !audblk->fsnroffst[0] && 
		 !audblk->fsnroffst[1] && !audblk->fsnroffst[2] && 
		 !audblk->fsnroffst[3] && !audblk->fsnroffst[4] &&
		 !audblk->cplfsnroffst && !audblk->lfefsnroffst)
	{
		memset(audblk->fbw_bap, 0, sizeof(short) * 256 * 5);
		memset(audblk->cpl_bap, 0, sizeof(short) * 256);
		memset(audblk->lfe_bap, 0, sizeof(short) * 7);
		return 0;
	}

	for(i = 0; i < bsi->nfchans; i++)
	{
		start = 0;
		end = audblk->endmant[i]; 
		fgain = mpeg3_fastgain[audblk->fgaincod[i]]; 
		snroffset = (((audblk->csnroffst - 15) << 4) + audblk->fsnroffst[i]) << 2 ;
		fastleak = 0;
		slowleak = 0;

		mpeg3audio_ac3_ba_compute_psd(start, 
				end, 
				(short*)audblk->fbw_exp[i], 
				audio->ac3_bit_allocation.psd, 
				audio->ac3_bit_allocation.bndpsd);

		mpeg3audio_ac3_ba_compute_excitation(audio,
				start, 
				end , 
				fgain, 
				fastleak, 
				slowleak, 
				0, 
				audio->ac3_bit_allocation.bndpsd, 
				audio->ac3_bit_allocation.excite);

		mpeg3audio_ac3_ba_compute_mask(audio,
				start, 
				end, 
				fscod, 
				audblk->deltbae[i], 
				audblk->deltnseg[i], 
				(short*)audblk->deltoffst[i], 
				(short*)audblk->deltba[i], 
				(short*)audblk->deltlen[i], 
				audio->ac3_bit_allocation.excite, 
				audio->ac3_bit_allocation.mask);

		mpeg3audio_ac3_ba_compute_bap(audio,
				start, 
				end, 
				snroffset, 
				audio->ac3_bit_allocation.psd, 
				audio->ac3_bit_allocation.mask, 
				(short*)audblk->fbw_bap[i]);
	}

	if(audblk->cplinu)
	{
		start = audblk->cplstrtmant; 
		end = audblk->cplendmant; 
		fgain = mpeg3_fastgain[audblk->cplfgaincod];
		snroffset = (((audblk->csnroffst - 15) << 4) + audblk->cplfsnroffst) << 2 ;
		fastleak = (audblk->cplfleak << 8) + 768; 
		slowleak = (audblk->cplsleak << 8) + 768;

		mpeg3audio_ac3_ba_compute_psd(start, 
				end, 
				(short*)audblk->cpl_exp, 
				audio->ac3_bit_allocation.psd, 
				audio->ac3_bit_allocation.bndpsd);

		mpeg3audio_ac3_ba_compute_excitation(audio,
				start, 
				end , 
				fgain, 
				fastleak, 
				slowleak, 
				0, 
				audio->ac3_bit_allocation.bndpsd, 
				audio->ac3_bit_allocation.excite);

		mpeg3audio_ac3_ba_compute_mask(audio,
				start, 
				end, 
				fscod, 
				audblk->cpldeltbae, 
				audblk->cpldeltnseg, 
				(short*)audblk->cpldeltoffst, 
				(short*)audblk->cpldeltba, 
				(short*)audblk->cpldeltlen, 
				audio->ac3_bit_allocation.excite, 
				audio->ac3_bit_allocation.mask);

		mpeg3audio_ac3_ba_compute_bap(audio,
				start, 
				end, 
				snroffset, 
				audio->ac3_bit_allocation.psd, 
				audio->ac3_bit_allocation.mask, 
				(short*)audblk->cpl_bap);
	}

	if(bsi->lfeon)
	{
		start = 0;
		end = 7;
		fgain = mpeg3_fastgain[audblk->lfefgaincod];
		snroffset = (((audblk->csnroffst - 15) << 4) + audblk->lfefsnroffst) << 2 ;
		fastleak = 0;
		slowleak = 0;

		mpeg3audio_ac3_ba_compute_psd(start, 
				end, 
				(short*)audblk->lfe_exp, 
				audio->ac3_bit_allocation.psd, 
				audio->ac3_bit_allocation.bndpsd);

		mpeg3audio_ac3_ba_compute_excitation(audio,
				start, 
				end , 
				fgain, 
				fastleak, 
				slowleak, 
				1, 
				audio->ac3_bit_allocation.bndpsd, 
				audio->ac3_bit_allocation.excite);

/* Perform no delta bit allocation for lfe */
		mpeg3audio_ac3_ba_compute_mask(audio,
				start, 
				end, 
				fscod, 
				2, 
				0, 
				0, 
				0, 
				0, 
				audio->ac3_bit_allocation.excite, 
				audio->ac3_bit_allocation.mask);

		mpeg3audio_ac3_ba_compute_bap(audio,
				start, 
				end, 
				snroffset, 
				audio->ac3_bit_allocation.psd, 
				audio->ac3_bit_allocation.mask, 
				(short*)audblk->lfe_bap);
	}

	return result;
}
