/* 
 *  idct_248.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - May 2000
 *     Copyright (C) Erik Walthinsen - May 2000
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

/* Attempt at efficient (?) 2-4-8 IDCT as defined by the DV standard,
 * based on matrix factorizations presented in "Direct Conversions
 * Between DV Format DCT and Ordinary DCT".  Neri Merhav.  Technical
 * Report: HP Laboratories Israel HPL-98-140.  August, 1988. 
 *
 * Note: this is very much an interim solution.  It is based on the 
 * same foundation (AAN algorithm) as the 88 MMX idct from Intel.
 * 
 * This version uses 32 bit math.  For MMX we will use 16bit, but that
 * is fairly subtle.  We also need to think hard about the best way to
 * merge quant/weight/prescale without jeprodizing precision.
 *
 * That said, this is way faster the brute force float version.
 */


#include <dv_types.h>

#include <stdio.h>
#include <math.h>

#define IDCT_248_UNIT_TEST 0

dv_248_coeff_t dv_idct_248_prescale[64];

/*
  beta2 = cos(M_PI/4);
  beta0 = beta2 - 0.5;
  beta1 = -1 - beta0;
  beta3 = -cos(3 * M_PI / 8);
  beta4 = cos(M_PI / 8);
*/
  

static gint32 beta0;
static gint32 beta1;
static gint32 beta2;
static gint32 beta3;
static gint32 beta4;

static gdouble C(gint u) {
  gdouble result;
  if(u == 0) {
    result = 0.5 / sqrt(2.0);
  } else {
    result = 0.5;
  } // else
  return(result);
} // C

static gdouble tickC(gint u) 
{
  gdouble result;
  if(u == 0) {
    result = 1.0 / sqrt(2.0);
  } else {
    result = 0.5;
  } // else
  return(result);
} // tickC

#if ARCH_X86 && defined(__GNUC__)

static inline gint32 fixed_multiply(gint32 a, gint32 b) {
  register gint32 eax asm("ax");
  register gint32 edx asm("dx");

  eax = a;

  __asm__("imull %2"
	  :"=a" (eax), "=d" (edx)
	  :"g" (b),
	  "0" (eax));
  return(edx << 2);
} // fixed_multiply

#else

/* Will this actually compile?  I dunno */
static inline gint32 fixed_multiply(gint32 a, gint32 b) {
  gint64 product;

  product = (gint64)a * (gint64)b;
  product >>= (32 - 2);
  return(product);
} // fixed_multiply

#endif

/* Compute the prescale vector.
 * (verify against  matlab result for kron(inv(D2),D))
 */
void dv_dct_248_init() {
  extern double dv_weight_inverse_248_matrix[];
  gint k, l;
  gdouble d;
  gdouble diag[2][8];
  gdouble dbeta0, dbeta1, dbeta2, dbeta3, dbeta4;

  dbeta2 = cos(M_PI/4);
  dbeta0 = dbeta2 - 0.5;
  dbeta1 = -1 - dbeta0;
  dbeta3 = -cos(3 * M_PI / 8);
  dbeta4 = cos(M_PI / 8);

  beta0 = dbeta0 * pow(2,30);
  beta1 = dbeta1 * pow(2,30);
  beta2 = dbeta2 * pow(2,30);
  beta3 = dbeta3 * pow(2,30);
  beta4 = dbeta4 * pow(2,30);

  for(k=0; k<4;k++) {
    d = C(k) / (2.0 * cos( (M_PI * (double)k) / 8.0 ));
    diag[0][k] = diag[0][k+4] = d;
  } // for
  for(k=0; k<8;k++) {
    diag[1][k] = tickC(k) / (2.0 * cos( M_PI * k / 16.0 ));
  } // for
  for(k=0; k<8;k++) {
    for(l= 0; l<8 ; l++) {
      // Note the 2^16 shift is for fixed point precision.
      dv_idct_248_prescale[k*8+l] = 1.0/diag[0][k] * diag[1][l] * pow(2.0,14.0);
      dv_idct_248_prescale[k*8+l] *= dv_weight_inverse_248_matrix[k*8+l];
    } // for
  } // for
} // dv_dct_248_init

