/* 
 *  weighting.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */


#include <dv_types.h>

#include <math.h>

#include "weighting.h"

dv_coeff_t preSC[64] ALIGN32 = {
	16384,22725,21407,19266, 16384,12873,8867,4520,
	22725,31521,29692,26722, 22725,17855,12299,6270,
	21407,29692,27969,25172, 21407,16819,11585,5906,
	19266,26722,25172,22654, 19266,15137,10426,5315,

	16384,22725,21407,19266, 16384,12873,8867,4520,
	12873,17855,16819,15137, 25746,20228,13933,7103,
	17734,24598,23170,20853, 17734,13933,9597,4892,
	18081,25080,23624,21261, 18081,14206,9785,4988
};

dv_coeff_t postSC88[64] ALIGN32;
dv_coeff_t postSC248[64] ALIGN32;

static double W[8];

#if !ARCH_X86
static dv_coeff_t dv_weight_inverse_88_matrix[64];
#endif

#if BRUTE_FORCE_DCT_88
static double dv_weight_88_matrix[64];
#endif
#if BRUTE_FORCE_DCT_248
static double dv_weight_248_matrix[64];
#endif

double dv_weight_inverse_248_matrix[64];


static inline double CS(int m) {
  return cos(((double)m) * M_PI / 16.0);
}

static void weight_88_inverse_float(double *block);
static void weight_88_float(double *block);
static void weight_248_float(double *block);

static inline short int_val(double f)
{
	return (short) floor(f + 0.5);
}

static void postscale88_init(double* post_sc)
{
	int i,j;
	double ci,cj;

	for( i = 0; i < 8; i++ ) {
		ci = i==0 ? 1/(8.*sqrt(2.)) : 1.0/16.0;
		/* di = i==0 ? 1.5/(sqrt(2.)) : 0.5;
		   ps3[i] = 2.0*2.0*ci/cos(i*M_PI/16); 
		   israelh. this is table1 from AAN paper. 
		   Note the trick if 8 or 16 deivision
		*/
		for( j = 0; j < 8; j++) {
			cj = j==0 ? 1/(8*sqrt(2.)) : 1.0/16.0;
			post_sc[i * 8 + j] = 4.0*4.0 * ci * cj / 
				(cos(i*M_PI/16)*cos(j*M_PI/16));
			/* israelh. patch the first 4.0? */
		}
	}
	post_sc[63] = 1.0;    
}

static void postscale248_init(double* post_sc)
{
	int i,j;
	double ci,cj;

	for( i = 0; i < 4; i++ ) {
		ci = i==0 ? 1/(4.*sqrt(2.)) : 1.0/8.0;
		/* di = i==0 ? 1.5/(sqrt(2.)) : 0.5;
		   ps3[i] = 2.0*2.0*ci/cos(i*M_PI/16); 
		   israelh. this is table1 from AAN paper. 
		   Note the trick if 8 or 16 deivision
		*/
		for( j = 0; j < 8; j++) {
			cj = j==0 ? 1/(8*sqrt(2.)) : 1.0/16.0;
			post_sc[i * 8 + j] = 4.0*2.0 * ci * cj / 
				(cos(i*M_PI/8)*cos(j*M_PI/16));
			post_sc[i * 8 + 32 + j] = 4.0*2.0 * ci * cj / 
				(cos(i*M_PI/8)*cos(j*M_PI/16));
			/* israelh. patch the first 4.0? */
		}
	}
	post_sc[63-32] = 1.0;    
	post_sc[63] = 1.0;    
}