/* Total cost: 144 mults, 576 adds, 144 shifts. AAN is cited as having
   cost 144 mults, 464 mults. Doing some CSE below would probably get
   us there.  In principle, 2-4-8 is less complex than 88, since one
   of the 1D's consists of 2 order 4 DCTS instead of an order 8, and
   DCT's complexity is super-linear. 

   Even so, this is a big improvement over brute force, which requires
   4096 floating point multiplies. */

/* Let gcc make these into shifts if it wants... */   
#define DIV_TWO(A) ((A) / 2) 
#define DIV_FOUR(A) ((A) / 4)

void dv_idct_248(dv_248_coeff_t *x248, dv_coeff_t *out)
{
	dv_248_coeff_t tmp[64];
	dv_248_coeff_t *in, *lhs;
	dv_248_coeff_t u,v,w,z;
	dv_248_coeff_t in0, in1, in2, in3, in4, in5, in6, in7;
	gint i;

#if 0
	/* This is to identify visually where 248 blocks are... */
	for(i=0;i<64;i++) {
		out[i] = 235 - 128;
	}
	return;
#endif

	// Now, tmp = inv(h2) * inv(g2) * (prescale = inv(d2) * x248 * d)
	// 32 mults, 64 adds, 80 shifts, 16 negates 
	in = x248;
	lhs = tmp;

#if IDCT_248_UNIT_TEST
	printf("\nt0:\n");
	for(i=0;i<64; i++) {
	  printf("%d ", (in[i] + 0x2000) >> 14);
	  if((i+1) % 8 == 0) printf("\n");
	} // for
#endif // IDCT_248_UNIT_TEST
	for(i=0; i<8; i++) {
		u = in[0+i];
		v = in[2*8+i];
		w = in[1*8+i];
		z = in[3*8+i];
		lhs[0*8+i] = DIV_FOUR(u) + DIV_TWO(v);
		lhs[1*8+i] = DIV_FOUR(u) - DIV_TWO(v);
		lhs[2*8+i] = fixed_multiply(w,beta0) + fixed_multiply(z,beta1);
                lhs[3*8+i] = -(DIV_TWO(w+z));
		u = in[4*8+i];
		v = in[6*8+i];
		w = in[5*8+i];
		z = in[7*8+i];
		lhs[4*8+i] = DIV_FOUR(u) + DIV_TWO(v);
		lhs[5*8+i] = DIV_FOUR(u) - DIV_TWO(v);
		lhs[6*8+i] = fixed_multiply(w,beta0) + fixed_multiply(z,beta1);
                lhs[7*8+i] = -(DIV_TWO(w+z));
	} // for 
#if IDCT_248_UNIT_TEST
	printf("\nt1:\n");
	for(i=0;i<64; i++) {
	  printf("%d ", (lhs[i] + 0x2000) >> 14);
	  if((i+1) % 8 == 0) printf("\n");
	} // for
#endif // IDCT_248_UNIT_TEST
	in = tmp;
	lhs = x248;
	// Do lhs  = inv(f) * inv(L2) * in  (butterfly) 
        // 192 adds, 64 shifts
	for(i=0; i<8; i++) {
		u = in[8*0+i];
		v = in[8*3+i];
		w = in[8*4+i];
		z = in[8*7+i];
		lhs[8*0+i] = DIV_FOUR(u - v + w - z);
		lhs[8*1+i] = DIV_FOUR(u - v - w + z);
		lhs[8*6+i] = DIV_FOUR(u + v + w + z);
		lhs[8*7+i] = DIV_FOUR(u + v - w - z);
		u = in[8*1+i];
		v = in[8*2+i];
		w = in[8*5+i];
		z = in[8*6+i];
		lhs[i+8*2] = DIV_FOUR(u + v + w + z);
		lhs[i+8*3] = DIV_FOUR(u + v - w - z);
		lhs[i+8*4] = DIV_FOUR(u - v + w - z);
		lhs[i+8*5] = DIV_FOUR(u - v - w + z);
	} // for
#if IDCT_248_UNIT_TEST
	printf("\nt2:\n");
	for(i=0;i<64; i++) {
	  printf("%d ", (lhs[i] + 0x2000) >> 14);
	  if((i+1) % 8 == 0) printf("\n");
	} // for
#endif // IDCT_248_UNIT_TEST
	in = x248;
	lhs = tmp;
	// Do lhs = in * p * b1 * b2 * m 
	// 48 mults, 48 adds 
	for(i=0; i<8; i++) {
		lhs[i*8+0] = in[i*8+0];
		lhs[i*8+1] = in[i*8+4];
		u = in[i*8+2];
		v = in[i*8+6];
		lhs[i*8+2] = fixed_multiply(u - v,beta2);
		lhs[i*8+3] = u + v;
		u = in[i*8+1];
		v = in[i*8+3];
                w = in[i*8+5];
		z = in[i*8+7];
		lhs[i*8+4] = fixed_multiply(u - z,beta3) + fixed_multiply(v - w,beta4);
		lhs[i*8+5] = fixed_multiply(u - v - w + z,beta2);
		lhs[i*8+6] = fixed_multiply(u - z,beta4) + fixed_multiply(w - v,beta3);
		lhs[i*8+7] = u + v + w + z;
	} // for
#if IDCT_248_UNIT_TEST
	printf("\nt3:\n");
	for(i=0;i<64; i++) {
	  printf("%d ", (lhs[i] + 0x2000) >> 14);
	  if((i+1) % 8 == 0) printf("\n");
	} // for
#endif // IDCT_248_UNIT_TEST
	in = lhs;
	lhs = x248;
	// Do lhs = in * a1 * a2 * a3  (butterflys...) 
	// 272 adds (will gcc factor some of these out?)
	for(i=0; i<8; i++) {
		in0 = in[i*8+0];
		in1 = in[i*8+1];
		in2 = in[i*8+2];
		in3 = in[i*8+3];
		in4 = in[i*8+4];
		in5 = in[i*8+5];
		in6 = in[i*8+6];
		in7 = in[i*8+7];
		lhs[i*8+0] = in0 + in1 + in2 + in3 + in6 + in7;
		lhs[i*8+1] = in0 - in1 + in2 + in5 + in6;
		lhs[i*8+2] = in0 - in1 - in2 - in4 + in5;
		lhs[i*8+3] = in0 + in1 - in2 - in3 - in4;
		lhs[i*8+4] = in0 + in1 - in2 - in3 + in4;
		lhs[i*8+5] = in0 - in1 - in2 + in4 - in5;
		lhs[i*8+6] = in0 - in1 + in2 - in5 - in6;
		lhs[i*8+7] = in0 + in1 + in2 + in3 - in6 - in7;
	} // for
#if IDCT_248_UNIT_TEST
	printf("\nout:\n");
	for(i=0;i<64; i++) {
	  printf("%d ", (lhs[i] + 0x2000) >> 14);
	  if((i+1) % 8 == 0) printf("\n");
	} // for
#endif // IDCT_248_UNIT_TEST
	for(i=0; i<64; i++)
	  out [i] = (lhs[i] + 0x2000) >> 14;
} // dv_idct_248

#if IDCT_248_UNIT_TEST

// This is a test matrix generated from forward 2-4-8 dct using matlab
static gdouble dv_idct_248_test_x248[64] = {  
  -126.6250, -227.4838, -111.9136, 0.2316, 19.8750, 0.3386, -0.0514,
  -0.0334, 14.7008, 17.6504, -8.5810, -16.1455, -0.0676, 0.1048, 0.2134,
  -0.7044, 0.1250, 0.5249, -0.1633, -0.5139, 0.1250, -0.1308, -0.0676,
  -0.0758, 0.5404, -0.4475, -0.0366, 0.3909, 0.1633, 0.0426, 0.0810,
  0.2244, 25.8750, 15.7781, -15.5290, -22.2636, -0.6250, 4.3342, 0.0733,
  -0.4023, 1.6544, -3.9592, 4.2829, -0.1547, -3.8753, 0.0593, -0.2866,
  0.8777, -0.3750, -3.9695, -0.1633, 4.5689, 0.6250, 0.3619, -0.0676,
  -0.5475, 4.3208, 0.2903, 0.4634, -0.5293, 0.1169, 0.4600, 0.2171,
  -0.2753
}; // dv_idct24_test_x248

int main(int argc, char **argv) {
  gint i;
  dv_248_coeff_t x248[64];
  dv_init_dct248();
  /* prescale - 64 mults */
  for(i=0; i<64; i++) {
    x248[i]  = dv_idct_248_test_x248[i] * dv_idct_248_prescale[i];
  }
  dv_idct_248(x248);
  exit(0);
} // main 

#endif //IDCT_248_UNIT_TEST


	