void weight_init(void) 
{
	double temp[64];
	double temp_postsc[64];
	int i, z, x;
#if ARCH_X86
	const double dv_weight_bias_factor = (double)(1UL << DV_WEIGHT_BIAS);
#endif

	W[0] = 1.0;
	W[1] = CS(4) / (4.0 * CS(7) * CS(2));
	W[2] = CS(4) / (2.0 * CS(6));
	W[3] = 1.0 / (2 * CS(5));
	W[4] = 7.0 / 8.0;
	W[5] = CS(4) / CS(3); 
	W[6] = CS(4) / CS(2); 
	W[7] = CS(4) / CS(1);
	
	for (i = 0; i < 64; i++) {
		temp[i] = 1.0;
	}
	weight_88_inverse_float(temp);

	for (i=0;i<64;i++) {
#if !ARCH_X86
		dv_weight_inverse_88_matrix[i] = (dv_coeff_t)rint(temp[i]);
#else
		/* If we're using MMX assembler, fold weights into the iDCT
		   prescale */
		preSC[i] *= temp[i] * (16.0 / dv_weight_bias_factor);
#endif
	}

	postscale88_init(temp_postsc);
	for (i = 0; i < 64; i++) {
		temp[i] = 1.0;
	}
	weight_88_float(temp);

	for (i=0;i<64;i++) {
#if BRUTE_FORCE_DCT_88
		dv_weight_88_matrix[i] = temp[i];
#else
		/* If we're not using brute force(tm), 
		   fold weights into the DCT
		   postscale */
		postSC88[i]= int_val(temp_postsc[i] * temp[i] * 32768.0 * 2.0);
#endif
	}
	postSC88[63] = temp[63] * 32768 * 2.0;    

	postscale248_init(temp_postsc);

	for (i = 0; i < 64; i++) {
		temp[i] = 1.0;
	}
	weight_248_float(temp);

	for (i=0;i<64;i++) {
#if BRUTE_FORCE_DCT_248
		dv_weight_248_matrix[i] = temp[i];
#else
		/* If we're not using brute force(tm), 
		   fold weights into the DCT
		   postscale */
		postSC248[i]= int_val(temp_postsc[i]* temp[i] * 32768.0 * 2.0);
#endif
	}

	for (z=0;z<4;z++) {
		for (x=0;x<8;x++) {
			dv_weight_inverse_248_matrix[z*8+x] = 
				2.0 / (W[x] * W[2*z]);
			dv_weight_inverse_248_matrix[(z+4)*8+x] = 
				2.0 / (W[x] * W[2*z]);
			
		}
	}
	dv_weight_inverse_248_matrix[0] = 4.0;
}

void weight_88(dv_coeff_t *block) 
{
	/* These weights are now folded into the dct postscaler - so this
	   function doesn't do anything. */
#if BRUTE_FORCE_DCT_88
	int i;

	for (i=0;i<64;i++) {
		block[i] *= dv_weight_88_matrix[i];
	}
#endif
}

static void weight_88_float(double *block) 
{
	int x,y;
	double dc;

	dc = block[0];
	for (y=0;y<8;y++) {
		for (x=0;x<8;x++) {
			block[y*8+x] *= W[x] * W[y] / 2.0;
		}
	}
	block[0] = dc / 4.0;
}


void weight_248(dv_coeff_t *block) 
{
	/* These weights are now folded into the dct postscaler - so this
	   function doesn't do anything. */
#if BRUTE_FORCE_DCT_248
	int i;

	for (i=0;i<64;i++) {
		block[i] *= dv_weight_248_matrix[i];
	}
#endif
}

static void weight_248_float(double *block) 
{
	int x,z;
	double dc;

	dc = block[0];
	for (z=0;z<4;z++) {
		for (x=0;x<8;x++) {
			block[z*8+x] *= W[x] * W[2*z] / 2;
			block[(z+4)*8+x] *= W[x] * W[2*z] / 2;
		}
	}
	block[0] = dc / 4;
	block[32] = dc / 4;
}


static void weight_88_inverse_float(double *block) 
{
	int x,y;
	double dc;

	dc = block[0];
	for (y=0;y<8;y++) {
		for (x=0;x<8;x++) {
			block[y*8+x] /= (W[x] * W[y] / 2.0);
		}
	}
	block[0] = dc * 4.0;
}

void weight_88_inverse(dv_coeff_t *block) 
{
	/* When we're using MMX assembler, weights are applied in the 8x8
	   iDCT prescale */
#if !ARCH_X86
	int i;

	for (i=0;i<64;i++) {
		block[i] *= dv_weight_inverse_88_matrix[i];
	}
#endif
}

void weight_248_inverse(dv_coeff_t *block) 
{
	/* These weights are now folded into the idct prescalers - so this
	   function doesn't do anything. */
#if 0
	int x,z;
	dv_coeff_t dc;
	
	dc = block[0];
	for (z=0;z<4;z++) {
		for (x=0;x<8;x++) {
			block[z*8+x] /= (W[x] * W[2*z] / 2);
			block[(z+4)*8+x] /= (W[x] * W[2*z] / 2);
		}
	}
	block[0] = dc * 4;
#endif
}
