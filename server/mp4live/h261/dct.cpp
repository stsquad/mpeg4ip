/*
 * dct.cc --
 *
 *      DCT code, also contains MMX version
 *
 * Copyright (c) 1994-2002 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * A. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * B. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * C. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/mpeg4ip/mpeg4ip/server/mp4live/h261/dct.cpp,v 1.1 2003/04/09 00:44:42 wmaycisco Exp $";
#endif

#include <sys/types.h>
#include "bsd-endian.h"
#include "dct.h"
#include <stdio.h>

/* conditional declaration */
#if MMX_DCT_ENABLED
void domidct8x8llmW(short *inptr, short *quantptr, int *wsptr,
			   u_char *outptr, int stride);
#endif

/*
 * Macros for fix-point (integer) arithmetic.  FP_NBITS gives the number
 * of binary digits past the decimal point.  FP_MUL computes the product
 * of two fixed point numbers.  A fixed point number and an integer
 * can be directly multiplied to give a fixed point number.  FP_SCALE
 * converts a floating point number to fixed point (and is used only
 * at startup, not by the dct engine).  FP_NORM converts a fixed
 * point number to scalar by rounding to the closest integer.
 * FP_JNORM is similar except it folds the jpeg bias of 128 into the
 * rounding addition.
 */
#define FP_NBITS 15
#define FP_MUL(a, b)	((((a) >> 5) * ((b) >> 5)) >> (FP_NBITS - 10))
#define FP_SCALE(v)	(int)((double)(v) * double(1 << FP_NBITS) + 0.5)
#define FP_NORM(v)	(((v) + (1 << (FP_NBITS-1))) >> FP_NBITS)
#define FP_JNORM(v)	(((v) + (257 << (FP_NBITS-1))) >> FP_NBITS)

#define M(n) ((m0 >> (n)) & 1)

/*
 * This macro stolen from nv.
 */
/* Sick little macro which will limit x to [0..255] with logical ops */
#define LIMIT8(x, t) ((t = (x)), (t &= ~(t>>31)), (t | ~((t-256) >> 31)))
#define LIMIT(x, t) (LIMIT8((x), t) & 0xff)

#if 0
wmay - not needed
/* row order */
const u_char ROWZAG[] = {
	0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0
};
#endif
/* column order */
const u_char DCT_COLZAG[] = {
	0, 8, 1, 2, 9, 16, 24, 17,
	10, 3, 4, 11, 18, 25, 32, 40,
	33, 26, 19, 12, 5, 6, 13, 20,
	27, 34, 41, 48, 56, 49, 42, 35,
	28, 21, 14, 7, 15, 22, 29, 36,
	43, 50, 57, 58, 51, 44, 37, 30,
	23, 31, 38, 45, 52, 59, 60, 53,
	46, 39, 47, 54, 61, 62, 55, 63,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0
};

#define A1 FP_SCALE(0.7071068)
#define A2 FP_SCALE(0.5411961)
#define A3 A1
#define A4 FP_SCALE(1.3065630)
#define A5 FP_SCALE(0.3826834)

#define FA1 (0.707106781f)
#define FA2 (0.541196100f)
#define FA3 FA1
#define FA4 (1.306562965f)
#define FA5 (0.382683433f)

/*
 * these magic numbers are scaling factors for each coef of the 1-d
 * AA&N DCT.  The scale factor for coef 0 is 1 and coef 1<=n<=7 is
 * cos(n*PI/16)*sqrt(2).  There is also a normalization of sqrt(8).
 * Formally you divide by the scale factor but we multiply by the
 * inverse because it's faster.  So the numbers below are the inverse
 * of what was just described.
 */
#define B0 0.35355339059327376220
#define B1 0.25489778955207958447
#define B2 0.27059805007309849220
#define B3 0.30067244346752264027
#define B4 0.35355339059327376220
#define B5 0.44998811156820785231
#define B6 0.65328148243818826392
#define B7 1.28145772387075308943

/*
 * Output multipliers for AA&N DCT
 * (i.e., first stage multipliers for inverse DCT).
 */
static const double first_stage[8] = { B0, B1, B2, B3, B4, B5, B6, B7, };

#ifdef _MSC_VER
// 'initializing' : truncation from 'const double to float
#pragma warning(disable: 4305)
#endif

/*
 * The first_stage array crossed with itself.  This allows us
 * to embed the first stage multipliers of the row pass by
 * computing scaled versions of the columns.
 */
static const int cross_stage[64] = {
	FP_SCALE(B0 * B0),
	FP_SCALE(B0 * B1),
	FP_SCALE(B0 * B2),
	FP_SCALE(B0 * B3),
	FP_SCALE(B0 * B4),
	FP_SCALE(B0 * B5),
	FP_SCALE(B0 * B6),
	FP_SCALE(B0 * B7),

	FP_SCALE(B1 * B0),
	FP_SCALE(B1 * B1),
	FP_SCALE(B1 * B2),
	FP_SCALE(B1 * B3),
	FP_SCALE(B1 * B4),
	FP_SCALE(B1 * B5),
	FP_SCALE(B1 * B6),
	FP_SCALE(B1 * B7),

	FP_SCALE(B2 * B0),
	FP_SCALE(B2 * B1),
	FP_SCALE(B2 * B2),
	FP_SCALE(B2 * B3),
	FP_SCALE(B2 * B4),
	FP_SCALE(B2 * B5),
	FP_SCALE(B2 * B6),
	FP_SCALE(B2 * B7),

	FP_SCALE(B3 * B0),
	FP_SCALE(B3 * B1),
	FP_SCALE(B3 * B2),
	FP_SCALE(B3 * B3),
	FP_SCALE(B3 * B4),
	FP_SCALE(B3 * B5),
	FP_SCALE(B3 * B6),
	FP_SCALE(B3 * B7),

	FP_SCALE(B4 * B0),
	FP_SCALE(B4 * B1),
	FP_SCALE(B4 * B2),
	FP_SCALE(B4 * B3),
	FP_SCALE(B4 * B4),
	FP_SCALE(B4 * B5),
	FP_SCALE(B4 * B6),
	FP_SCALE(B4 * B7),

	FP_SCALE(B5 * B0),
	FP_SCALE(B5 * B1),
	FP_SCALE(B5 * B2),
	FP_SCALE(B5 * B3),
	FP_SCALE(B5 * B4),
	FP_SCALE(B5 * B5),
	FP_SCALE(B5 * B6),
	FP_SCALE(B5 * B7),

	FP_SCALE(B6 * B0),
	FP_SCALE(B6 * B1),
	FP_SCALE(B6 * B2),
	FP_SCALE(B6 * B3),
	FP_SCALE(B6 * B4),
	FP_SCALE(B6 * B5),
	FP_SCALE(B6 * B6),
	FP_SCALE(B6 * B7),

	FP_SCALE(B7 * B0),
	FP_SCALE(B7 * B1),
	FP_SCALE(B7 * B2),
	FP_SCALE(B7 * B3),
	FP_SCALE(B7 * B4),
	FP_SCALE(B7 * B5),
	FP_SCALE(B7 * B6),
	FP_SCALE(B7 * B7),
};
static const float f_cross_stage[64] = {
    B0 * B0,
    B0 * B1,
    B0 * B2,
    B0 * B3,
    B0 * B4,
    B0 * B5,
    B0 * B6,
    B0 * B7,

    B1 * B0,
    B1 * B1,
    B1 * B2,
    B1 * B3,
    B1 * B4,
    B1 * B5,
    B1 * B6,
    B1 * B7,

    B2 * B0,
    B2 * B1,
    B2 * B2,
    B2 * B3,
    B2 * B4,
    B2 * B5,
    B2 * B6,
    B2 * B7,

    B3 * B0,
    B3 * B1,
    B3 * B2,
    B3 * B3,
    B3 * B4,
    B3 * B5,
    B3 * B6,
    B3 * B7,

    B4 * B0,
    B4 * B1,
    B4 * B2,
    B4 * B3,
    B4 * B4,
    B4 * B5,
    B4 * B6,
    B4 * B7,

    B5 * B0,
    B5 * B1,
    B5 * B2,
    B5 * B3,
    B5 * B4,
    B5 * B5,
    B5 * B6,
    B5 * B7,

    B6 * B0,
    B6 * B1,
    B6 * B2,
    B6 * B3,
    B6 * B4,
    B6 * B5,
    B6 * B6,
    B6 * B7,

    B7 * B0,
    B7 * B1,
    B7 * B2,
    B7 * B3,
    B7 * B4,
    B7 * B5,
    B7 * B6,
    B7 * B7,
};

/*
 * Map a quantization table in natural, row-order,
 * into the qt input expected by rdct().
 */
#if 0
wmay - not needed
void
rdct_fold_q(const int* in, int* out)
{
#if !MMX_DCT_ENABLED
	for (int i = 0; i < 64; ++i) {
		/*
		 * Fold column and row passes of the dct.
		 * By scaling each column DCT independently,
		 * we pre-bias all the row DCT's so the
		 * first multiplier is already embedded
		 * in the temporary result.  Thanks to
		 * Martin Vetterli for explaining how
		 * to do this.
		 */
		double v = double(in[i]);
		v *= first_stage[i & 7];
		v *= first_stage[i >> 3];
		out[i] = FP_SCALE(v);
	}
#else
	/* the MMX version of rdct() expects the quantization
	 * table to be an array of short */
	for (int i = 0; i < 64; i++)
	  out[i] = in[i];
#endif
}
#endif

/*
 * Just like rdct_fold_q() but we divide by the quantizer.
 */
void
dct_fdct_fold_q(const int* in, float* out)
{
	for (int i = 0; i < 64; ++i) {
		double v = first_stage[i >> 3];
		v *= first_stage[i & 7];
		double q = double(in[i]);
		out[i] = (float) (v / q);
	}
}

#if 0
wmay - not needed
void dcsum(int dc, u_char* in, u_char* out, int stride)
{
	for (int k = 8; --k >= 0; ) {
		int t;
#ifdef INT_64
		/*FIXME assume little-endian */
		INT_64 i = *(INT_64*)in;
		INT_64 o = (INT_64)LIMIT(dc + (i >> 56), t) << 56;
		o |=  (INT_64)LIMIT(dc + (i >> 48 & 0xff), t) << 48;
		o |=  (INT_64)LIMIT(dc + (i >> 40 & 0xff), t) << 40;
		o |=  (INT_64)LIMIT(dc + (i >> 32 & 0xff), t) << 32;
		o |=  (INT_64)LIMIT(dc + (i >> 24 & 0xff), t) << 24;
		o |=  (INT_64)LIMIT(dc + (i >> 16 & 0xff), t) << 16;
		o |=  (INT_64)LIMIT(dc + (i >> 8 & 0xff), t) << 8;
		o |=  (INT_64)LIMIT(dc + (i & 0xff), t);
		*(INT_64*)out = o;
#else
		u_int o = 0;
		u_int i = *(u_int*)in;
		SPLICE(o, LIMIT(dc + EXTRACT(i, 24), t), 24);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 16), t), 16);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 8), t), 8);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 0), t), 0);
		*(u_int*)out = o;

		o = 0;
		i = *(u_int*)(in + 4);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 24),  t), 24);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 16), t), 16);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 8), t), 8);
		SPLICE(o, LIMIT(dc + EXTRACT(i, 0), t), 0);
		*(u_int*)(out + 4) = o;
#endif
		in += stride;
		out += stride;
	}
}

void dcsum2(int dc, u_char* in, u_char* out, int stride)
{
	for (int k = 8; --k >= 0; ) {
		int t;
		u_int o = 0;
		SPLICE(o, LIMIT(dc + in[0], t), 24);
		SPLICE(o, LIMIT(dc + in[1], t), 16);
		SPLICE(o, LIMIT(dc + in[2], t), 8);
		SPLICE(o, LIMIT(dc + in[3], t), 0);
		*(u_int*)out = o;

		o = 0;
		SPLICE(o, LIMIT(dc + in[4], t), 24);
		SPLICE(o, LIMIT(dc + in[5], t), 16);
		SPLICE(o, LIMIT(dc + in[6], t), 8);
		SPLICE(o, LIMIT(dc + in[7], t), 0);
		*(u_int*)(out + 4) = o;

		in += stride;
		out += stride;
	}
}
void dcfill(int DC, u_char* out, int stride)
{
	int t;
	u_int dc = DC;
	dc = LIMIT(dc, t);
	dc |= dc << 8;
	dc |= dc << 16;
#ifdef INT_64
	INT_64 xdc = dc;
	xdc |= xdc << 32;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
	out += stride;
	*(INT_64 *)out = xdc;
#else
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
	out += stride;
	*(u_int*)out = dc;
	*(u_int*)(out + 4) = dc;
#endif
}

#endif
/*
 * This routine mixes the DC & AC components of an 8x8 block of
 * pixels.  This routine is called for every block decoded so it
 * needs to be efficient.  It tries to do as many pixels in parallel
 * as will fit in a word.  The one complication is that it has to
 * deal with overflow (sum > 255) and underflow (sum < 0).  Underflow
 * & overflow are only possible if both terms have the same sign and
 * are indicated by the result having a different sign than the terms.
 * Note that underflow is more worrisome than overflow since it results
 * in bright white dots in a black field.
 * The DC term and sum are biased by 128 so a negative number has the
 * 2^7 bit = 0.  The AC term is not biased so a negative number has
 * the 2^7 bit = 1.  So underflow is indicated by (DC & AC & sum) != 0;
 */
#define MIX_LOGIC(sum, a, b, omask, uflo) \
{ \
	sum = a + b; \
	uflo = (a ^ b) & (a ^ sum) & omask; \
	if (uflo) { \
		if ((b = uflo & a) != 0) { \
			/* integer overflows */ \
			b |= b >> 1; \
			b |= b >> 2; \
			b |= b >> 4; \
			sum |= b; \
		} \
		if ((uflo &=~ b) != 0) { \
			/* integer underflow(s) */ \
			uflo |= uflo >> 1; \
			uflo |= uflo >> 2; \
			uflo |= uflo >> 4; \
			sum &= ~uflo; \
		} \
	} \
}
/*
 * Table of products of 8-bit scaled coefficients
 * and idct coefficients (there are only 33 unique
 * coefficients so we index via a compact ID).
 */
extern "C" u_char multab[];
/*
 * Array of coefficient ID's used to index multab.
 */
extern "C" u_int dct_basis[64][64 / sizeof(u_int)];

/*FIXME*/
#define LIMIT_512(s) ((s) > 511 ? 511 : (s) < -512 ? -512 : (s))

#ifdef INT_64
/*FIXME assume little-endian */
#define PSPLICE(v, n) pix |= (INT_64)(v) << ((n)*8)
#define DID4PIX
#define PSTORE ((INT_64*)p)[0] = pix
#define PIXDEF INT_64 pix = 0; int v, oflo = 0
#else
#define PSPLICE(v, n) SPLICE(pix, (v), (3 - ((n)&3)) * 8)
#define DID4PIX pix0 = pix; pix = 0
#define PSTORE ((u_int*)p)[0] = pix0; ((u_int*)p)[1] = pix
#define PIXDEF	u_int pix0, pix = 0; int v, oflo = 0
#endif
#define DOJPIX(val, n) v = FP_JNORM(val); oflo |= v; PSPLICE(v, n)
#define DOJPIXLIMIT(val, n) PSPLICE(LIMIT(FP_JNORM(val),t), n)
#define DOPIX(val, n) v = FP_NORM(val); oflo |= v; PSPLICE(v, n)
#define DOPIXLIMIT(val, n) PSPLICE(LIMIT(FP_NORM(val),t), n)
#define DOPIXIN(val, n) v = FP_NORM(val) + in[n]; oflo |= v; PSPLICE(v, n)
#define DOPIXINLIMIT(val, n) PSPLICE(LIMIT(FP_NORM(val) + in[n], t), n)

#if MMX_DCT_ENABLED
/* this code invokes mmx version of rdct.  This routine uses the algorithm
 * described by C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast
 * 1-D DCT Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 * Speech, and Signal Processing 1989.  The algorithm itself is slower but more
 * accurate than the AAN algorithm used in the standard version of this routine.
 */
char *output_buf[8];       // array of pointers to short
char output_space[8][8];   // space that assembly code can write to

void
#ifdef INT_64
rdct(register short *bp, INT_64 m0, u_char* p, int stride, const int* qt)
#else
rdct(register short *bp, u_int m0, u_int m1, u_char* p,
     int stride, const int* qt)
#endif
{

  int workspace[68];
  short bpt[8][8];
  short qtmx[64];
  for (int i = 0; i < 64; i++) {
    qtmx[i] = (short)qt[i];
    bpt[i & 7][i >> 3] = *(bp + i);
  }
  domidct8x8llmW(&bpt[0][0], qtmx, workspace, p, stride);

}

#else

/* The following code is the standard implementation of the DCT */

/*
 * A 2D Inverse DCT based on a column-row decomposition using
 * Arai, Agui, and Nakajmia's 8pt 1D Inverse DCT, from Fig. 4-8
 * Pennebaker & Mitchell (i.e., the pink JPEG book).  This figure
 * is the forward transform; reverse the flowgraph for the inverse
 * (you need to draw a picture).  The outputs are DFT coefficients
 * and need to be scaled according to Eq [4-3].
 *
 * The input coefficients are, counter to tradition, in column-order.
 * The bit mask indicates which coefficients are non-zero.  If the
 * corresponding bit is zero, then the coefficient is assumed zero
 * and the input coefficient is not referenced and need not be defined.
 * The 8-bit outputs are computed in row order and placed in the
 * output array pointed to by p, with each of the eight 8-byte lines
 * offset by "stride" bytes.
 *
 * qt is the inverse quantization table in column order.  These
 * coefficients are the product of the inverse quantization factor,
 * specified by the jpeg quantization table, and the first multiplier
 * in the inverse DCT flow graph.  This first multiplier is actually
 * biased by the row term so that the columns are pre-scaled to
 * eliminate the first row multiplication stage.
 *
 * The output is biased by 128, i.e., [-128,127] is mapped to [0,255],
 * which is relevant to jpeg.
 */
void
#ifdef INT_64
rdct(register short *bp, INT_64 m0, u_char* p, int stride, const int* qt)
#else
rdct(register short *bp, u_int m0, u_int m1, u_char* p,
     int stride, const int* qt)
#endif
{
	/*
	 * First pass is 1D transform over the columns.  Note that
	 * the coefficients are stored in column order, so even
	 * though we're accessing the columns, we access them
	 * in a row-like fashion.  We use a column-row decomposition
	 * instead of a row-column decomposition so we can splice
	 * pixels in an efficient, row-wise order in the second
	 * stage.
	 *
	 * The inverse quantization is folded together with the
	 * first stage multiplier.  This reduces the number of
	 * multiplies (per 8-pt transform) from 11 to 5 (ignoring
	 * the inverse quantization which must be done anyway).
	 *
	 * Because we compute appropriately scaled column DCTs,
	 * the row DCTs require only 5 multiplies per row total.
	 * So the total number of multiplies for the 8x8 DCT is 80
	 * (ignoring the checks for zero coefficients).
	 */
	int tmp[64];
	int* tp = tmp;
	int i;
	for (i = 8; --i >= 0; ) {
		if ((m0 & 0xfe) == 0) {
			/* AC terms all zero */
			int v = M(0) ? qt[0] * bp[0] : 0;
			tp[0] = v;
			tp[1] = v;
			tp[2] = v;
			tp[3] = v;
			tp[4] = v;
			tp[5] = v;
			tp[6] = v;
			tp[7] = v;
		} else {
			int t0, t1, t2, t3, t4, t5, t6, t7;

			if ((m0 & 0xaa) == 0)
				t4 = t5 = t6 = t7 = 0;
			else {
				t0 = M(5) ? qt[5] * bp[5] : 0;
				t2 = M(1) ? qt[1] * bp[1] : 0;
				t6 = M(7) ? qt[7] * bp[7] : 0;
				t7 = M(3) ? qt[3] * bp[3] : 0;

				t4 = t0 - t7;
				t1 = t2 + t6;
				t6 = t2 - t6;
				t7 += t0;

				t5 = t1 - t7;
				t7 += t1;

				t2 = FP_MUL(-A5, t4 + t6);
				t4 = FP_MUL(-A2, t4);
				t0 = t4 + t2;
				t1 = FP_MUL(A3, t5);
				t2 += FP_MUL(A4, t6);

				t4 = -t0;
				t5 = t1 - t0;
				t6 = t1 + t2;
				t7 += t2;
			}

#ifdef notdef
			if ((m0 & 0x55) == 0)
				t0 = t1 = t2 = t3 = 0;
			else {
#endif
				t0 = M(0) ? qt[0] * bp[0] : 0;
				t1 = M(4) ? qt[4] * bp[4] : 0;
				int x0 = t0 + t1;
				int x1 = t0 - t1;

				t0 = M(2) ? qt[2] * bp[2] : 0;
				t3 = M(6) ? qt[6] * bp[6] : 0;
				t2 = t0 - t3;
				t3 += t0;

				t2 = FP_MUL(A1, t2);
				t3 += t2;

				t0 = x0 + t3;
				t1 = x1 + t2;
				t2 = x1 - t2;
				t3 = x0 - t3;
#ifdef notdef
			}
#endif

			tp[0] = t0 + t7;
			tp[7] = t0 - t7;
			tp[1] = t1 + t6;
			tp[6] = t1 - t6;
			tp[2] = t2 + t5;
			tp[5] = t2 - t5;
			tp[3] = t3 + t4;
			tp[4] = t3 - t4;
		}
		tp += 8;
		bp += 8;
		qt += 8;

		m0 >>= 8;
#ifndef INT_64
		m0 |= m1 << 24;
		m1 >>= 8;
#endif
	}
	tp -= 64;
	/*
	 * Second pass is 1D transform over the rows.  Note that
	 * the coefficients are stored in column order, so even
	 * though we're accessing the rows, we access them
	 * in a column-like fashion.
	 *
	 * The pass above already computed the first multiplier
	 * in the flow graph.
	 */
	for (i = 8; --i >= 0; ) {
		int t0, t1, t2, t3, t4, t5, t6, t7;

		t0 = tp[8*5];
		t2 = tp[8*1];
		t6 = tp[8*7];
		t7 = tp[8*3];

#ifdef notdef
		if ((t0 | t2 | t6 | t7) == 0) {
			t4 = t5 = 0;
		} else
#endif
{
			t4 = t0 - t7;
			t1 = t2 + t6;
			t6 = t2 - t6;
			t7 += t0;

			t5 = t1 - t7;
			t7 += t1;

			t2 = FP_MUL(-A5, t4 + t6);
			t4 = FP_MUL(-A2, t4);
			t0 = t4 + t2;
			t1 = FP_MUL(A3, t5);
			t2 += FP_MUL(A4, t6);

			t4 = -t0;
			t5 = t1 - t0;
			t6 = t1 + t2;
			t7 += t2;
		}

		t0 = tp[8*0];
		t1 = tp[8*4];
		int x0 = t0 + t1;
		int x1 = t0 - t1;

		t0 = tp[8*2];
		t3 = tp[8*6];
		t2 = t0 - t3;
		t3 += t0;

		t2 = FP_MUL(A1, t2);
		t3 += t2;

		t0 = x0 + t3;
		t1 = x1 + t2;
		t2 = x1 - t2;
		t3 = x0 - t3;

		PIXDEF;
		DOJPIX(t0 + t7, 0);
		DOJPIX(t1 + t6, 1);
		DOJPIX(t2 + t5, 2);
		DOJPIX(t3 + t4, 3);
		DID4PIX;
		DOJPIX(t3 - t4, 4);
		DOJPIX(t2 - t5, 5);
		DOJPIX(t1 - t6, 6);
		DOJPIX(t0 - t7, 7);
		if (oflo & ~0xff) {
			int t;
			pix = 0;
			DOJPIXLIMIT(t0 + t7, 0);
			DOJPIXLIMIT(t1 + t6, 1);
			DOJPIXLIMIT(t2 + t5, 2);
			DOJPIXLIMIT(t3 + t4, 3);
			DID4PIX;
			DOJPIXLIMIT(t3 - t4, 4);
			DOJPIXLIMIT(t2 - t5, 5);
			DOJPIXLIMIT(t1 - t6, 6);
			DOJPIXLIMIT(t0 - t7, 7);
		}
		PSTORE;

		++tp;
		p += stride;
	}

}
#endif

/*
 * Inverse 2-D transform, similar to routine above (see comment above),
 * but more appropriate for H.261 instead of JPEG.  This routine does
 * not bias the output by 128, and has an additional argument which is
 * an input array which gets summed together with the inverse-transform.
 * For example, this allows motion-compensation to be folded in here,
 * saving an extra traversal of the block.  The input pointer can be
 * null, if motion-compensation is not needed.
 *
 * This routine does not take a quantization table, since the H.261
 * inverse quantizer is easily implemented via table lookup in the decoder.
 */
void
#ifdef INT_64
rdct(register short *bp, INT_64 m0, u_char* p, int stride, const u_char* in)
#else
rdct(register short *bp, u_int m0, u_int m1, u_char* p, int stride, const u_char *in)
#endif
{
	int tmp[64];
	int* tp = tmp;
	const int* qt = cross_stage;
	/*
	 * First pass is 1D transform over the rows of the input array.
	 */
	int i;
	for (i = 8; --i >= 0; ) {
		if ((m0 & 0xfe) == 0) {
			/*
			 * All ac terms are zero.
			 */
			int v = 0;
			if (M(0))
				v = qt[0] * bp[0];
			tp[0] = v;
			tp[1] = v;
			tp[2] = v;
			tp[3] = v;
			tp[4] = v;
			tp[5] = v;
			tp[6] = v;
			tp[7] = v;
		} else {
			int t4 = 0, t5 = 0, t6 = 0, t7 = 0;
			if (m0 & 0xaa) {
				/* odd part */
				if (M(1))
					t4 = qt[1] * bp[1];
				if (M(3))
					t5 = qt[3] * bp[3];
				if (M(5))
					t6 = qt[5] * bp[5];
				if (M(7))
					t7 = qt[7] * bp[7];

				int x0 = t6 - t5;
				t6 += t5;
				int x1 = t4 - t7;
				t7 += t4;

				t5 = FP_MUL(t7 - t6, A3);
				t7 += t6;

				t4 = FP_MUL(x1 + x0, A5);
				t6 = FP_MUL(x1, A4) - t4;
				t4 += FP_MUL(x0, A2);

				t7 += t6;
				t6 += t5;
				t5 += t4;
			}
			int t0 = 0, t1 = 0, t2 = 0, t3 = 0;
			if (m0 & 0x55) {
				/* even part */
				if (M(0))
					t0 = qt[0] * bp[0];
				if (M(2))
					t1 = qt[2] * bp[2];
				if (M(4))
					t2 = qt[4] * bp[4];
				if (M(6))
					t3 = qt[6] * bp[6];

				int x0 = FP_MUL(t1 - t3, A1);
				t3 += t1;
				t1 = t0 - t2;
				t0 += t2;
				t2 = t3 + x0;
				t3 = t0 - t2;
				t0 += t2;
				t2 = t1 - x0;
				t1 += x0;
			}
			tp[0] = t0 + t7;
			tp[1] = t1 + t6;
			tp[2] = t2 + t5;
			tp[3] = t3 + t4;
			tp[4] = t3 - t4;
			tp[5] = t2 - t5;
			tp[6] = t1 - t6;
			tp[7] = t0 - t7;
		}
		qt += 8;
		tp += 8;
		bp += 8;
		m0 >>= 8;
#ifndef INT_64
		m0 |= m1 << 24;
		m1 >>= 8;
#endif
	}
	tp -= 64;
	/*
	 * Second pass is 1D transform over the rows of the temp array.
	 */
	for (i = 8; --i >= 0; ) {
		int t4 = tp[8*1];
		int t5 = tp[8*3];
		int t6 = tp[8*5];
		int t7 = tp[8*7];
		if ((t4|t5|t6|t7) != 0) {
			/* odd part */
			int x0 = t6 - t5;
			t6 += t5;
			int x1 = t4 - t7;
			t7 += t4;

			t5 = FP_MUL(t7 - t6, A3);
			t7 += t6;

			t4 = FP_MUL(x1 + x0, A5);
			t6 = FP_MUL(x1, A4) - t4;
			t4 += FP_MUL(x0, A2);

			t7 += t6;
			t6 += t5;
			t5 += t4;
		}
		int t0 = tp[8*0];
		int t1 = tp[8*2];
		int t2 = tp[8*4];
		int t3 = tp[8*6];
		if ((t0|t1|t2|t3) != 0) {
			/* even part */
			int x0 = FP_MUL(t1 - t3, A1);
			t3 += t1;
			t1 = t0 - t2;
			t0 += t2;
			t2 = t3 + x0;
			t3 = t0 - t2;
			t0 += t2;
			t2 = t1 - x0;
			t1 += x0;
		}
		if (in != 0) {
			PIXDEF;
			DOPIXIN(t0 + t7, 0);
			DOPIXIN(t1 + t6, 1);
			DOPIXIN(t2 + t5, 2);
			DOPIXIN(t3 + t4, 3);
			DID4PIX;
			DOPIXIN(t3 - t4, 4);
			DOPIXIN(t2 - t5, 5);
			DOPIXIN(t1 - t6, 6);
			DOPIXIN(t0 - t7, 7);
			if (oflo & ~0xff) {
				int t;
				pix = 0;
				DOPIXINLIMIT(t0 + t7, 0);
				DOPIXINLIMIT(t1 + t6, 1);
				DOPIXINLIMIT(t2 + t5, 2);
				DOPIXINLIMIT(t3 + t4, 3);
				DID4PIX;
				DOPIXINLIMIT(t3 - t4, 4);
				DOPIXINLIMIT(t2 - t5, 5);
				DOPIXINLIMIT(t1 - t6, 6);
				DOPIXINLIMIT(t0 - t7, 7);
			}
			PSTORE;
			in += stride;
		} else {
			PIXDEF;
			DOPIX(t0 + t7, 0);
			DOPIX(t1 + t6, 1);
			DOPIX(t2 + t5, 2);
			DOPIX(t3 + t4, 3);
			DID4PIX;
			DOPIX(t3 - t4, 4);
			DOPIX(t2 - t5, 5);
			DOPIX(t1 - t6, 6);
			DOPIX(t0 - t7, 7);
			if (oflo & ~0xff) {
				int t;
				pix = 0;
				DOPIXLIMIT(t0 + t7, 0);
				DOPIXLIMIT(t1 + t6, 1);
				DOPIXLIMIT(t2 + t5, 2);
				DOPIXLIMIT(t3 + t4, 3);
				DID4PIX;
				DOPIXLIMIT(t3 - t4, 4);
				DOPIXLIMIT(t2 - t5, 5);
				DOPIXLIMIT(t1 - t6, 6);
				DOPIXLIMIT(t0 - t7, 7);
			}
			PSTORE;
		}
		tp += 1;
		p += stride;
	}
}

/*
 * This macro does the combined descale-and-quantize
 * multiply.  It truncates rather than rounds to give
 * the behavior required for the h.261 deadband quantizer.
 */
#define FWD_DandQ(v, iq) short((v) * qt[iq])

void dct_fdct(const u_char* in, int stride, short* out, const float* qt)
{
	float tmp[64];
	float* tp = tmp;

	int i;
	for (i = 8; --i >= 0; ) {
		float x0, x1, x2, x3, t0, t1, t2, t3, t4, t5, t6, t7;
		t0 = float(in[0] + in[7]);
		t7 = float(in[0] - in[7]);
		t1 = float(in[1] + in[6]);
		t6 = float(in[1] - in[6]);
		t2 = float(in[2] + in[5]);
		t5 = float(in[2] - in[5]);
		t3 = float(in[3] + in[4]);
		t4 = float(in[3] - in[4]);

		/* even part */
		x0 = t0 + t3;
		x2 = t1 + t2;
		tp[8*0] = x0 + x2;
		tp[8*4] = x0 - x2;

		x1 = t0 - t3;
		x3 = t1 - t2;
		t0 = (x1 + x3) * FA1;
		tp[8*2] = x1 + t0;
		tp[8*6] = x1 - t0;

		/* odd part */
		x0 = t4 + t5;
		x1 = t5 + t6;
		x2 = t6 + t7;

		t3 = x1 * FA1;
		t4 = t7 - t3;

		t0 = (x0 - x2) * FA5;
		t1 = x0 * FA2 + t0;
		tp[8*3] = t4 - t1;
		tp[8*5] = t4 + t1;

		t7 += t3;
		t2 = x2 * FA4 + t0;
		tp[8*1] = t7 + t2;
		tp[8*7] = t7 - t2;

		in += stride;
		tp += 1;
	}
	tp -= 8;

	for (i = 8; --i >= 0; ) {
		float x0, x1, x2, x3, t0, t1, t2, t3, t4, t5, t6, t7;
		t0 = tp[0] + tp[7];
		t7 = tp[0] - tp[7];
		t1 = tp[1] + tp[6];
		t6 = tp[1] - tp[6];
		t2 = tp[2] + tp[5];
		t5 = tp[2] - tp[5];
		t3 = tp[3] + tp[4];
		t4 = tp[3] - tp[4];

		/* even part */
		x0 = t0 + t3;
		x2 = t1 + t2;
		out[0] = FWD_DandQ(x0 + x2, 0);
		out[4] = FWD_DandQ(x0 - x2, 4);

		x1 = t0 - t3;
		x3 = t1 - t2;
		t0 = (x1 + x3) * FA1;
		out[2] = FWD_DandQ(x1 + t0, 2);
		out[6] = FWD_DandQ(x1 - t0, 6);

		/* odd part */
		x0 = t4 + t5;
		x1 = t5 + t6;
		x2 = t6 + t7;

		t3 = x1 * FA1;
		t4 = t7 - t3;

		t0 = (x0 - x2) * FA5;
		t1 = x0 * FA2 + t0;
		out[3] = FWD_DandQ(t4 - t1, 3);
		out[5] = FWD_DandQ(t4 + t1, 5);

		t7 += t3;
		t2 = x2 * FA4 + t0;
		out[1] = FWD_DandQ(t7 + t2, 1);
		out[7] = FWD_DandQ(t7 - t2, 7);

		out += 8;
		tp += 8;
		qt += 8;
	}
}

/*
 * decimate the *rows* of the two input 8x8 DCT matrices into
 * a single output matrix.  we decimate rows rather than
 * columns even though we want column decimation because
 * the DCTs are stored in column order.
 */
#if 0
void
dct_decimate(const short* in0, const short* in1, short* o)
{
	for (int k = 0; k < 8; ++k) {
		int x00 = in0[0];
		int x01 = in0[1];
		int x02 = in0[2];
		int x03 = in0[3];
		int x10 = in1[0];
		int x11 = in1[1];
		int x12 = in1[2];
		int x13 = in1[3];
#define X_N 4
#define X_5(v)  ((v) << (X_N - 1))
#define X_25(v)  ((v) << (X_N - 2))
#define X_125(v)  ((v) << (X_N - 3))
#define X_0625(v)  ((v) << (X_N - 4))
#define X_375(v) (X_25(v) + X_125(v))
#define X_625(v) (X_5(v) + X_125(v))
#define X_75(v) (X_5(v) + X_25(v))
#define X_6875(v) (X_5(v) + X_125(v) + X_0625(v))
#define X_1875(v) (X_125(v) + X_0625(v))
#define X_NORM(v) ((v) >> X_N)

		/*
		 * 0.50000000  0.09011998  0.00000000 0.10630376
		 * 	0.50000000  0.09011998  0.00000000  0.10630376
		 * 0.45306372  0.28832037  0.03732892 0.08667963
		 * 	-0.45306372  0.11942621  0.10630376 -0.06764951
		 * 0.00000000  0.49039264  0.17677670 0.00000000
		 * 	0.00000000 -0.49039264 -0.17677670  0.00000000
		 * -0.15909482  0.34009707  0.38408888 0.05735049
		 *	0.15909482  0.43576792 -0.09011998 -0.13845632
		 * 0.00000000 -0.03732892  0.46193977 0.25663998
		 * 	0.00000000 -0.03732892  0.46193977  0.25663998
		 * 0.10630376 -0.18235049  0.25663998 0.42361940
		 *	-0.10630376 -0.16332037 -0.45306372 -0.01587282
		 * 0.00000000  0.00000000 -0.07322330 0.41573481
		 * 	0.00000000  0.00000000  0.07322330 -0.41573481
		 * -0.09011998  0.13399123 -0.18766514 0.24442621
		 *	0.09011998  0.13845632  0.15909482  0.47539609
		 */

		o[0] = X_NORM(X_5(x00 + x10) + X_0625(x01 + x11) +
			      X_125(x03 + x13));
		o[1] = X_NORM(X_5(x00 - x10) + X_25(x01) + X_0625(x03) +
			      X_125(x11 + x12));
		o[2] = X_NORM(X_5(x01 - x11) + X_1875(x02 + x12));
		o[3] = X_NORM(X_1875(x10 - x00) + X_375(x01 + x02) +
			      X_5(x11) - X_125(x13));
		o[4] = X_NORM(X_5(x02 + x12) + X_25(x03 + x13));
		o[5] = X_NORM(X_125(x00 - x10) - X_1875(x01 + x11) +
			      X_25(x02) + X_5(x03 - x12));
		o[6] = X_NORM(X_625(x12 - x02) + X_375(x03 + x13));
		o[7] = X_NORM(X_125(x01 - x00 + x11 + x10 + x12) +
			      X_1875(x02) + X_25(x03) + X_5(x13));

		o += 8;
		in0 += 8;
		in1 += 8;
	}
}
#endif

#ifdef _MSC_VER
// 'initializing' : truncation from 'const double to float
#pragma warning(default: 4305)
#endif

/* What follows is the assembly language implementation of the mmx dct */

 /***************************************************************************
 *
 *      This program has been developed by Intel Corporation.
 *      You have Intel's permission to incorporate this code
 *      into your product, royalty free.  Intel has various
 *      intellectual property rights which it may assert under
 *      certain circumstances, such as if another manufacturer's
 *      processor mis-identifies itself as being "GenuineIntel"
 *      when the CPUID instruction is executed.
 *
 *      Intel specifically disclaims all warranties, express or
 *      implied, and all liability, including consequential and
 *      other indirect damages, for the use of this code,
 *      including liability for infringement of any proprietary
 *      rights, and including the warranties of merchantability
 *      and fitness for a particular purpose.  Intel does not
 *      assume any responsibility for any errors which may
 *      appear in this code nor any responsibility to update it.
 *
 *  *  Other brands and names are the property of their respective
 *     owners.
 *
 *  Copyright (c) 1997, Intel Corporation.  All rights reserved.
 ***************************************************************************/

/* This implementation is based on an algorithm described in
 *   C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
 *   Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 *   Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
 * The primary algorithm described there uses 11 multiplies and 29 adds.
 * We use their alternate method with 12 multiplies and 32 adds.
 * The advantage of this method is that no data path contains more than one
 * multiplication; this allows a very simple and accurate implementation in
 * scaled fixed-point arithmetic, with a minimal number of shifts.
 */

#if MMX_DCT_ENABLED

/* This version will compile with the GNU compiler */
#ifdef __GNUC__
void domidct8x8llmW(short *inptr, short *quantptr, int *wsptr,
		    u_char *outptr, int stride)
{
    	static	u_int64_t fix_029_n089n196 __asm__("fix_029_n089n196")	= 0x098ea46e098ea46eLL;
	static	u_int64_t fix_n196_n089 __asm__("fix_n196_n089")	= 0xc13be333c13be333LL;
	static	u_int64_t fix_205_n256n039 __asm__("fix_205_n256n039")	= 0x41b3a18141b3a181LL;
	static	u_int64_t fix_n039_n256 __asm__("fix_n039_n256")	= 0xf384adfdf384adfdLL;
	static	u_int64_t fix_307n256_n196 __asm__("fix_307n256_n196")	= 0x1051c13b1051c13bLL;
	static	u_int64_t fix_n256_n196 __asm__("fix_n256_n196")	= 0xadfdc13badfdc13bLL;
	static	u_int64_t fix_150_n089n039 __asm__("fix_150_n089n039")	= 0x300bd6b7300bd6b7LL;
	static	u_int64_t fix_n039_n089 __asm__("fix_n039_n089")	= 0xf384e333f384e333LL;
	static	u_int64_t fix_117_117 __asm__("fix_117_117")		= 0x25a125a125a125a1LL;
	static	u_int64_t fix_054_054p076 __asm__("fix_054_054p076")	= 0x115129cf115129cfLL;
	static	u_int64_t fix_054n184_054 __asm__("fix_054n184_054")	= 0xd6301151d6301151LL;

	static	u_int64_t fix_054n184 __asm__("fix_054n184")		= 0xd630d630d630d630LL;
	static	u_int64_t fix_054 __asm__("fix_054")			= 0x1151115111511151LL;
	static	u_int64_t fix_054p076 __asm__("fix_054p076")		= 0x29cf29cf29cf29cfLL;
	static	u_int64_t fix_n196p307n256 __asm__("fix_n196p307n256")	= 0xd18cd18cd18cd18cLL;
	static	u_int64_t fix_n089n039p150 __asm__("fix_n089n039p150")	= 0x06c206c206c206c2LL;
	static	u_int64_t fix_n256 __asm__("fix_n256")			= 0xadfdadfdadfdadfdLL;
	static	u_int64_t fix_n039 __asm__("fix_n039")			= 0xf384f384f384f384LL;
	static	u_int64_t fix_n256n039p205 __asm__("fix_n256n039p205")	= 0xe334e334e334e334LL;
	static	u_int64_t fix_n196 __asm__("fix_n196")			= 0xc13bc13bc13bc13bLL;
	static	u_int64_t fix_n089 __asm__("fix_n089")			= 0xe333e333e333e333LL;
	static	u_int64_t fixn089n196p029 __asm__("fixn089n196p029")	= 0xadfcadfcadfcadfcLL;

	static  u_int64_t const_0x2xx8 __asm__("const_0x2xx8")	= 0x0000010000000100LL;
	static  u_int64_t const_0x0808 __asm__("const_0x0808")	= 0x0808080808080808LL;

	__asm__ __volatile__(

        "#quantptr is in %%edi\n"
        "#inptr is in %%ebx\n"
        "#wsptr is in %%esi\n"
        "#outptr is in 20(%%ebp)\n"
        "#stride is in 24(%%ebp)\n"

        "addl            $0x07,%%esi              #align wsptr to qword\n"
        "andl            $0xfffffff8,%%esi #align wsptr to qword\n"

        "movl            %%esi,%%eax\n"



        "movq            8*4(%%ebx),%%mm0          #p1(1,0)\n"
        "pmullw          8*4(%%edi),%%mm0          #p1(1,1)\n"

        "movq            8*12(%%ebx),%%mm1         #p1(2,0)\n"
        "pmullw          8*12(%%edi),%%mm1         #p1(2,1)\n"

        "movq            8*0(%%ebx),%%mm6          #p1(5,0)\n"
        "pmullw          8*0(%%edi),%%mm6          #p1(5,1)\n"

        "movq            %%mm0,%%mm2                               #p1(3,0)\n"
        "movq            8*8(%%ebx),%%mm7          #p1(6,0)\n"

        "punpcklwd       %%mm1,%%mm0                               #p1(3,1)\n"
        "pmullw          8*8(%%edi),%%mm7          #p1(6,1)\n"

        "movq            %%mm0,%%mm4                               #p1(3,2)\n"
        "punpckhwd       %%mm1,%%mm2                               #p1(3,4)\n"

        "pmaddwd         fix_054n184_054,%%mm0   #p1(3,3)\n"
        "movq            %%mm2,%%mm5                               #p1(3,5)\n"

        "pmaddwd         fix_054n184_054,%%mm2   #p1(3,6)\n"
        "pxor            %%mm1,%%mm1       #p1(7,0)\n"

        "pmaddwd         fix_054_054p076,%%mm4           #p1(4,0)\n"
        "punpcklwd   %%mm6,%%mm1   #p1(7,1)\n"

        "pmaddwd         fix_054_054p076,%%mm5           #p1(4,1)\n"
        "psrad           $3,%%mm1         #p1(7,2)\n"

        "pxor            %%mm3,%%mm3       #p1(7,3)\n"

        "punpcklwd       %%mm7,%%mm3       #p1(7,4)\n"

        "psrad           $3,%%mm3         #p1(7,5)\n"

        "paddd           %%mm3,%%mm1       #p1(7,6)\n"

        "movq            %%mm1,%%mm3       #p1(7,7)\n"

        "paddd           %%mm4,%%mm1       #p1(7,8)\n"

        "psubd           %%mm4,%%mm3       #p1(7,9)\n"

        "movq            %%mm1,8*16(%%esi)         #p1(7,10)\n"
        "pxor            %%mm4,%%mm4       #p1(7,12)\n"

        "movq            %%mm3,8*22(%%esi)         #p1(7,11)\n"
        "punpckhwd       %%mm6,%%mm4       #p1(7,13)\n"

        "psrad           $3,%%mm4         #p1(7,14)\n"
        "pxor            %%mm1,%%mm1       #p1(7,15)\n"

        "punpckhwd       %%mm7,%%mm1       #p1(7,16)\n"

        "psrad           $3,%%mm1         #p1(7,17)\n"

        "paddd           %%mm1,%%mm4       #p1(7,18)\n"

        "movq            %%mm4,%%mm3       #p1(7,19)\n"
        "pxor            %%mm1,%%mm1       #p1(8,0)\n"

        "paddd           %%mm5,%%mm3       #p1(7,20)\n"
        "punpcklwd       %%mm6,%%mm1       #p1(8,1)\n"

        "psubd           %%mm5,%%mm4       #p1(7,21)\n"
        "psrad           $3,%%mm1         #p1(8,2)\n"

        "movq            %%mm3,8*17(%%esi)         #p1(7,22)\n"
        "pxor            %%mm5,%%mm5       #p1(8,3)\n"

        "movq            %%mm4,8*23(%%esi)         #p1(7,23)\n"
        "punpcklwd       %%mm7,%%mm5       #p1(8,4)\n"

        "psrad           $3,%%mm5         #p1(8,5)\n"
        "pxor            %%mm4,%%mm4       #p1(8,12)\n"

        "psubd           %%mm5,%%mm1       #p1(8,6)\n"
        "punpckhwd       %%mm6,%%mm4       #p1(8,13)\n"

        "movq            %%mm1,%%mm3       #p1(8,7)\n"
        "psrad           $3,%%mm4         #p1(8,14)\n"

        "paddd           %%mm0,%%mm1       #p1(8,8)\n"
        "pxor            %%mm5,%%mm5       #p1(8,15)\n"

        "psubd           %%mm0,%%mm3       #p1(8,9)\n"
        "movq            8*14(%%ebx),%%mm0         #p1(9,0)\n"

        "punpckhwd       %%mm7,%%mm5       #p1(8,16)\n"
        "pmullw          8*14(%%edi),%%mm0         #p1(9,1)\n"

        "movq            %%mm1,8*18(%%esi)         #p1(8,10)\n"
        "psrad           $3,%%mm5         #p1(8,17)\n"

        "movq            %%mm3,8*20(%%esi)         #p1(8,11)\n"
        "psubd           %%mm5,%%mm4       #p1(8,18)\n"

        "movq            %%mm4,%%mm3       #p1(8,19)\n"
        "movq            8*6(%%ebx),%%mm1          #p1(10,0)\n"

        "paddd           %%mm2,%%mm3       #p1(8,20)\n"
        "pmullw          8*6(%%edi),%%mm1          #p1(10,1)\n"

        "psubd           %%mm2,%%mm4       #p1(8,21)\n"
        "movq            %%mm0,%%mm5                       #p1(11,1)\n"

        "movq            %%mm4,8*21(%%esi)         #p1(8,23)\n"

        "movq            %%mm3,8*19(%%esi)         #p1(8,22)\n"
        "movq            %%mm0,%%mm4                       #p1(11,0)\n"

        "punpcklwd       %%mm1,%%mm4                       #p1(11,2)\n"
        "movq            8*10(%%ebx),%%mm2         #p1(12,0)\n"

        "punpckhwd       %%mm1,%%mm5                       #p1(11,4)\n"
        "pmullw          8*10(%%edi),%%mm2         #p1(12,1)\n"

        "movq            8*2(%%ebx),%%mm3          #p1(13,0)\n"

        "pmullw          8*2(%%edi),%%mm3          #p1(13,1)\n"
        "movq            %%mm2,%%mm6                       #p1(14,0)\n"

        "pmaddwd         fix_117_117,%%mm4       #p1(11,3)\n"
        "movq            %%mm2,%%mm7                       #p1(14,1)\n"

        "pmaddwd         fix_117_117,%%mm5       #p1(11,5)\n"
        "punpcklwd       %%mm3,%%mm6                       #p1(14,2)\n"

        "pmaddwd         fix_117_117,%%mm6       #p1(14,3)\n"
        "punpckhwd       %%mm3,%%mm7                       #p1(14,4)\n"

        "pmaddwd         fix_117_117,%%mm7       #p1(14,5)\n"
        "paddd           %%mm6,%%mm4                       #p1(15,0)\n"

        "paddd           %%mm7,%%mm5                       #p1(15,1)\n"
        "movq            %%mm4,8*24(%%esi)         #p1(15,2)\n"

        "movq            %%mm5,8*25(%%esi)         #p1(15,3)\n"
        "movq            %%mm0,%%mm6                               #p1(16,0)\n"

        "movq            %%mm3,%%mm7                               #p1(16,3)\n"
        "punpcklwd       %%mm2,%%mm6                               #p1(16,1)\n"

        "punpcklwd       %%mm3,%%mm7                               #p1(16,4)\n"
        "pmaddwd         fix_n039_n089,%%mm6             #p1(16,2)\n"

        "pmaddwd         fix_150_n089n039,%%mm7  #p1(16,5)\n"
        "movq            %%mm0,%%mm4                               #p1(16,12)\n"

        "paddd           8*24(%%esi),%%mm6                 #p1(16,6)\n"
        "punpckhwd       %%mm2,%%mm4                               #p1(16,13)\n"

        "paddd           %%mm7,%%mm6                               #p1(16,7)\n"
        "pmaddwd         fix_n039_n089,%%mm4             #p1(16,14)\n"

        "movq            %%mm6,%%mm7                               #p1(16,8)\n"
        "paddd           8*25(%%esi),%%mm4                 #p1(16,18)\n"

        "movq            %%mm3,%%mm5                               #p1(16,15)\n"
        "paddd           8*16(%%esi),%%mm6                 #p1(16,9)\n"

        "punpckhwd       %%mm3,%%mm5                               #p1(16,16)\n"
        "paddd           const_0x2xx8,%%mm6              #p1(16,10)\n"

        "psrad           $9,%%mm6                                 #p1(16,11)\n"
        "pmaddwd         fix_150_n089n039,%%mm5  #p1(16,17)\n"

        "paddd           %%mm5,%%mm4                               #p1(16,19)\n"

        "movq            %%mm4,%%mm5                               #p1(16,20)\n"

        "paddd           8*17(%%esi),%%mm4                 #p1(16,21)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(16,22)\n"

        "psrad           $9,%%mm4                                 #p1(16,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(16,24)\n"

        "movq            %%mm6,8*0(%%esi)                  #p1(16,25)\n"

        "movq            8*16(%%esi),%%mm4                 #p1(16,26)\n"

        "psubd           %%mm7,%%mm4                               #p1(16,27)\n"
        "movq            8*17(%%esi),%%mm6                 #p1(16,30)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(16,28)\n"
        "movq            %%mm1,%%mm7                               #p1(17,3)\n"

        "psrad           $9,%%mm4                                 #p1(16,29)\n"
        "psubd           %%mm5,%%mm6                               #p1(16,31)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(16,32)\n"
        "punpcklwd       %%mm1,%%mm7                               #p1(17,4)\n"

        "pmaddwd         fix_307n256_n196,%%mm7  #p1(17,5)\n"
        "psrad           $9,%%mm6                                 #p1(16,33)\n"

        "packssdw        %%mm6,%%mm4                               #p1(16,34)\n"
        "movq            %%mm4,8*14(%%esi)                 #p1(16,35)\n"

        "movq            %%mm0,%%mm6                               #p1(17,0)\n"
        "movq            %%mm0,%%mm4                               #p1(17,12)\n"

        "punpcklwd       %%mm2,%%mm6                               #p1(17,1)\n"
        "punpckhwd       %%mm2,%%mm4                               #p1(17,13)\n"

        "pmaddwd         fix_n256_n196,%%mm6             #p1(17,2)\n"
        "movq            %%mm1,%%mm5                               #p1(17,15)\n"

        "paddd           8*24(%%esi),%%mm6                 #p1(17,6)\n"
        "punpckhwd       %%mm1,%%mm5                               #p1(17,16)\n"

        "paddd           %%mm7,%%mm6                               #p1(17,7)\n"
        "pmaddwd         fix_n256_n196,%%mm4             #p1(17,14)\n"

        "movq            %%mm6,%%mm7                               #p1(17,8)\n"
        "pmaddwd         fix_307n256_n196,%%mm5  #p1(17,17)\n"

        "paddd           8*18(%%esi),%%mm6                 #p1(17,9)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(17,10)\n"

        "psrad           $9,%%mm6                                 #p1(17,11)\n"
        "paddd           8*25(%%esi),%%mm4                 #p1(17,18)\n"

        "paddd           %%mm5,%%mm4                               #p1(17,19)\n"

        "movq            %%mm4,%%mm5                               #p1(17,20)\n"

        "paddd           8*19(%%esi),%%mm4                 #p1(17,21)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(17,22)\n"

        "psrad           $9,%%mm4                                 #p1(17,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(17,24)\n"

        "movq            %%mm6,8*2(%%esi)                  #p1(17,25)\n"

        "movq            8*18(%%esi),%%mm4                 #p1(17,26)\n"

        "movq            8*19(%%esi),%%mm6                 #p1(17,30)\n"
        "psubd           %%mm7,%%mm4                               #p1(17,27)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(17,28)\n"
        "psubd           %%mm5,%%mm6                               #p1(17,31)\n"

        "psrad           $9,%%mm4                                 #p1(17,29)\n"
        "paddd           const_0x2xx8,%%mm6              #p1(17,32)\n"

        "psrad           $9,%%mm6                                 #p1(17,33)\n"
        "movq            %%mm2,%%mm7                               #p1(18,3)\n"

        "packssdw        %%mm6,%%mm4                               #p1(17,34)\n"
        "movq            %%mm4,8*12(%%esi)                 #p1(17,35)\n"

        "movq            %%mm1,%%mm6                               #p1(18,0)\n"
        "punpcklwd       %%mm2,%%mm7                               #p1(18,4)\n"

        "punpcklwd       %%mm3,%%mm6                               #p1(18,1)\n"
        "pmaddwd         fix_205_n256n039,%%mm7  #p1(18,5)\n"

        "pmaddwd         fix_n039_n256,%%mm6             #p1(18,2)\n"
        "movq            %%mm1,%%mm4                               #p1(18,12)\n"

        "paddd           8*24(%%esi),%%mm6                 #p1(18,6)\n"
        "punpckhwd       %%mm3,%%mm4                               #p1(18,13)\n"

        "paddd           %%mm7,%%mm6                               #p1(18,7)\n"
        "pmaddwd         fix_n039_n256,%%mm4             #p1(18,14)\n"

        "movq            %%mm6,%%mm7                               #p1(18,8)\n"
        "movq            %%mm2,%%mm5                               #p1(18,15)\n"

        "paddd           8*20(%%esi),%%mm6                 #p1(18,9)\n"
        "punpckhwd       %%mm2,%%mm5                               #p1(18,16)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(18,10)\n"

        "psrad           $9,%%mm6                                 #p1(18,11)\n"
        "pmaddwd         fix_205_n256n039,%%mm5  #p1(18,17)\n"

        "paddd           8*25(%%esi),%%mm4                 #p1(18,18)\n"

        "paddd           %%mm5,%%mm4                               #p1(18,19)\n"

        "movq            %%mm4,%%mm5                               #p1(18,20)\n"

        "paddd           8*21(%%esi),%%mm4                 #p1(18,21)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(18,22)\n"

        "psrad           $9,%%mm4                                 #p1(18,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(18,24)\n"

        "movq            %%mm6,8*4(%%esi)                  #p1(18,25)\n"

        "movq            8*20(%%esi),%%mm4                 #p1(18,26)\n"

        "psubd           %%mm7,%%mm4                               #p1(18,27)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(18,28)\n"
        "movq            %%mm0,%%mm7                               #p1(19,3)\n"

        "psrad           $9,%%mm4                                 #p1(18,29)\n"
        "movq            8*21(%%esi),%%mm6                 #p1(18,30)\n"

        "psubd           %%mm5,%%mm6                               #p1(18,31)\n"
        "punpcklwd       %%mm0,%%mm7                               #p1(19,4)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(18,32)\n"

        "psrad           $9,%%mm6                                 #p1(18,33)\n"
        "pmaddwd         fix_029_n089n196,%%mm7  #p1(19,5)\n"

        "packssdw        %%mm6,%%mm4                               #p1(18,34)\n"

        "movq            %%mm4,8*10(%%esi)                 #p1(18,35)\n"
        "movq            %%mm3,%%mm6                               #p1(19,0)\n"

        "punpcklwd       %%mm1,%%mm6                               #p1(19,1)\n"
        "movq            %%mm0,%%mm5                               #p1(19,15)\n"

        "pmaddwd         fix_n196_n089,%%mm6             #p1(19,2)\n"
        "punpckhwd       %%mm0,%%mm5                               #p1(19,16)\n"


        "paddd           8*24(%%esi),%%mm6                 #p1(19,6)\n"
        "movq            %%mm3,%%mm4                               #p1(19,12)\n"

        "paddd           %%mm7,%%mm6                               #p1(19,7)\n"
        "punpckhwd       %%mm1,%%mm4                               #p1(19,13)\n"


        "movq            %%mm6,%%mm7                               #p1(19,8)\n"
        "pmaddwd         fix_n196_n089,%%mm4     #p1(19,14)\n"

        "paddd           8*22(%%esi),%%mm6                 #p1(19,9)\n"

        "pmaddwd         fix_029_n089n196,%%mm5  #p1(19,17)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(19,10)\n"

        "psrad           $9,%%mm6                                 #p1(19,11)\n"
        "paddd           8*25(%%esi),%%mm4                 #p1(19,18)\n"

        "paddd           %%mm5,%%mm4                               #p1(19,19)\n"

        "movq            %%mm4,%%mm5                               #p1(19,20)\n"
        "paddd           8*23(%%esi),%%mm4                 #p1(19,21)\n"
        "paddd           const_0x2xx8,%%mm4              #p1(19,22)\n"
        "psrad           $9,%%mm4                                 #p1(19,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(19,24)\n"
        "movq            %%mm6,8*6(%%esi)                  #p1(19,25)\n"

        "movq            8*22(%%esi),%%mm4                 #p1(19,26)\n"

        "psubd           %%mm7,%%mm4                               #p1(19,27)\n"
        "movq            8*23(%%esi),%%mm6                 #p1(19,30)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(19,28)\n"
        "psubd           %%mm5,%%mm6                               #p1(19,31)\n"

        "psrad           $9,%%mm4                                 #p1(19,29)\n"
        "paddd           const_0x2xx8,%%mm6              #p1(19,32)\n"

        "psrad           $9,%%mm6                                 #p1(19,33)\n"

        "packssdw        %%mm6,%%mm4                               #p1(19,34)\n"
        "movq            %%mm4,8*8(%%esi)                  #p1(19,35)\n"



        "addl            $8,%%edi\n"
        "addl            $8,%%ebx\n"
        "addl            $8,%%esi\n"




        "movq            8*4(%%ebx),%%mm0          #p1(1,0)\n"
        "pmullw          8*4(%%edi),%%mm0          #p1(1,1)\n"

    movq                8*12(%%ebx),%%mm1         #p1(2,0)
        "pmullw          8*12(%%edi),%%mm1         #p1(2,1)\n"

        "movq            8*0(%%ebx),%%mm6          #p1(5,0)\n"
        "pmullw          8*0(%%edi),%%mm6          #p1(5,1)\n"

        "movq            %%mm0,%%mm2                               #p1(3,0)\n"
        "movq            8*8(%%ebx),%%mm7          #p1(6,0)\n"

        "punpcklwd       %%mm1,%%mm0                               #p1(3,1)\n"
        "pmullw          8*8(%%edi),%%mm7          #p1(6,1)\n"

        "movq            %%mm0,%%mm4                               #p1(3,2)\n"
        "punpckhwd       %%mm1,%%mm2                               #p1(3,4)\n"

        "pmaddwd         fix_054n184_054,%%mm0   #p1(3,3)\n"
        "movq            %%mm2,%%mm5                               #p1(3,5)\n"

        "pmaddwd         fix_054n184_054,%%mm2   #p1(3,6)\n"
        "pxor            %%mm1,%%mm1       #p1(7,0)\n"

        "pmaddwd         fix_054_054p076,%%mm4           #p1(4,0)\n"
        "punpcklwd   %%mm6,%%mm1   #p1(7,1)\n"

        "pmaddwd         fix_054_054p076,%%mm5           #p1(4,1)\n"
        "psrad           $3,%%mm1         #p1(7,2)\n"

        "pxor            %%mm3,%%mm3       #p1(7,3)\n"

        "punpcklwd       %%mm7,%%mm3       #p1(7,4)\n"

        "psrad           $3,%%mm3         #p1(7,5)\n"

        "paddd           %%mm3,%%mm1       #p1(7,6)\n"

        "movq            %%mm1,%%mm3       #p1(7,7)\n"

        "paddd           %%mm4,%%mm1       #p1(7,8)\n"

        "psubd           %%mm4,%%mm3       #p1(7,9)\n"

        "movq            %%mm1,8*16(%%esi)         #p1(7,10)\n"
        "pxor            %%mm4,%%mm4       #p1(7,12)\n"

        "movq            %%mm3,8*22(%%esi)         #p1(7,11)\n"
        "punpckhwd       %%mm6,%%mm4       #p1(7,13)\n"

        "psrad           $3,%%mm4         #p1(7,14)\n"
        "pxor            %%mm1,%%mm1       #p1(7,15)\n"

        "punpckhwd       %%mm7,%%mm1       #p1(7,16)\n"

        "psrad           $3,%%mm1         #p1(7,17)\n"

        "paddd           %%mm1,%%mm4       #p1(7,18)\n"

        "movq            %%mm4,%%mm3       #p1(7,19)\n"
        "pxor            %%mm1,%%mm1       #p1(8,0)\n"

        "paddd           %%mm5,%%mm3       #p1(7,20)\n"
        "punpcklwd       %%mm6,%%mm1       #p1(8,1)\n"

        "psubd           %%mm5,%%mm4       #p1(7,21)\n"
        "psrad           $3,%%mm1         #p1(8,2)\n"

        "movq            %%mm3,8*17(%%esi)         #p1(7,22)\n"
        "pxor            %%mm5,%%mm5       #p1(8,3)\n"

        "movq            %%mm4,8*23(%%esi)         #p1(7,23)\n"
        "punpcklwd       %%mm7,%%mm5       #p1(8,4)\n"

        "psrad           $3,%%mm5         #p1(8,5)\n"
        "pxor            %%mm4,%%mm4       #p1(8,12)\n"

        "psubd           %%mm5,%%mm1       #p1(8,6)\n"
        "punpckhwd       %%mm6,%%mm4       #p1(8,13)\n"

        "movq            %%mm1,%%mm3       #p1(8,7)\n"
        "psrad           $3,%%mm4         #p1(8,14)\n"

        "paddd           %%mm0,%%mm1       #p1(8,8)\n"
        "pxor            %%mm5,%%mm5       #p1(8,15)\n"

        "psubd           %%mm0,%%mm3       #p1(8,9)\n"
        "movq            8*14(%%ebx),%%mm0         #p1(9,0)\n"

        "punpckhwd       %%mm7,%%mm5       #p1(8,16)\n"
        "pmullw          8*14(%%edi),%%mm0         #p1(9,1)\n"

        "movq            %%mm1,8*18(%%esi)         #p1(8,10)\n"
        "psrad           $3,%%mm5         #p1(8,17)\n"

        "movq            %%mm3,8*20(%%esi)         #p1(8,11)\n"
        "psubd           %%mm5,%%mm4       #p1(8,18)\n"

        "movq            %%mm4,%%mm3       #p1(8,19)\n"
        "movq            8*6(%%ebx),%%mm1          #p1(10,0)\n"

        "paddd           %%mm2,%%mm3       #p1(8,20)\n"
        "pmullw          8*6(%%edi),%%mm1          #p1(10,1)\n"

        "psubd           %%mm2,%%mm4       #p1(8,21)\n"
        "movq            %%mm0,%%mm5                       #p1(11,1)\n"

        "movq            %%mm4,8*21(%%esi)         #p1(8,23)\n"

        "movq            %%mm3,8*19(%%esi)         #p1(8,22)\n"
        "movq            %%mm0,%%mm4                       #p1(11,0)\n"

        "punpcklwd       %%mm1,%%mm4                       #p1(11,2)\n"
        "movq            8*10(%%ebx),%%mm2         #p1(12,0)\n"

        "punpckhwd       %%mm1,%%mm5                       #p1(11,4)\n"
        "pmullw          8*10(%%edi),%%mm2         #p1(12,1)\n"

        "movq            8*2(%%ebx),%%mm3          #p1(13,0)\n"

        "pmullw          8*2(%%edi),%%mm3          #p1(13,1)\n"
        "movq            %%mm2,%%mm6                       #p1(14,0)\n"

        "pmaddwd         fix_117_117,%%mm4       #p1(11,3)\n"
        "movq            %%mm2,%%mm7                       #p1(14,1)\n"

        "pmaddwd         fix_117_117,%%mm5       #p1(11,5)\n"
        "punpcklwd       %%mm3,%%mm6                       #p1(14,2)\n"

        "pmaddwd         fix_117_117,%%mm6       #p1(14,3)\n"
        "punpckhwd       %%mm3,%%mm7                       #p1(14,4)\n"

        "pmaddwd         fix_117_117,%%mm7       #p1(14,5)\n"
        "paddd           %%mm6,%%mm4                       #p1(15,0)\n"

        "paddd           %%mm7,%%mm5                       #p1(15,1)\n"
        "movq            %%mm4,8*24(%%esi)         #p1(15,2)\n"

        "movq            %%mm5,8*25(%%esi)         #p1(15,3)\n"
        "movq            %%mm0,%%mm6                               #p1(16,0)\n"

        "movq            %%mm3,%%mm7                               #p1(16,3)\n"
        "punpcklwd       %%mm2,%%mm6                               #p1(16,1)\n"

        "punpcklwd       %%mm3,%%mm7                               #p1(16,4)\n"
        "pmaddwd         fix_n039_n089,%%mm6             #p1(16,2)\n"

        "pmaddwd         fix_150_n089n039,%%mm7  #p1(16,5)\n"
        "movq            %%mm0,%%mm4                               #p1(16,12)\n"

        "paddd           8*24(%%esi),%%mm6                 #p1(16,6)\n"
        "punpckhwd       %%mm2,%%mm4                               #p1(16,13)\n"

        "paddd           %%mm7,%%mm6                               #p1(16,7)\n"
        "pmaddwd         fix_n039_n089,%%mm4             #p1(16,14)\n"

        "movq            %%mm6,%%mm7                               #p1(16,8)\n"
        "paddd           8*25(%%esi),%%mm4                 #p1(16,18)\n"

        "movq            %%mm3,%%mm5                               #p1(16,15)\n"
        "paddd           8*16(%%esi),%%mm6                 #p1(16,9)\n"

        "punpckhwd       %%mm3,%%mm5                               #p1(16,16)\n"
        "paddd           const_0x2xx8,%%mm6              #p1(16,10)\n"

        "psrad           $9,%%mm6                                 #p1(16,11)\n"
        "pmaddwd         fix_150_n089n039,%%mm5  #p1(16,17)\n"

        "paddd           %%mm5,%%mm4                               #p1(16,19)\n"

        "movq            %%mm4,%%mm5                               #p1(16,20)\n"

        "paddd           8*17(%%esi),%%mm4                 #p1(16,21)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(16,22)\n"

        "psrad           $9,%%mm4                                 #p1(16,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(16,24)\n"

        "movq            %%mm6,8*0(%%esi)                  #p1(16,25)\n"

        "movq            8*16(%%esi),%%mm4                 #p1(16,26)\n"

        "psubd           %%mm7,%%mm4                               #p1(16,27)\n"
        "movq            8*17(%%esi),%%mm6                 #p1(16,30)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(16,28)\n"
        "movq            %%mm1,%%mm7                               #p1(17,3)\n"

        "psrad           $9,%%mm4                                 #p1(16,29)\n"
        "psubd           %%mm5,%%mm6                               #p1(16,31)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(16,32)\n"
        "punpcklwd       %%mm1,%%mm7                               #p1(17,4)\n"

        "pmaddwd         fix_307n256_n196,%%mm7  #p1(17,5)\n"
        "psrad           $9,%%mm6                                 #p1(16,33)\n"

        "packssdw        %%mm6,%%mm4                               #p1(16,34)\n"
        "movq            %%mm4,8*14(%%esi)                 #p1(16,35)\n"

        "movq            %%mm0,%%mm6                               #p1(17,0)\n"
        "movq            %%mm0,%%mm4                               #p1(17,12)\n"

        "punpcklwd       %%mm2,%%mm6                               #p1(17,1)\n"
        "punpckhwd       %%mm2,%%mm4                               #p1(17,13)\n"

        "pmaddwd         fix_n256_n196,%%mm6             #p1(17,2)\n"
        "movq            %%mm1,%%mm5                               #p1(17,15)\n"

        "paddd           8*24(%%esi),%%mm6                 #p1(17,6)\n"
        "punpckhwd       %%mm1,%%mm5                               #p1(17,16)\n"

        "paddd           %%mm7,%%mm6                               #p1(17,7)\n"
        "pmaddwd         fix_n256_n196,%%mm4             #p1(17,14)\n"

        "movq            %%mm6,%%mm7                               #p1(17,8)\n"
        "pmaddwd         fix_307n256_n196,%%mm5  #p1(17,17)\n"

        "paddd           8*18(%%esi),%%mm6                 #p1(17,9)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(17,10)\n"

        "psrad           $9,%%mm6                                 #p1(17,11)\n"
        "paddd           8*25(%%esi),%%mm4                 #p1(17,18)\n"

        "paddd           %%mm5,%%mm4                               #p1(17,19)\n"

        "movq            %%mm4,%%mm5                               #p1(17,20)\n"

        "paddd           8*19(%%esi),%%mm4                 #p1(17,21)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(17,22)\n"

        "psrad           $9,%%mm4                                 #p1(17,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(17,24)\n"

        "movq            %%mm6,8*2(%%esi)                  #p1(17,25)\n"

        "movq            8*18(%%esi),%%mm4                 #p1(17,26)\n"

        "movq            8*19(%%esi),%%mm6                 #p1(17,30)\n"
        "psubd           %%mm7,%%mm4                               #p1(17,27)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(17,28)\n"
        "psubd           %%mm5,%%mm6                               #p1(17,31)\n"

        "psrad           $9,%%mm4                                 #p1(17,29)\n"
        "paddd           const_0x2xx8,%%mm6              #p1(17,32)\n"

        "psrad           $9,%%mm6                                 #p1(17,33)\n"
        "movq            %%mm2,%%mm7                               #p1(18,3)\n"

        "packssdw        %%mm6,%%mm4                               #p1(17,34)\n"
        "movq            %%mm4,8*12(%%esi)                 #p1(17,35)\n"

        "movq            %%mm1,%%mm6                               #p1(18,0)\n"
        "punpcklwd       %%mm2,%%mm7                               #p1(18,4)\n"

        "punpcklwd       %%mm3,%%mm6                               #p1(18,1)\n"
        "pmaddwd         fix_205_n256n039,%%mm7  #p1(18,5)\n"

        "pmaddwd         fix_n039_n256,%%mm6             #p1(18,2)\n"
        "movq            %%mm1,%%mm4                               #p1(18,12)\n"

        "paddd           8*24(%%esi),%%mm6                 #p1(18,6)\n"
        "punpckhwd       %%mm3,%%mm4                               #p1(18,13)\n"

        "paddd           %%mm7,%%mm6                               #p1(18,7)\n"
        "pmaddwd         fix_n039_n256,%%mm4             #p1(18,14)\n"

        "movq            %%mm6,%%mm7                               #p1(18,8)\n"
        "movq            %%mm2,%%mm5                               #p1(18,15)\n"

        "paddd           8*20(%%esi),%%mm6                 #p1(18,9)\n"
        "punpckhwd       %%mm2,%%mm5                               #p1(18,16)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(18,10)\n"

        "psrad           $9,%%mm6                                 #p1(18,11)\n"
        "pmaddwd         fix_205_n256n039,%%mm5  #p1(18,17)\n"

        "paddd           8*25(%%esi),%%mm4                 #p1(18,18)\n"

        "paddd           %%mm5,%%mm4                               #p1(18,19)\n"

        "movq            %%mm4,%%mm5                               #p1(18,20)\n"

        "paddd           8*21(%%esi),%%mm4                 #p1(18,21)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(18,22)\n"

        "psrad           $9,%%mm4                                 #p1(18,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(18,24)\n"

        "movq            %%mm6,8*4(%%esi)                  #p1(18,25)\n"

        "movq            8*20(%%esi),%%mm4                 #p1(18,26)\n"

        "psubd           %%mm7,%%mm4                               #p1(18,27)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(18,28)\n"
        "movq            %%mm0,%%mm7                               #p1(19,3)\n"

        "psrad           $9,%%mm4                                 #p1(18,29)\n"
        "movq            8*21(%%esi),%%mm6                 #p1(18,30)\n"

        "psubd           %%mm5,%%mm6                               #p1(18,31)\n"
        "punpcklwd       %%mm0,%%mm7                               #p1(19,4)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(18,32)\n"

        "psrad           $9,%%mm6                                 #p1(18,33)\n"
        "pmaddwd         fix_029_n089n196,%%mm7  #p1(19,5)\n"

        "packssdw        %%mm6,%%mm4                               #p1(18,34)\n"

        "movq            %%mm4,8*10(%%esi)                 #p1(18,35)\n"
        "movq            %%mm3,%%mm6                               #p1(19,0)\n"

        "punpcklwd       %%mm1,%%mm6                               #p1(19,1)\n"
        "movq            %%mm0,%%mm5                               #p1(19,15)\n"

        "pmaddwd         fix_n196_n089,%%mm6             #p1(19,2)\n"
        "punpckhwd       %%mm0,%%mm5                               #p1(19,16)\n"


        "paddd           8*24(%%esi),%%mm6                 #p1(19,6)\n"
        "movq            %%mm3,%%mm4                               #p1(19,12)\n"

        "paddd           %%mm7,%%mm6                               #p1(19,7)\n"
        "punpckhwd       %%mm1,%%mm4                               #p1(19,13)\n"


        "movq            %%mm6,%%mm7                               #p1(19,8)\n"
        "pmaddwd         fix_n196_n089,%%mm4     #p1(19,14)\n"

        "paddd           8*22(%%esi),%%mm6                 #p1(19,9)\n"

        "pmaddwd         fix_029_n089n196,%%mm5  #p1(19,17)\n"

        "paddd           const_0x2xx8,%%mm6              #p1(19,10)\n"

        "psrad           $9,%%mm6                                 #p1(19,11)\n"
        "paddd           8*25(%%esi),%%mm4                 #p1(19,18)\n"

        "paddd           %%mm5,%%mm4                               #p1(19,19)\n"

        "movq            %%mm4,%%mm5                               #p1(19,20)\n"
        "paddd           8*23(%%esi),%%mm4                 #p1(19,21)\n"
        "paddd           const_0x2xx8,%%mm4              #p1(19,22)\n"
        "psrad           $9,%%mm4                                 #p1(19,23)\n"

        "packssdw        %%mm4,%%mm6                               #p1(19,24)\n"
        "movq            %%mm6,8*6(%%esi)                  #p1(19,25)\n"

        "movq            8*22(%%esi),%%mm4                 #p1(19,26)\n"

        "psubd           %%mm7,%%mm4                               #p1(19,27)\n"
        "movq            8*23(%%esi),%%mm6                 #p1(19,30)\n"

        "paddd           const_0x2xx8,%%mm4              #p1(19,28)\n"
        "psubd           %%mm5,%%mm6                               #p1(19,31)\n"

        "psrad           $9,%%mm4                                 #p1(19,29)\n"
        "paddd           const_0x2xx8,%%mm6              #p1(19,32)\n"

        "psrad           $9,%%mm6                                 #p1(19,33)\n"

        "packssdw        %%mm6,%%mm4                               #p1(19,34)\n"
        "movq            %%mm4,8*8(%%esi)                  #p1(19,35)\n"


        "movl                    %%eax,%%esi\n"

        "movl                    20(%%ebp), %%edi\n"




        "movq            8*0(%%esi),%%mm0          #tran(0)\n"

        "movq            %%mm0,%%mm1                       #tran(1)\n"
        "movq            8*2(%%esi),%%mm2          #tran(2)\n"

        "punpcklwd       %%mm2,%%mm0                       #tran(3)\n"
        "movq            8*4(%%esi),%%mm3          #tran(5)\n"

        "punpckhwd       %%mm2,%%mm1                       #tran(4)\n"
        "movq            8*6(%%esi),%%mm5          #tran(7)\n"

        "movq            %%mm3,%%mm4                       #tran(6)\n"
        "movq            %%mm0,%%mm6                       #tran(10)\n"

        "punpcklwd       %%mm5,%%mm3                       #tran(8)\n"
        "movq            %%mm1,%%mm7                       #tran(11)\n"

        "punpckldq       %%mm3,%%mm0                       #tran(12)\n"

        "punpckhwd       %%mm5,%%mm4                       #tran(9)\n"
        "movq            %%mm0,8*0(%%esi)          #tran(16)\n"

        "punpckhdq       %%mm3,%%mm6                       #tran(13)\n"
        "movq            8*1(%%esi),%%mm0          #tran(20)\n"

        "punpckldq       %%mm4,%%mm1                       #tran(14)\n"
        "movq            %%mm6,8*2(%%esi)          #tran(17)\n"

        "punpckhdq       %%mm4,%%mm7                       #tran(15)\n"
        "movq            %%mm1,8*4(%%esi)          #tran(18)\n"

        "movq            %%mm0,%%mm1                       #tran(21)\n"
        "movq            8*5(%%esi),%%mm3          #tran(25)\n"

        "movq            8*3(%%esi),%%mm2          #tran(22)\n"
        "movq            %%mm3,%%mm4                       #tran(26)\n"

        "punpcklwd       %%mm2,%%mm0                       #tran(23)\n"
        "movq            %%mm7,8*6(%%esi)          #tran(19)\n"

        "punpckhwd       %%mm2,%%mm1                       #tran(24)\n"
        "movq            8*7(%%esi),%%mm5          #tran(27)\n"

        "punpcklwd       %%mm5,%%mm3                       #tran(28)\n"
        "movq            %%mm0,%%mm6                       #tran(30)\n"

        "movq            %%mm1,%%mm7                       #tran(31)\n"
        "punpckhdq       %%mm3,%%mm6                       #tran(33)\n"

        "punpckhwd       %%mm5,%%mm4                       #tran(29)\n"
        "movq            %%mm6,%%mm2                       #p2(1,0)\n"

        "punpckhdq       %%mm4,%%mm7                       #tran(35)\n"
        "movq            8*2(%%esi),%%mm5          #p2(1,2)\n"

        "paddw           %%mm7,%%mm2                       #p2(1,1)\n"
        "paddw           8*6(%%esi),%%mm5          #p2(1,3)\n"

        "punpckldq       %%mm3,%%mm0                       #tran(32)\n"
        "paddw           %%mm5,%%mm2                       #p2(1,4)\n"

        "punpckldq       %%mm4,%%mm1                       #tran(34)\n"
        "movq            8*2(%%esi),%%mm5                  #p2(3,0)\n"

        "pmulhw          fix_117_117,%%mm2       #p2(1,5)\n"
        "movq            %%mm7,%%mm4                               #p2(2,0)\n"

        "pmulhw          fixn089n196p029,%%mm4   #p2(2,1)\n"
        "movq            %%mm6,%%mm3                                       #p2(6,0)\n"

        "pmulhw          fix_n256n039p205,%%mm3          #p2(6,1)\n"

        "pmulhw          fix_n089,%%mm5                  #p2(3,1)\n"

        "movq            %%mm2,8*24(%%eax)         #p2(1,6)\n"

        "movq            8*6(%%esi),%%mm2          #p2(4,0)\n"

        "pmulhw          fix_n196,%%mm2          #p2(4,1)\n"

        "paddw           8*24(%%eax),%%mm4                         #p2(5,0)\n"

        "paddw           8*24(%%eax),%%mm3                 #p2(9,0)\n"

        "paddw           %%mm2,%%mm5                                       #p2(5,1)\n"

        "movq            8*2(%%esi),%%mm2                  #p2(7,0)\n"
        "paddw           %%mm4,%%mm5                                       #p2(5,2)\n"
        "pmulhw          fix_n039,%%mm2                  #p2(7,1)\n"

        "movq            %%mm5,8*1(%%esi)                          #p2(5,3)\n"

        "movq            8*6(%%esi),%%mm4          #p2(8,0)\n"
        "movq            %%mm6,%%mm5                       #p2(10,0)\n"

        "pmulhw          fix_n256,%%mm4          #p2(8,1)\n"

        "pmulhw          fix_n039,%%mm5          #p2(10,1)\n"

        "pmulhw          fix_n256,%%mm6                  #p2(15,0)\n"

        "paddw           %%mm4,%%mm2                               #p2(9,1)\n"

        "movq            %%mm7,%%mm4                       #p2(11,0)\n"

        "pmulhw          fix_n089,%%mm4          #p2(11,1)\n"
        "paddw           %%mm3,%%mm2                               #p2(9,2)\n"

        "movq            %%mm2,8*3(%%esi)                  #p2(9,3)\n"

        "movq            8*2(%%esi),%%mm3                  #p2(13,0)\n"

        "pmulhw          fix_n196,%%mm7                  #p2(16,0)\n"

        "pmulhw          fix_n089n039p150,%%mm3  #p2(13,1)\n"
        "paddw           %%mm4,%%mm5                       #p2(12,0)\n"


        "paddw           8*24(%%eax),%%mm5                 #p2(14,0)\n"

        "movq            8*6(%%esi),%%mm2                  #p2(18,0)\n"

        "pmulhw          fix_n196p307n256,%%mm2  #p2(18,1)\n"
        "paddw           %%mm3,%%mm5                               #p2(14,1)\n"

        "movq            %%mm5,8*5(%%esi)                  #p2(14,2)\n"
        "paddw           %%mm7,%%mm6                               #p2(17,0)\n"

        "paddw           8*24(%%eax),%%mm6                 #p2(19,0)\n"
        "movq            %%mm1,%%mm3                               #p2(21,0)\n"

        "movq            8*4(%%esi),%%mm4                  #p2(20,0)\n"
        "paddw           %%mm2,%%mm6                               #p2(19,1)\n"

        "movq            %%mm6,8*7(%%esi)                  #p2(19,2)\n"
        "movq            %%mm4,%%mm5                               #p2(20,1)\n"

        "movq            8*0(%%esi),%%mm7                  #p2(26,0)\n"

        "pmulhw          fix_054p076,%%mm4               #p2(20,2)\n"
        "psubw           %%mm0,%%mm7                               #p2(27,0)\n"

        "pmulhw          fix_054,%%mm3                   #p2(21,1)\n"
        "movq            %%mm0,%%mm2                               #p2(26,1)\n"

        "pmulhw          fix_054,%%mm5                   #p2(23,0)\n"
        "psraw           $3,%%mm7                                 #p2(27,1)\n"

        "paddw           8*0(%%esi),%%mm2                  #p2(26,2)\n"
        "movq            %%mm7,%%mm6                               #p2(28,0)\n"

        "pmulhw          fix_054n184,%%mm1               #p2(24,0)\n"
        "psraw           $3,%%mm2                                 #p2(26,3)\n"

        "paddw           %%mm3,%%mm4                               #p2(22,0)\n"
        "paddw           %%mm1,%%mm5                               #p2(25,0)\n"

        "psubw           %%mm5,%%mm6                               #p2(29,0)\n"
        "movq            %%mm2,%%mm3                               #p2(30,0)\n"

        "paddw           %%mm4,%%mm2                               #p2(30,1)\n"
        "paddw           %%mm5,%%mm7                               #p2(28,1)\n"

        "movq            %%mm2,%%mm1                               #p2(32,0)\n"
        "psubw           %%mm4,%%mm3                               #p2(31,0)\n"

        "paddw           8*5(%%esi),%%mm1                  #p2(32,1)\n"
        "movq            %%mm7,%%mm0                               #p2(33,0)\n"

        "psubw           8*5(%%esi),%%mm2                  #p2(32,2)\n"
        "movq            %%mm6,%%mm4                               #p2(34,0)\n"

        "paddw           const_0x0808,%%mm1              #p2(32,3)\n"

        "paddw           const_0x0808,%%mm2              #p2(32,4)\n"
        "psraw           $4,%%mm1                                 #p2(32,5)\n"

        "psraw           $4,%%mm2                                 #p2(32,6)\n"
        "paddw           8*7(%%esi),%%mm7                  #p2(33,1)\n"

        "packuswb        %%mm2,%%mm1                               #p2(32,7)\n"
        "psubw           8*7(%%esi),%%mm0                  #p2(33,2)\n"

        "paddw           const_0x0808,%%mm7              #p2(33,3)\n"

        "paddw           const_0x0808,%%mm0              #p2(33,4)\n"
        "psraw           $4,%%mm7                                 #p2(33,5)\n"

        "psraw           $4,%%mm0                                 #p2(33,6)\n"
        "paddw           8*3(%%esi),%%mm4                  #p2(34,1)\n"

        "packuswb        %%mm0,%%mm7                               #p2(33,7)\n"
        "psubw           8*3(%%esi),%%mm6                  #p2(34,2)\n"

        "paddw           const_0x0808,%%mm4              #p2(34,3)\n"
        "movq            %%mm3,%%mm5                               #p2(35,0)\n"

        "paddw           const_0x0808,%%mm6              #p2(34,4)\n"
        "psraw           $4,%%mm4                                 #p2(34,5)\n"

        "psraw           $4,%%mm6                                 #p2(34,6)\n"
        "paddw           8*1(%%esi),%%mm3                  #p2(35,1)\n"

        "packuswb        %%mm6,%%mm4                               #p2(34,7)\n"
        "psubw           8*1(%%esi),%%mm5                  #p2(35,2)\n"

        "movq            %%mm1,%%mm0                               #p2(36,0)\n"
        "paddw           const_0x0808,%%mm3              #p2(35,3)\n"

        "paddw           const_0x0808,%%mm5              #p2(35,4)\n"
        "punpcklbw       %%mm7,%%mm0                               #p2(36,1)\n"

        "psraw           $4,%%mm3                                 #p2(35,5)\n"
        "movq            %%mm4,%%mm2                               #p2(37,0)\n"

        "psraw           $4,%%mm5                                 #p2(35,6)\n"
        "movq            %%mm0,%%mm6                               #p2(38,0)\n"

        "packuswb        %%mm5,%%mm3                               #p2(35,7)\n"
        "movl                    %%edi,%%ebx                             #p2(42,0)\n"

        "punpckhbw       %%mm1,%%mm7                               #p2(36,2)\n"
        "addl                    24(%%ebp),%%edi\n"
        "movl                    %%edi,%%ecx                    #p2(42,1)\n"

        "punpcklbw       %%mm3,%%mm2                               #p2(37,1)\n"
        "addl                    24(%%ebp),%%edi\n"
        "movl                    %%edi,%%edx                    #p2(42,2)\n"

        "punpckhbw       %%mm4,%%mm3                               #p2(37,2)\n"

        "punpcklwd       %%mm2,%%mm0                               #p2(38,1)\n"
        "movq            %%mm3,%%mm5                               #p2(39,0)\n"

        "punpckhwd       %%mm2,%%mm6                               #p2(38,2)\n"
        "movq            %%mm0,%%mm1                               #p2(40,0)\n"

        "punpcklwd       %%mm7,%%mm3                               #p2(39,1)\n"

        "punpckldq       %%mm3,%%mm0                               #p2(40,1)\n"

        "punpckhdq       %%mm3,%%mm1                               #p2(40,2)\n"
        "movq            %%mm0,(%%ebx)                             #p2(43,0)\n"

        "punpckhwd       %%mm7,%%mm5                               #p2(39,2)\n"
        "movq            %%mm1,(%%ecx)                             #p2(43,1)\n"

        "movq            %%mm6,%%mm4                               #p2(41,0)\n"
        "addl                    24(%%ebp),%%edi\n"
        "movl                    %%edi,%%ebx                   #p2(43,3)\n"

        "punpckldq       %%mm5,%%mm4                               #p2(41,1)\n"

        "punpckhdq       %%mm5,%%mm6                               #p2(41,2)\n"
        "movq            %%mm4,(%%edx)                             #p2(43,2)\n"

        "movq            %%mm6,(%%ebx)                             #p2(43,5)\n"




        "addl                    $64,%%esi\n"
        "addl                    24(%%ebp),%%edi\n"



        "movq            8*0(%%esi),%%mm0          #tran(0)\n"

        "movq            %%mm0,%%mm1                       #tran(1)\n"
        "movq            8*2(%%esi),%%mm2          #tran(2)\n"

        "punpcklwd       %%mm2,%%mm0                       #tran(3)\n"
        "movq            8*4(%%esi),%%mm3          #tran(5)\n"

        "punpckhwd       %%mm2,%%mm1                       #tran(4)\n"
        "movq            8*6(%%esi),%%mm5          #tran(7)\n"

        "movq            %%mm3,%%mm4                       #tran(6)\n"
        "movq            %%mm0,%%mm6                       #tran(10)\n"

        "punpcklwd       %%mm5,%%mm3                       #tran(8)\n"
        "movq            %%mm1,%%mm7                       #tran(11)\n"

        "punpckldq       %%mm3,%%mm0                       #tran(12)\n"

        "punpckhwd       %%mm5,%%mm4                       #tran(9)\n"
        "movq            %%mm0,8*0(%%esi)          #tran(16)\n"

        "punpckhdq       %%mm3,%%mm6                       #tran(13)\n"
        "movq            8*1(%%esi),%%mm0          #tran(20)\n"

        "punpckldq       %%mm4,%%mm1                       #tran(14)\n"
        "movq            %%mm6,8*2(%%esi)          #tran(17)\n"

        "punpckhdq       %%mm4,%%mm7                       #tran(15)\n"
        "movq            %%mm1,8*4(%%esi)          #tran(18)\n"

        "movq            %%mm0,%%mm1                       #tran(21)\n"
        "movq            8*5(%%esi),%%mm3          #tran(25)\n"

        "movq            8*3(%%esi),%%mm2          #tran(22)\n"
        "movq            %%mm3,%%mm4                       #tran(26)\n"

        "punpcklwd       %%mm2,%%mm0                       #tran(23)\n"
        "movq            %%mm7,8*6(%%esi)          #tran(19)\n"

        "punpckhwd       %%mm2,%%mm1                       #tran(24)\n"
        "movq            8*7(%%esi),%%mm5          #tran(27)\n"

        "punpcklwd       %%mm5,%%mm3                       #tran(28)\n"
        "movq            %%mm0,%%mm6                       #tran(30)\n"

        "movq            %%mm1,%%mm7                       #tran(31)\n"
        "punpckhdq       %%mm3,%%mm6                       #tran(33)\n"

        "punpckhwd       %%mm5,%%mm4                       #tran(29)\n"
        "movq            %%mm6,%%mm2                       #p2(1,0)\n"

        "punpckhdq       %%mm4,%%mm7                       #tran(35)\n"
        "movq            8*2(%%esi),%%mm5          #p2(1,2)\n"

        "paddw           %%mm7,%%mm2                       #p2(1,1)\n"
        "paddw           8*6(%%esi),%%mm5          #p2(1,3)\n"

        "punpckldq       %%mm3,%%mm0                       #tran(32)\n"
        "paddw           %%mm5,%%mm2                       #p2(1,4)\n"

        "punpckldq       %%mm4,%%mm1                       #tran(34)\n"
        "movq            8*2(%%esi),%%mm5                  #p2(3,0)\n"

        "pmulhw          fix_117_117,%%mm2       #p2(1,5)\n"
        "movq            %%mm7,%%mm4                               #p2(2,0)\n"

        "pmulhw          fixn089n196p029,%%mm4   #p2(2,1)\n"
        "movq            %%mm6,%%mm3                                       #p2(6,0)\n"

        "pmulhw          fix_n256n039p205,%%mm3          #p2(6,1)\n"

        "pmulhw          fix_n089,%%mm5                  #p2(3,1)\n"

        "movq            %%mm2,8*24(%%eax)         #p2(1,6)\n"

        "movq            8*6(%%esi),%%mm2          #p2(4,0)\n"

        "pmulhw          fix_n196,%%mm2          #p2(4,1)\n"

        "paddw           8*24(%%eax),%%mm4                         #p2(5,0)\n"

        "paddw           8*24(%%eax),%%mm3                 #p2(9,0)\n"

        "paddw           %%mm2,%%mm5                                       #p2(5,1)\n"

        "movq            8*2(%%esi),%%mm2                  #p2(7,0)\n"
        "paddw           %%mm4,%%mm5                                       #p2(5,2)\n"
        "pmulhw          fix_n039,%%mm2                  #p2(7,1)\n"

        "movq            %%mm5,8*1(%%esi)                          #p2(5,3)\n"

        "movq            8*6(%%esi),%%mm4          #p2(8,0)\n"
        "movq            %%mm6,%%mm5                       #p2(10,0)\n"

        "pmulhw          fix_n256,%%mm4          #p2(8,1)\n"

        "pmulhw          fix_n039,%%mm5          #p2(10,1)\n"

        "pmulhw          fix_n256,%%mm6                  #p2(15,0)\n"

        "paddw           %%mm4,%%mm2                               #p2(9,1)\n"

        "movq            %%mm7,%%mm4                       #p2(11,0)\n"

        "pmulhw          fix_n089,%%mm4          #p2(11,1)\n"
        "paddw           %%mm3,%%mm2                               #p2(9,2)\n"

        "movq            %%mm2,8*3(%%esi)                  #p2(9,3)\n"

        "movq            8*2(%%esi),%%mm3                  #p2(13,0)\n"

        "pmulhw          fix_n196,%%mm7                  #p2(16,0)\n"

        "pmulhw          fix_n089n039p150,%%mm3  #p2(13,1)\n"
        "paddw           %%mm4,%%mm5                       #p2(12,0)\n"


        "paddw           8*24(%%eax),%%mm5                 #p2(14,0)\n"

        "movq            8*6(%%esi),%%mm2                  #p2(18,0)\n"

        "pmulhw          fix_n196p307n256,%%mm2  #p2(18,1)\n"
        "paddw           %%mm3,%%mm5                               #p2(14,1)\n"

        "movq            %%mm5,8*5(%%esi)                  #p2(14,2)\n"
        "paddw           %%mm7,%%mm6                               #p2(17,0)\n"

        "paddw           8*24(%%eax),%%mm6                 #p2(19,0)\n"
        "movq            %%mm1,%%mm3                               #p2(21,0)\n"

        "movq            8*4(%%esi),%%mm4                  #p2(20,0)\n"
        "paddw           %%mm2,%%mm6                               #p2(19,1)\n"

        "movq            %%mm6,8*7(%%esi)                  #p2(19,2)\n"
        "movq            %%mm4,%%mm5                               #p2(20,1)\n"

        "movq            8*0(%%esi),%%mm7                  #p2(26,0)\n"

        "pmulhw          fix_054p076,%%mm4               #p2(20,2)\n"
        "psubw           %%mm0,%%mm7                               #p2(27,0)\n"

        "pmulhw          fix_054,%%mm3                   #p2(21,1)\n"
        "movq            %%mm0,%%mm2                               #p2(26,1)\n"

        "pmulhw          fix_054,%%mm5                   #p2(23,0)\n"
        "psraw           $3,%%mm7                                 #p2(27,1)\n"

        "paddw           8*0(%%esi),%%mm2                  #p2(26,2)\n"
        "movq            %%mm7,%%mm6                               #p2(28,0)\n"

        "pmulhw          fix_054n184,%%mm1               #p2(24,0)\n"
        "psraw           $3,%%mm2                                 #p2(26,3)\n"

        "paddw           %%mm3,%%mm4                               #p2(22,0)\n"
        "paddw           %%mm1,%%mm5                               #p2(25,0)\n"

        "psubw           %%mm5,%%mm6                               #p2(29,0)\n"
        "movq            %%mm2,%%mm3                               #p2(30,0)\n"

        "paddw           %%mm4,%%mm2                               #p2(30,1)\n"
        "paddw           %%mm5,%%mm7                               #p2(28,1)\n"

        "movq            %%mm2,%%mm1                               #p2(32,0)\n"
        "psubw           %%mm4,%%mm3                               #p2(31,0)\n"

        "paddw           8*5(%%esi),%%mm1                  #p2(32,1)\n"
        "movq            %%mm7,%%mm0                               #p2(33,0)\n"

        "psubw           8*5(%%esi),%%mm2                  #p2(32,2)\n"
        "movq            %%mm6,%%mm4                               #p2(34,0)\n"

        "paddw           const_0x0808,%%mm1              #p2(32,3)\n"

        "paddw           const_0x0808,%%mm2              #p2(32,4)\n"
        "psraw           $4,%%mm1                                 #p2(32,5)\n"

        "psraw           $4,%%mm2                                 #p2(32,6)\n"
        "paddw           8*7(%%esi),%%mm7                  #p2(33,1)\n"

        "packuswb        %%mm2,%%mm1                               #p2(32,7)\n"
        "psubw           8*7(%%esi),%%mm0                  #p2(33,2)\n"

        "paddw           const_0x0808,%%mm7              #p2(33,3)\n"

        "paddw           const_0x0808,%%mm0              #p2(33,4)\n"
        "psraw           $4,%%mm7                                 #p2(33,5)\n"

        "psraw           $4,%%mm0                                 #p2(33,6)\n"
        "paddw           8*3(%%esi),%%mm4                  #p2(34,1)\n"

        "packuswb        %%mm0,%%mm7                               #p2(33,7)\n"
        "psubw           8*3(%%esi),%%mm6                  #p2(34,2)\n"

        "paddw           const_0x0808,%%mm4              #p2(34,3)\n"
        "movq            %%mm3,%%mm5                               #p2(35,0)\n"

        "paddw           const_0x0808,%%mm6              #p2(34,4)\n"
        "psraw           $4,%%mm4                                 #p2(34,5)\n"

        "psraw           $4,%%mm6                                 #p2(34,6)\n"
        "paddw           8*1(%%esi),%%mm3                  #p2(35,1)\n"

        "packuswb        %%mm6,%%mm4                               #p2(34,7)\n"
        "psubw           8*1(%%esi),%%mm5                  #p2(35,2)\n"

        "movq            %%mm1,%%mm0                               #p2(36,0)\n"
        "paddw           const_0x0808,%%mm3              #p2(35,3)\n"

        "paddw           const_0x0808,%%mm5              #p2(35,4)\n"
        "punpcklbw       %%mm7,%%mm0                               #p2(36,1)\n"

        "psraw           $4,%%mm3                                 #p2(35,5)\n"
        "movq            %%mm4,%%mm2                               #p2(37,0)\n"

        "psraw           $4,%%mm5                                 #p2(35,6)\n"
        "movq            %%mm0,%%mm6                               #p2(38,0)\n"

        "packuswb        %%mm5,%%mm3                               #p2(35,7)\n"
        "movl                    %%edi,%%ebx                             #p2(42,0)\n"

        "punpckhbw       %%mm1,%%mm7                               #p2(36,2)\n"
        "addl                    24(%%ebp),%%edi\n"
        "movl                    %%edi,%%ecx                    #p2(42,1)\n"

        "punpcklbw       %%mm3,%%mm2                               #p2(37,1)\n"
        "addl                    24(%%ebp),%%edi\n"
        "movl                    %%edi,%%edx                    #p2(42,2)\n"

        "punpckhbw       %%mm4,%%mm3                               #p2(37,2)\n"

        "punpcklwd       %%mm2,%%mm0                               #p2(38,1)\n"
        "movq            %%mm3,%%mm5                               #p2(39,0)\n"

        "punpckhwd       %%mm2,%%mm6                               #p2(38,2)\n"
        "movq            %%mm0,%%mm1                               #p2(40,0)\n"

        "punpcklwd       %%mm7,%%mm3                               #p2(39,1)\n"

        "punpckldq       %%mm3,%%mm0                               #p2(40,1)\n"

        "punpckhdq       %%mm3,%%mm1                               #p2(40,2)\n"
        "movq            %%mm0,(%%ebx)                             #p2(43,0)\n"

        "punpckhwd       %%mm7,%%mm5                               #p2(39,2)\n"
        "movq            %%mm1,(%%ecx)                             #p2(43,1)\n"

        "movq            %%mm6,%%mm4                               #p2(41,0)\n"
        "addl                    24(%%ebp),%%edi\n"
        "movl                    %%edi,%%ebx                   #p2(43,3)\n"

        "punpckldq       %%mm5,%%mm4                               #p2(41,1)\n"

        "punpckhdq       %%mm5,%%mm6                               #p2(41,2)\n"
        "movq            %%mm4,(%%edx)                             #p2(43,2)\n"

        "movq            %%mm6,(%%ebx)                             #p2(43,5)\n"



        "#        emms\n"

	: "=D" (quantptr), "=b" (inptr), "=S" (wsptr)
	: "D" (quantptr), "b" (inptr), "S" (wsptr)
	: "memory", "%eax", "%ecx", "%edx"

	);
}
#else

/* This assembly code will compile with Visual C++ */
#ifdef _MSC_EXTENSIONS
void domidct8x8llmW(short *inptr, short *quantptr, int *wsptr,
			   u_char *outptr, int stride)
{
	static	__int64 fix_029_n089n196	= 0x098ea46e098ea46e;
	static	__int64 fix_n196_n089		= 0xc13be333c13be333;
	static	__int64 fix_205_n256n039	= 0x41b3a18141b3a181;
	static	__int64 fix_n039_n256		= 0xf384adfdf384adfd;
	static	__int64 fix_307n256_n196	= 0x1051c13b1051c13b;
	static	__int64 fix_n256_n196		= 0xadfdc13badfdc13b;
	static	__int64 fix_150_n089n039	= 0x300bd6b7300bd6b7;
	static	__int64 fix_n039_n089		= 0xf384e333f384e333;
	static	__int64 fix_117_117			= 0x25a125a125a125a1;
	static	__int64 fix_054_054p076		= 0x115129cf115129cf;
	static	__int64 fix_054n184_054		= 0xd6301151d6301151;

	static	__int64 fix_054n184 		= 0xd630d630d630d630;
	static	__int64 fix_054				= 0x1151115111511151;
	static	__int64 fix_054p076			= 0x29cf29cf29cf29cf;
	static	__int64 fix_n196p307n256	= 0xd18cd18cd18cd18c;
	static	__int64 fix_n089n039p150	= 0x06c206c206c206c2;
	static	__int64 fix_n256			= 0xadfdadfdadfdadfd;
	static	__int64 fix_n039			= 0xf384f384f384f384;
	static	__int64 fix_n256n039p205	= 0xe334e334e334e334;
	static	__int64 fix_n196			= 0xc13bc13bc13bc13b;
	static	__int64 fix_n089			= 0xe333e333e333e333;
	static	__int64 fixn089n196p029		= 0xadfcadfcadfcadfc;

	static  __int64 const_0x2xx8		= 0x0000010000000100;
	static  __int64 const_0x0808		= 0x0808080808080808;

	__asm{

	mov		edi, quantptr
	mov		ebx, inptr
	mov		esi, wsptr

	add		esi, 0x07		;align wsptr to qword
	and		esi, 0xfffffff8	;align wsptr to qword

	mov		eax, esi

	/* Pass 1. */

	movq		mm0, [ebx + 8*4]	;p1(1,0)
	pmullw		mm0, [edi + 8*4]	;p1(1,1)

    movq		mm1, [ebx + 8*12]	;p1(2,0)
	pmullw		mm1, [edi + 8*12]	;p1(2,1)

	movq		mm6, [ebx + 8*0]	;p1(5,0)
	pmullw		mm6, [edi + 8*0]	;p1(5,1)

	movq		mm2, mm0				;p1(3,0)
	movq		mm7, [ebx + 8*8]	;p1(6,0)

	punpcklwd	mm0, mm1				;p1(3,1)
	pmullw		mm7, [edi + 8*8]	;p1(6,1)

	movq		mm4, mm0				;p1(3,2)
	punpckhwd	mm2, mm1				;p1(3,4)

	pmaddwd		mm0, fix_054n184_054	;p1(3,3)
	movq		mm5, mm2				;p1(3,5)

	pmaddwd		mm2, fix_054n184_054	;p1(3,6)
	pxor		mm1, mm1	;p1(7,0)

	pmaddwd		mm4, fix_054_054p076		;p1(4,0)
	punpcklwd   mm1, mm6	;p1(7,1)

	pmaddwd		mm5, fix_054_054p076		;p1(4,1)
	psrad		mm1, 3		;p1(7,2)

	pxor		mm3, mm3	;p1(7,3)

	punpcklwd	mm3, mm7	;p1(7,4)

	psrad		mm3, 3		;p1(7,5)

	paddd		mm1, mm3	;p1(7,6)

	movq		mm3, mm1	;p1(7,7)

	paddd		mm1, mm4	;p1(7,8)

	psubd		mm3, mm4	;p1(7,9)

	movq		[esi + 8*16], mm1	;p1(7,10)
	pxor		mm4, mm4	;p1(7,12)

	movq		[esi + 8*22], mm3	;p1(7,11)
	punpckhwd	mm4, mm6	;p1(7,13)

	psrad		mm4, 3		;p1(7,14)
	pxor		mm1, mm1	;p1(7,15)

	punpckhwd	mm1, mm7	;p1(7,16)

	psrad		mm1, 3		;p1(7,17)

	paddd		mm4, mm1	;p1(7,18)

	movq		mm3, mm4	;p1(7,19)
	pxor		mm1, mm1	;p1(8,0)

	paddd		mm3, mm5	;p1(7,20)
	punpcklwd	mm1, mm6	;p1(8,1)

	psubd		mm4, mm5	;p1(7,21)
	psrad		mm1, 3		;p1(8,2)

	movq		[esi + 8*17], mm3	;p1(7,22)
	pxor		mm5, mm5	;p1(8,3)

	movq		[esi + 8*23], mm4	;p1(7,23)
	punpcklwd	mm5, mm7	;p1(8,4)

	psrad		mm5, 3		;p1(8,5)
	pxor		mm4, mm4	;p1(8,12)

	psubd		mm1, mm5	;p1(8,6)
	punpckhwd	mm4, mm6	;p1(8,13)

	movq		mm3, mm1	;p1(8,7)
	psrad		mm4, 3		;p1(8,14)

	paddd		mm1, mm0	;p1(8,8)
	pxor		mm5, mm5	;p1(8,15)

	psubd		mm3, mm0	;p1(8,9)
	movq		mm0, [ebx + 8*14]	;p1(9,0)

	punpckhwd	mm5, mm7	;p1(8,16)
	pmullw		mm0, [edi + 8*14]	;p1(9,1)

	movq		[esi + 8*18], mm1	;p1(8,10)
	psrad		mm5, 3		;p1(8,17)

	movq		[esi + 8*20], mm3	;p1(8,11)
	psubd		mm4, mm5	;p1(8,18)

	movq		mm3, mm4	;p1(8,19)
	movq		mm1, [ebx + 8*6]	;p1(10,0)

	paddd		mm3, mm2	;p1(8,20)
	pmullw		mm1, [edi + 8*6]	;p1(10,1)

	psubd		mm4, mm2	;p1(8,21)
	movq		mm5, mm0			;p1(11,1)

	movq		[esi + 8*21], mm4	;p1(8,23)

	movq		[esi + 8*19], mm3	;p1(8,22)
	movq		mm4, mm0			;p1(11,0)

	punpcklwd	mm4, mm1			;p1(11,2)
	movq		mm2, [ebx + 8*10]	;p1(12,0)

	punpckhwd	mm5, mm1			;p1(11,4)
	pmullw		mm2, [edi + 8*10]	;p1(12,1)

	movq		mm3, [ebx + 8*2]	;p1(13,0)

	pmullw		mm3, [edi + 8*2]	;p1(13,1)
	movq		mm6, mm2			;p1(14,0)

	pmaddwd		mm4, fix_117_117	;p1(11,3)
	movq		mm7, mm2			;p1(14,1)

	pmaddwd		mm5, fix_117_117	;p1(11,5)
	punpcklwd	mm6, mm3			;p1(14,2)

	pmaddwd		mm6, fix_117_117	;p1(14,3)
	punpckhwd	mm7, mm3			;p1(14,4)

	pmaddwd		mm7, fix_117_117	;p1(14,5)
	paddd		mm4, mm6			;p1(15,0)

	paddd		mm5, mm7			;p1(15,1)
   	movq		[esi+8*24], mm4		;p1(15,2)

	movq		[esi+8*25], mm5		;p1(15,3)
	movq		mm6, mm0				;p1(16,0)

	movq		mm7, mm3				;p1(16,3)
	punpcklwd	mm6, mm2				;p1(16,1)

	punpcklwd	mm7, mm3				;p1(16,4)
	pmaddwd		mm6, fix_n039_n089		;p1(16,2)

	pmaddwd		mm7, fix_150_n089n039	;p1(16,5)
	movq		mm4, mm0				;p1(16,12)

	paddd		mm6, [esi+8*24]			;p1(16,6)
	punpckhwd	mm4, mm2				;p1(16,13)

	paddd		mm6, mm7				;p1(16,7)
	pmaddwd		mm4, fix_n039_n089		;p1(16,14)

	movq		mm7, mm6				;p1(16,8)
	paddd		mm4, [esi+8*25]			;p1(16,18)

	movq		mm5, mm3				;p1(16,15)
	paddd		mm6, [esi + 8*16]		;p1(16,9)

	punpckhwd	mm5, mm3				;p1(16,16)
	paddd		mm6, const_0x2xx8		;p1(16,10)

	psrad		mm6, 9					;p1(16,11)
	pmaddwd		mm5, fix_150_n089n039	;p1(16,17)

	paddd		mm4, mm5				;p1(16,19)

	movq		mm5, mm4				;p1(16,20)

	paddd		mm4, [esi + 8*17]		;p1(16,21)

	paddd		mm4, const_0x2xx8		;p1(16,22)

	psrad		mm4, 9					;p1(16,23)

	packssdw	mm6, mm4				;p1(16,24)

	movq		[esi + 8*0], mm6		;p1(16,25)

	movq		mm4, [esi + 8*16]		;p1(16,26)

	psubd		mm4, mm7				;p1(16,27)
	movq		mm6, [esi + 8*17]		;p1(16,30)

	paddd		mm4, const_0x2xx8		;p1(16,28)
	movq		mm7, mm1				;p1(17,3)

	psrad		mm4, 9					;p1(16,29)
	psubd		mm6, mm5				;p1(16,31)

	paddd		mm6, const_0x2xx8		;p1(16,32)
	punpcklwd	mm7, mm1				;p1(17,4)

	pmaddwd		mm7, fix_307n256_n196	;p1(17,5)
	psrad		mm6, 9					;p1(16,33)

	packssdw	mm4, mm6				;p1(16,34)
	movq		[esi + 8*14], mm4		;p1(16,35)

	movq		mm6, mm0				;p1(17,0)
	movq		mm4, mm0				;p1(17,12)

	punpcklwd	mm6, mm2				;p1(17,1)
	punpckhwd	mm4, mm2				;p1(17,13)

	pmaddwd		mm6, fix_n256_n196		;p1(17,2)
	movq		mm5, mm1				;p1(17,15)

	paddd		mm6, [esi+8*24]			;p1(17,6)
	punpckhwd	mm5, mm1				;p1(17,16)

	paddd		mm6, mm7				;p1(17,7)
	pmaddwd		mm4, fix_n256_n196		;p1(17,14)

	movq		mm7, mm6				;p1(17,8)
	pmaddwd		mm5, fix_307n256_n196	;p1(17,17)

	paddd		mm6, [esi + 8*18]		;p1(17,9)

	paddd		mm6, const_0x2xx8		;p1(17,10)

	psrad		mm6, 9					;p1(17,11)
	paddd		mm4, [esi+8*25]			;p1(17,18)

	paddd		mm4, mm5				;p1(17,19)

	movq		mm5, mm4				;p1(17,20)

	paddd		mm4, [esi + 8*19]		;p1(17,21)

	paddd		mm4, const_0x2xx8		;p1(17,22)

	psrad		mm4, 9					;p1(17,23)

	packssdw	mm6, mm4				;p1(17,24)

	movq		[esi + 8*2], mm6		;p1(17,25)

	movq		mm4, [esi + 8*18]		;p1(17,26)

	movq		mm6, [esi + 8*19]		;p1(17,30)
	psubd		mm4, mm7				;p1(17,27)

	paddd		mm4, const_0x2xx8		;p1(17,28)
	psubd		mm6, mm5				;p1(17,31)

	psrad		mm4, 9					;p1(17,29)
	paddd		mm6, const_0x2xx8		;p1(17,32)

	psrad		mm6, 9					;p1(17,33)
	movq		mm7, mm2				;p1(18,3)

	packssdw	mm4, mm6				;p1(17,34)
	movq		[esi + 8*12], mm4		;p1(17,35)

	movq		mm6, mm1				;p1(18,0)
	punpcklwd	mm7, mm2				;p1(18,4)

	punpcklwd	mm6, mm3				;p1(18,1)
	pmaddwd		mm7, fix_205_n256n039	;p1(18,5)

	pmaddwd		mm6, fix_n039_n256		;p1(18,2)
	movq		mm4, mm1				;p1(18,12)

	paddd		mm6, [esi+8*24]			;p1(18,6)
	punpckhwd	mm4, mm3				;p1(18,13)

	paddd		mm6, mm7				;p1(18,7)
	pmaddwd		mm4, fix_n039_n256		;p1(18,14)

	movq		mm7, mm6				;p1(18,8)
	movq		mm5, mm2				;p1(18,15)

	paddd		mm6, [esi + 8*20]		;p1(18,9)
	punpckhwd	mm5, mm2				;p1(18,16)

	paddd		mm6, const_0x2xx8		;p1(18,10)

	psrad		mm6, 9					;p1(18,11)
	pmaddwd		mm5, fix_205_n256n039	;p1(18,17)

	paddd		mm4, [esi+8*25]			;p1(18,18)

	paddd		mm4, mm5				;p1(18,19)

	movq		mm5, mm4				;p1(18,20)

	paddd		mm4, [esi + 8*21]		;p1(18,21)

	paddd		mm4, const_0x2xx8		;p1(18,22)

	psrad		mm4, 9					;p1(18,23)

	packssdw	mm6, mm4				;p1(18,24)

	movq		[esi + 8*4], mm6		;p1(18,25)

	movq		mm4, [esi + 8*20]		;p1(18,26)

	psubd		mm4, mm7				;p1(18,27)

	paddd		mm4, const_0x2xx8		;p1(18,28)
	movq		mm7, mm0				;p1(19,3)

	psrad		mm4, 9					;p1(18,29)
	movq		mm6, [esi + 8*21]		;p1(18,30)

	psubd		mm6, mm5				;p1(18,31)
	punpcklwd	mm7, mm0				;p1(19,4)

	paddd		mm6, const_0x2xx8		;p1(18,32)

	psrad		mm6, 9					;p1(18,33)
	pmaddwd		mm7, fix_029_n089n196	;p1(19,5)

	packssdw	mm4, mm6				;p1(18,34)

	movq		[esi + 8*10], mm4		;p1(18,35)
	movq		mm6, mm3				;p1(19,0)

	punpcklwd	mm6, mm1				;p1(19,1)
	movq		mm5, mm0				;p1(19,15)

	pmaddwd		mm6, fix_n196_n089		;p1(19,2)
	punpckhwd	mm5, mm0				;p1(19,16)


	paddd		mm6, [esi+8*24]			;p1(19,6)
	movq		mm4, mm3				;p1(19,12)

	paddd		mm6, mm7				;p1(19,7)
	punpckhwd	mm4, mm1				;p1(19,13)


	movq		mm7, mm6				;p1(19,8)
	pmaddwd		mm4, fix_n196_n089  	;p1(19,14)

	paddd		mm6, [esi + 8*22]		;p1(19,9)

	pmaddwd		mm5, fix_029_n089n196	;p1(19,17)

	paddd		mm6, const_0x2xx8		;p1(19,10)

	psrad		mm6, 9					;p1(19,11)
	paddd		mm4, [esi+8*25]			;p1(19,18)

	paddd		mm4, mm5				;p1(19,19)

	movq		mm5, mm4				;p1(19,20)
	paddd		mm4, [esi + 8*23]		;p1(19,21)
	paddd		mm4, const_0x2xx8		;p1(19,22)
	psrad		mm4, 9					;p1(19,23)

	packssdw	mm6, mm4				;p1(19,24)
	movq		[esi + 8*6], mm6		;p1(19,25)

	movq		mm4, [esi + 8*22]		;p1(19,26)

	psubd		mm4, mm7				;p1(19,27)
	movq		mm6, [esi + 8*23]		;p1(19,30)

	paddd		mm4, const_0x2xx8		;p1(19,28)
	psubd		mm6, mm5				;p1(19,31)

	psrad		mm4, 9					;p1(19,29)
	paddd		mm6, const_0x2xx8		;p1(19,32)

	psrad		mm6, 9					;p1(19,33)

	packssdw	mm4, mm6				;p1(19,34)
	movq		[esi + 8*8], mm4		;p1(19,35)

//************************************************************

	add		edi, 8
	add		ebx, 8
	add		esi, 8

//************************************************************
	/* Pass 1. */

	movq		mm0, [ebx + 8*4]	;p1(1,0)
	pmullw		mm0, [edi + 8*4]	;p1(1,1)

    movq		mm1, [ebx + 8*12]	;p1(2,0)
	pmullw		mm1, [edi + 8*12]	;p1(2,1)

	movq		mm6, [ebx + 8*0]	;p1(5,0)
	pmullw		mm6, [edi + 8*0]	;p1(5,1)

	movq		mm2, mm0				;p1(3,0)
	movq		mm7, [ebx + 8*8]	;p1(6,0)

	punpcklwd	mm0, mm1				;p1(3,1)
	pmullw		mm7, [edi + 8*8]	;p1(6,1)

	movq		mm4, mm0				;p1(3,2)
	punpckhwd	mm2, mm1				;p1(3,4)

	pmaddwd		mm0, fix_054n184_054	;p1(3,3)
	movq		mm5, mm2				;p1(3,5)

	pmaddwd		mm2, fix_054n184_054	;p1(3,6)
	pxor		mm1, mm1	;p1(7,0)

	pmaddwd		mm4, fix_054_054p076		;p1(4,0)
	punpcklwd   mm1, mm6	;p1(7,1)

	pmaddwd		mm5, fix_054_054p076		;p1(4,1)
	psrad		mm1, 3		;p1(7,2)

	pxor		mm3, mm3	;p1(7,3)

	punpcklwd	mm3, mm7	;p1(7,4)

	psrad		mm3, 3		;p1(7,5)

	paddd		mm1, mm3	;p1(7,6)

	movq		mm3, mm1	;p1(7,7)

	paddd		mm1, mm4	;p1(7,8)

	psubd		mm3, mm4	;p1(7,9)

	movq		[esi + 8*16], mm1	;p1(7,10)
	pxor		mm4, mm4	;p1(7,12)

	movq		[esi + 8*22], mm3	;p1(7,11)
	punpckhwd	mm4, mm6	;p1(7,13)

	psrad		mm4, 3		;p1(7,14)
	pxor		mm1, mm1	;p1(7,15)

	punpckhwd	mm1, mm7	;p1(7,16)

	psrad		mm1, 3		;p1(7,17)

	paddd		mm4, mm1	;p1(7,18)

	movq		mm3, mm4	;p1(7,19)
	pxor		mm1, mm1	;p1(8,0)

	paddd		mm3, mm5	;p1(7,20)
	punpcklwd	mm1, mm6	;p1(8,1)

	psubd		mm4, mm5	;p1(7,21)
	psrad		mm1, 3		;p1(8,2)

	movq		[esi + 8*17], mm3	;p1(7,22)
	pxor		mm5, mm5	;p1(8,3)

	movq		[esi + 8*23], mm4	;p1(7,23)
	punpcklwd	mm5, mm7	;p1(8,4)

	psrad		mm5, 3		;p1(8,5)
	pxor		mm4, mm4	;p1(8,12)

	psubd		mm1, mm5	;p1(8,6)
	punpckhwd	mm4, mm6	;p1(8,13)

	movq		mm3, mm1	;p1(8,7)
	psrad		mm4, 3		;p1(8,14)

	paddd		mm1, mm0	;p1(8,8)
	pxor		mm5, mm5	;p1(8,15)

	psubd		mm3, mm0	;p1(8,9)
	movq		mm0, [ebx + 8*14]	;p1(9,0)

	punpckhwd	mm5, mm7	;p1(8,16)
	pmullw		mm0, [edi + 8*14]	;p1(9,1)

	movq		[esi + 8*18], mm1	;p1(8,10)
	psrad		mm5, 3		;p1(8,17)

	movq		[esi + 8*20], mm3	;p1(8,11)
	psubd		mm4, mm5	;p1(8,18)

	movq		mm3, mm4	;p1(8,19)
	movq		mm1, [ebx + 8*6]	;p1(10,0)

	paddd		mm3, mm2	;p1(8,20)
	pmullw		mm1, [edi + 8*6]	;p1(10,1)

	psubd		mm4, mm2	;p1(8,21)
	movq		mm5, mm0			;p1(11,1)

	movq		[esi + 8*21], mm4	;p1(8,23)

	movq		[esi + 8*19], mm3	;p1(8,22)
	movq		mm4, mm0			;p1(11,0)

	punpcklwd	mm4, mm1			;p1(11,2)
	movq		mm2, [ebx + 8*10]	;p1(12,0)

	punpckhwd	mm5, mm1			;p1(11,4)
	pmullw		mm2, [edi + 8*10]	;p1(12,1)

	movq		mm3, [ebx + 8*2]	;p1(13,0)

	pmullw		mm3, [edi + 8*2]	;p1(13,1)
	movq		mm6, mm2			;p1(14,0)

	pmaddwd		mm4, fix_117_117	;p1(11,3)
	movq		mm7, mm2			;p1(14,1)

	pmaddwd		mm5, fix_117_117	;p1(11,5)
	punpcklwd	mm6, mm3			;p1(14,2)

	pmaddwd		mm6, fix_117_117	;p1(14,3)
	punpckhwd	mm7, mm3			;p1(14,4)

	pmaddwd		mm7, fix_117_117	;p1(14,5)
	paddd		mm4, mm6			;p1(15,0)

	paddd		mm5, mm7			;p1(15,1)
   	movq		[esi+8*24], mm4		;p1(15,2)

	movq		[esi+8*25], mm5		;p1(15,3)
	movq		mm6, mm0				;p1(16,0)

	movq		mm7, mm3				;p1(16,3)
	punpcklwd	mm6, mm2				;p1(16,1)

	punpcklwd	mm7, mm3				;p1(16,4)
	pmaddwd		mm6, fix_n039_n089		;p1(16,2)

	pmaddwd		mm7, fix_150_n089n039	;p1(16,5)
	movq		mm4, mm0				;p1(16,12)

	paddd		mm6, [esi+8*24]			;p1(16,6)
	punpckhwd	mm4, mm2				;p1(16,13)

	paddd		mm6, mm7				;p1(16,7)
	pmaddwd		mm4, fix_n039_n089		;p1(16,14)

	movq		mm7, mm6				;p1(16,8)
	paddd		mm4, [esi+8*25]			;p1(16,18)

	movq		mm5, mm3				;p1(16,15)
	paddd		mm6, [esi + 8*16]		;p1(16,9)

	punpckhwd	mm5, mm3				;p1(16,16)
	paddd		mm6, const_0x2xx8		;p1(16,10)

	psrad		mm6, 9					;p1(16,11)
	pmaddwd		mm5, fix_150_n089n039	;p1(16,17)

	paddd		mm4, mm5				;p1(16,19)

	movq		mm5, mm4				;p1(16,20)

	paddd		mm4, [esi + 8*17]		;p1(16,21)

	paddd		mm4, const_0x2xx8		;p1(16,22)

	psrad		mm4, 9					;p1(16,23)

	packssdw	mm6, mm4				;p1(16,24)

	movq		[esi + 8*0], mm6		;p1(16,25)

	movq		mm4, [esi + 8*16]		;p1(16,26)

	psubd		mm4, mm7				;p1(16,27)
	movq		mm6, [esi + 8*17]		;p1(16,30)

	paddd		mm4, const_0x2xx8		;p1(16,28)
	movq		mm7, mm1				;p1(17,3)

	psrad		mm4, 9					;p1(16,29)
	psubd		mm6, mm5				;p1(16,31)

	paddd		mm6, const_0x2xx8		;p1(16,32)
	punpcklwd	mm7, mm1				;p1(17,4)

	pmaddwd		mm7, fix_307n256_n196	;p1(17,5)
	psrad		mm6, 9					;p1(16,33)

	packssdw	mm4, mm6				;p1(16,34)
	movq		[esi + 8*14], mm4		;p1(16,35)

	movq		mm6, mm0				;p1(17,0)
	movq		mm4, mm0				;p1(17,12)

	punpcklwd	mm6, mm2				;p1(17,1)
	punpckhwd	mm4, mm2				;p1(17,13)

	pmaddwd		mm6, fix_n256_n196		;p1(17,2)
	movq		mm5, mm1				;p1(17,15)

	paddd		mm6, [esi+8*24]			;p1(17,6)
	punpckhwd	mm5, mm1				;p1(17,16)

	paddd		mm6, mm7				;p1(17,7)
	pmaddwd		mm4, fix_n256_n196		;p1(17,14)

	movq		mm7, mm6				;p1(17,8)
	pmaddwd		mm5, fix_307n256_n196	;p1(17,17)

	paddd		mm6, [esi + 8*18]		;p1(17,9)

	paddd		mm6, const_0x2xx8		;p1(17,10)

	psrad		mm6, 9					;p1(17,11)
	paddd		mm4, [esi+8*25]			;p1(17,18)

	paddd		mm4, mm5				;p1(17,19)

	movq		mm5, mm4				;p1(17,20)

	paddd		mm4, [esi + 8*19]		;p1(17,21)

	paddd		mm4, const_0x2xx8		;p1(17,22)

	psrad		mm4, 9					;p1(17,23)

	packssdw	mm6, mm4				;p1(17,24)

	movq		[esi + 8*2], mm6		;p1(17,25)

	movq		mm4, [esi + 8*18]		;p1(17,26)

	movq		mm6, [esi + 8*19]		;p1(17,30)
	psubd		mm4, mm7				;p1(17,27)

	paddd		mm4, const_0x2xx8		;p1(17,28)
	psubd		mm6, mm5				;p1(17,31)

	psrad		mm4, 9					;p1(17,29)
	paddd		mm6, const_0x2xx8		;p1(17,32)

	psrad		mm6, 9					;p1(17,33)
	movq		mm7, mm2				;p1(18,3)

	packssdw	mm4, mm6				;p1(17,34)
	movq		[esi + 8*12], mm4		;p1(17,35)

	movq		mm6, mm1				;p1(18,0)
	punpcklwd	mm7, mm2				;p1(18,4)

	punpcklwd	mm6, mm3				;p1(18,1)
	pmaddwd		mm7, fix_205_n256n039	;p1(18,5)

	pmaddwd		mm6, fix_n039_n256		;p1(18,2)
	movq		mm4, mm1				;p1(18,12)

	paddd		mm6, [esi+8*24]			;p1(18,6)
	punpckhwd	mm4, mm3				;p1(18,13)

	paddd		mm6, mm7				;p1(18,7)
	pmaddwd		mm4, fix_n039_n256		;p1(18,14)

	movq		mm7, mm6				;p1(18,8)
	movq		mm5, mm2				;p1(18,15)

	paddd		mm6, [esi + 8*20]		;p1(18,9)
	punpckhwd	mm5, mm2				;p1(18,16)

	paddd		mm6, const_0x2xx8		;p1(18,10)

	psrad		mm6, 9					;p1(18,11)
	pmaddwd		mm5, fix_205_n256n039	;p1(18,17)

	paddd		mm4, [esi+8*25]			;p1(18,18)

	paddd		mm4, mm5				;p1(18,19)

	movq		mm5, mm4				;p1(18,20)

	paddd		mm4, [esi + 8*21]		;p1(18,21)

	paddd		mm4, const_0x2xx8		;p1(18,22)

	psrad		mm4, 9					;p1(18,23)

	packssdw	mm6, mm4				;p1(18,24)

	movq		[esi + 8*4], mm6		;p1(18,25)

	movq		mm4, [esi + 8*20]		;p1(18,26)

	psubd		mm4, mm7				;p1(18,27)

	paddd		mm4, const_0x2xx8		;p1(18,28)
	movq		mm7, mm0				;p1(19,3)

	psrad		mm4, 9					;p1(18,29)
	movq		mm6, [esi + 8*21]		;p1(18,30)

	psubd		mm6, mm5				;p1(18,31)
	punpcklwd	mm7, mm0				;p1(19,4)

	paddd		mm6, const_0x2xx8		;p1(18,32)

	psrad		mm6, 9					;p1(18,33)
	pmaddwd		mm7, fix_029_n089n196	;p1(19,5)

	packssdw	mm4, mm6				;p1(18,34)

	movq		[esi + 8*10], mm4		;p1(18,35)
	movq		mm6, mm3				;p1(19,0)

	punpcklwd	mm6, mm1				;p1(19,1)
	movq		mm5, mm0				;p1(19,15)

	pmaddwd		mm6, fix_n196_n089		;p1(19,2)
	punpckhwd	mm5, mm0				;p1(19,16)


	paddd		mm6, [esi+8*24]			;p1(19,6)
	movq		mm4, mm3				;p1(19,12)

	paddd		mm6, mm7				;p1(19,7)
	punpckhwd	mm4, mm1				;p1(19,13)


	movq		mm7, mm6				;p1(19,8)
	pmaddwd		mm4, fix_n196_n089  	;p1(19,14)

	paddd		mm6, [esi + 8*22]		;p1(19,9)

	pmaddwd		mm5, fix_029_n089n196	;p1(19,17)

	paddd		mm6, const_0x2xx8		;p1(19,10)

	psrad		mm6, 9					;p1(19,11)
	paddd		mm4, [esi+8*25]			;p1(19,18)

	paddd		mm4, mm5				;p1(19,19)

	movq		mm5, mm4				;p1(19,20)
	paddd		mm4, [esi + 8*23]		;p1(19,21)
	paddd		mm4, const_0x2xx8		;p1(19,22)
	psrad		mm4, 9					;p1(19,23)

	packssdw	mm6, mm4				;p1(19,24)
	movq		[esi + 8*6], mm6		;p1(19,25)

	movq		mm4, [esi + 8*22]		;p1(19,26)

	psubd		mm4, mm7				;p1(19,27)
	movq		mm6, [esi + 8*23]		;p1(19,30)

	paddd		mm4, const_0x2xx8		;p1(19,28)
	psubd		mm6, mm5				;p1(19,31)

	psrad		mm4, 9					;p1(19,29)
	paddd		mm6, const_0x2xx8		;p1(19,32)

	psrad		mm6, 9					;p1(19,33)

	packssdw	mm4, mm6				;p1(19,34)
	movq		[esi + 8*8], mm4		;p1(19,35)

//************************************************************
	mov			esi, eax

	mov			edi, outptr


//transpose 4 rows of wsptr

	movq		mm0, [esi+8*0]		;tran(0)

	movq		mm1, mm0			;tran(1)
	movq		mm2, [esi+8*2]		;tran(2)

	punpcklwd	mm0, mm2			;tran(3)
	movq		mm3, [esi+8*4]		;tran(5)

	punpckhwd	mm1, mm2			;tran(4)
	movq		mm5, [esi+8*6]		;tran(7)

	movq		mm4, mm3			;tran(6)
	movq		mm6, mm0			;tran(10)

	punpcklwd	mm3, mm5			;tran(8)
	movq		mm7, mm1			;tran(11)

	punpckldq	mm0, mm3			;tran(12)

	punpckhwd	mm4, mm5			;tran(9)
	movq		[esi+8*0], mm0		;tran(16)

	punpckhdq	mm6, mm3			;tran(13)
	movq		mm0, [esi+8*1]		;tran(20)

	punpckldq	mm1, mm4			;tran(14)
	movq		[esi+8*2], mm6		;tran(17)

	punpckhdq	mm7, mm4			;tran(15)
	movq		[esi+8*4], mm1		;tran(18)

	movq		mm1, mm0			;tran(21)
	movq		mm3, [esi+8*5]		;tran(25)

	movq		mm2, [esi+8*3]		;tran(22)
	movq		mm4, mm3			;tran(26)

	punpcklwd	mm0, mm2			;tran(23)
	movq		[esi+8*6], mm7		;tran(19)

	punpckhwd	mm1, mm2			;tran(24)
	movq		mm5, [esi+8*7]		;tran(27)

	punpcklwd	mm3, mm5			;tran(28)
	movq		mm6, mm0			;tran(30)

	movq		mm7, mm1			;tran(31)
	punpckhdq	mm6, mm3			;tran(33)

	punpckhwd	mm4, mm5			;tran(29)
	movq		mm2, mm6			;p2(1,0)

	punpckhdq	mm7, mm4			;tran(35)
	movq		mm5, [esi + 8*2]	;p2(1,2)

	paddw		mm2, mm7			;p2(1,1)
	paddw		mm5, [esi + 8*6]	;p2(1,3)

	punpckldq	mm0, mm3			;tran(32)
	paddw		mm2, mm5			;p2(1,4)

	punpckldq	mm1, mm4			;tran(34)
	movq		mm5, [esi + 8*2]		;p2(3,0)

	pmulhw		mm2, fix_117_117	;p2(1,5)
	movq		mm4, mm7				;p2(2,0)

	pmulhw		mm4, fixn089n196p029	;p2(2,1)
	movq		mm3, mm6					;p2(6,0)

	pmulhw		mm3, fix_n256n039p205		;p2(6,1)

	pmulhw		mm5, fix_n089			;p2(3,1)

	movq		[eax + 8*24], mm2	;p2(1,6)

	movq		mm2, [esi + 8*6]	;p2(4,0)

	pmulhw		mm2, fix_n196		;p2(4,1)

	paddw		mm4, [eax + 8*24]			;p2(5,0)

	paddw		mm3, [eax + 8*24]		;p2(9,0)

	paddw		mm5, mm2					;p2(5,1)

	movq		mm2, [esi + 8*2]		;p2(7,0)
	paddw		mm5, mm4					;p2(5,2)
	pmulhw		mm2, fix_n039			;p2(7,1)

	movq		[esi + 8*1], mm5			;p2(5,3)

	movq		mm4, [esi + 8*6]	;p2(8,0)
	movq		mm5, mm6			;p2(10,0)

	pmulhw		mm4, fix_n256		;p2(8,1)

	pmulhw		mm5, fix_n039		;p2(10,1)

	pmulhw		mm6, fix_n256			;p2(15,0)

	paddw		mm2, mm4				;p2(9,1)

	movq		mm4, mm7			;p2(11,0)

	pmulhw		mm4, fix_n089		;p2(11,1)
	paddw		mm2, mm3				;p2(9,2)

	movq		[esi + 8*3], mm2		;p2(9,3)

	movq		mm3, [esi + 8*2]		;p2(13,0)

	pmulhw		mm7, fix_n196			;p2(16,0)

	pmulhw		mm3, fix_n089n039p150	;p2(13,1)
	paddw		mm5, mm4			;p2(12,0)


	paddw		mm5, [eax + 8*24]		;p2(14,0)

	movq		mm2, [esi + 8*6]		;p2(18,0)

	pmulhw		mm2, fix_n196p307n256	;p2(18,1)
	paddw		mm5, mm3				;p2(14,1)

	movq		[esi + 8*5], mm5		;p2(14,2)
	paddw		mm6, mm7				;p2(17,0)

	paddw		mm6, [eax + 8*24]		;p2(19,0)
	movq		mm3, mm1				;p2(21,0)

	movq		mm4, [esi + 8*4]		;p2(20,0)
	paddw		mm6, mm2				;p2(19,1)

	movq		[esi + 8*7], mm6		;p2(19,2)
	movq		mm5, mm4				;p2(20,1)

	movq		mm7, [esi + 8*0]		;p2(26,0)

	pmulhw		mm4, fix_054p076		;p2(20,2)
	psubw		mm7, mm0				;p2(27,0)

	pmulhw		mm3, fix_054			;p2(21,1)
	movq		mm2, mm0				;p2(26,1)

	pmulhw		mm5, fix_054			;p2(23,0)
	psraw		mm7, 3					;p2(27,1)

	paddw		mm2, [esi + 8*0]		;p2(26,2)
	movq		mm6, mm7				;p2(28,0)

	pmulhw		mm1, fix_054n184		;p2(24,0)
	psraw		mm2, 3					;p2(26,3)

	paddw		mm4, mm3				;p2(22,0)
	paddw		mm5, mm1				;p2(25,0)

	psubw		mm6, mm5				;p2(29,0)
	movq		mm3, mm2				;p2(30,0)

	paddw		mm2, mm4				;p2(30,1)
	paddw		mm7, mm5				;p2(28,1)

	movq		mm1, mm2				;p2(32,0)
	psubw		mm3, mm4				;p2(31,0)

	paddw		mm1, [esi + 8*5]		;p2(32,1)
	movq		mm0, mm7				;p2(33,0)

	psubw		mm2, [esi + 8*5]		;p2(32,2)
	movq		mm4, mm6				;p2(34,0)

	paddw		mm1, const_0x0808		;p2(32,3)

	paddw		mm2, const_0x0808		;p2(32,4)
	psraw		mm1, 4					;p2(32,5)

	psraw		mm2, 4					;p2(32,6)
	paddw		mm7, [esi + 8*7]		;p2(33,1)

	packuswb	mm1, mm2				;p2(32,7)
	psubw		mm0, [esi + 8*7]		;p2(33,2)

	paddw		mm7, const_0x0808		;p2(33,3)

	paddw		mm0, const_0x0808		;p2(33,4)
	psraw		mm7, 4					;p2(33,5)

	psraw		mm0, 4					;p2(33,6)
	paddw		mm4, [esi + 8*3]		;p2(34,1)

	packuswb	mm7, mm0				;p2(33,7)
	psubw		mm6, [esi + 8*3]		;p2(34,2)

	paddw		mm4, const_0x0808		;p2(34,3)
	movq		mm5, mm3				;p2(35,0)

	paddw		mm6, const_0x0808		;p2(34,4)
	psraw		mm4, 4					;p2(34,5)

	psraw		mm6, 4					;p2(34,6)
	paddw		mm3, [esi + 8*1]		;p2(35,1)

	packuswb	mm4, mm6				;p2(34,7)
	psubw		mm5, [esi + 8*1]		;p2(35,2)

	movq		mm0, mm1				;p2(36,0)
	paddw		mm3, const_0x0808		;p2(35,3)

	paddw		mm5, const_0x0808		;p2(35,4)
	punpcklbw	mm0, mm7				;p2(36,1)

	psraw		mm3, 4					;p2(35,5)
	movq		mm2, mm4				;p2(37,0)

	psraw		mm5, 4					;p2(35,6)
	movq		mm6, mm0				;p2(38,0)

	packuswb	mm3, mm5				;p2(35,7)
	mov			ebx, edi				;p2(42,0)

	punpckhbw	mm7, mm1				;p2(36,2)
	add                     edi, stride
	mov			ecx, edi			;p2(42,1)

	punpcklbw	mm2, mm3				;p2(37,1)
	add                     edi, stride
	mov			edx, edi			;p2(42,2)

	punpckhbw	mm3, mm4				;p2(37,2)

	punpcklwd	mm0, mm2				;p2(38,1)
	movq		mm5, mm3				;p2(39,0)

	punpckhwd	mm6, mm2				;p2(38,2)
	movq		mm1, mm0				;p2(40,0)

	punpcklwd	mm3, mm7				;p2(39,1)

	punpckldq	mm0, mm3				;p2(40,1)

	punpckhdq	mm1, mm3				;p2(40,2)
	movq		[ebx], mm0				;p2(43,0)

	punpckhwd	mm5, mm7				;p2(39,2)
	movq		[ecx], mm1				;p2(43,1)

	movq		mm4, mm6				;p2(41,0)
	add                     edi, stride
	mov			ebx, edi			;p2(43,3)

	punpckldq	mm4, mm5				;p2(41,1)

	punpckhdq	mm6, mm5				;p2(41,2)
	movq		[edx], mm4				;p2(43,2)

	movq		[ebx], mm6				;p2(43,5)

//************************************************************
//	Process next 4 rows

	add			esi, 64
	add                     edi, stride

//transpose next 4 rows of wsptr

	movq		mm0, [esi+8*0]		;tran(0)

	movq		mm1, mm0			;tran(1)
	movq		mm2, [esi+8*2]		;tran(2)

	punpcklwd	mm0, mm2			;tran(3)
	movq		mm3, [esi+8*4]		;tran(5)

	punpckhwd	mm1, mm2			;tran(4)
	movq		mm5, [esi+8*6]		;tran(7)

	movq		mm4, mm3			;tran(6)
	movq		mm6, mm0			;tran(10)

	punpcklwd	mm3, mm5			;tran(8)
	movq		mm7, mm1			;tran(11)

	punpckldq	mm0, mm3			;tran(12)

	punpckhwd	mm4, mm5			;tran(9)
	movq		[esi+8*0], mm0		;tran(16)

	punpckhdq	mm6, mm3			;tran(13)
	movq		mm0, [esi+8*1]		;tran(20)

	punpckldq	mm1, mm4			;tran(14)
	movq		[esi+8*2], mm6		;tran(17)

	punpckhdq	mm7, mm4			;tran(15)
	movq		[esi+8*4], mm1		;tran(18)

	movq		mm1, mm0			;tran(21)
	movq		mm3, [esi+8*5]		;tran(25)

	movq		mm2, [esi+8*3]		;tran(22)
	movq		mm4, mm3			;tran(26)

	punpcklwd	mm0, mm2			;tran(23)
	movq		[esi+8*6], mm7		;tran(19)

	punpckhwd	mm1, mm2			;tran(24)
	movq		mm5, [esi+8*7]		;tran(27)

	punpcklwd	mm3, mm5			;tran(28)
	movq		mm6, mm0			;tran(30)

	movq		mm7, mm1			;tran(31)
	punpckhdq	mm6, mm3			;tran(33)

	punpckhwd	mm4, mm5			;tran(29)
	movq		mm2, mm6			;p2(1,0)

	punpckhdq	mm7, mm4			;tran(35)
	movq		mm5, [esi + 8*2]	;p2(1,2)

	paddw		mm2, mm7			;p2(1,1)
	paddw		mm5, [esi + 8*6]	;p2(1,3)

	punpckldq	mm0, mm3			;tran(32)
	paddw		mm2, mm5			;p2(1,4)

	punpckldq	mm1, mm4			;tran(34)
	movq		mm5, [esi + 8*2]		;p2(3,0)

	pmulhw		mm2, fix_117_117	;p2(1,5)
	movq		mm4, mm7				;p2(2,0)

	pmulhw		mm4, fixn089n196p029	;p2(2,1)
	movq		mm3, mm6					;p2(6,0)

	pmulhw		mm3, fix_n256n039p205		;p2(6,1)

	pmulhw		mm5, fix_n089			;p2(3,1)

	movq		[eax + 8*24], mm2	;p2(1,6)

	movq		mm2, [esi + 8*6]	;p2(4,0)

	pmulhw		mm2, fix_n196		;p2(4,1)

	paddw		mm4, [eax + 8*24]			;p2(5,0)

	paddw		mm3, [eax + 8*24]		;p2(9,0)

	paddw		mm5, mm2					;p2(5,1)

	movq		mm2, [esi + 8*2]		;p2(7,0)
	paddw		mm5, mm4					;p2(5,2)
	pmulhw		mm2, fix_n039			;p2(7,1)

	movq		[esi + 8*1], mm5			;p2(5,3)

	movq		mm4, [esi + 8*6]	;p2(8,0)
	movq		mm5, mm6			;p2(10,0)

	pmulhw		mm4, fix_n256		;p2(8,1)

	pmulhw		mm5, fix_n039		;p2(10,1)

	pmulhw		mm6, fix_n256			;p2(15,0)

	paddw		mm2, mm4				;p2(9,1)

	movq		mm4, mm7			;p2(11,0)

	pmulhw		mm4, fix_n089		;p2(11,1)
	paddw		mm2, mm3				;p2(9,2)

	movq		[esi + 8*3], mm2		;p2(9,3)

	movq		mm3, [esi + 8*2]		;p2(13,0)

	pmulhw		mm7, fix_n196			;p2(16,0)

	pmulhw		mm3, fix_n089n039p150	;p2(13,1)
	paddw		mm5, mm4			;p2(12,0)


	paddw		mm5, [eax + 8*24]		;p2(14,0)

	movq		mm2, [esi + 8*6]		;p2(18,0)

	pmulhw		mm2, fix_n196p307n256	;p2(18,1)
	paddw		mm5, mm3				;p2(14,1)

	movq		[esi + 8*5], mm5		;p2(14,2)
	paddw		mm6, mm7				;p2(17,0)

	paddw		mm6, [eax + 8*24]		;p2(19,0)
	movq		mm3, mm1				;p2(21,0)

	movq		mm4, [esi + 8*4]		;p2(20,0)
	paddw		mm6, mm2				;p2(19,1)

	movq		[esi + 8*7], mm6		;p2(19,2)
	movq		mm5, mm4				;p2(20,1)

	movq		mm7, [esi + 8*0]		;p2(26,0)

	pmulhw		mm4, fix_054p076		;p2(20,2)
	psubw		mm7, mm0				;p2(27,0)

	pmulhw		mm3, fix_054			;p2(21,1)
	movq		mm2, mm0				;p2(26,1)

	pmulhw		mm5, fix_054			;p2(23,0)
	psraw		mm7, 3					;p2(27,1)

	paddw		mm2, [esi + 8*0]		;p2(26,2)
	movq		mm6, mm7				;p2(28,0)

	pmulhw		mm1, fix_054n184		;p2(24,0)
	psraw		mm2, 3					;p2(26,3)

	paddw		mm4, mm3				;p2(22,0)
	paddw		mm5, mm1				;p2(25,0)

	psubw		mm6, mm5				;p2(29,0)
	movq		mm3, mm2				;p2(30,0)

	paddw		mm2, mm4				;p2(30,1)
	paddw		mm7, mm5				;p2(28,1)

	movq		mm1, mm2				;p2(32,0)
	psubw		mm3, mm4				;p2(31,0)

	paddw		mm1, [esi + 8*5]		;p2(32,1)
	movq		mm0, mm7				;p2(33,0)

	psubw		mm2, [esi + 8*5]		;p2(32,2)
	movq		mm4, mm6				;p2(34,0)

	paddw		mm1, const_0x0808		;p2(32,3)

	paddw		mm2, const_0x0808		;p2(32,4)
	psraw		mm1, 4					;p2(32,5)

	psraw		mm2, 4					;p2(32,6)
	paddw		mm7, [esi + 8*7]		;p2(33,1)

	packuswb	mm1, mm2				;p2(32,7)
	psubw		mm0, [esi + 8*7]		;p2(33,2)

	paddw		mm7, const_0x0808		;p2(33,3)

	paddw		mm0, const_0x0808		;p2(33,4)
	psraw		mm7, 4					;p2(33,5)

	psraw		mm0, 4					;p2(33,6)
	paddw		mm4, [esi + 8*3]		;p2(34,1)

	packuswb	mm7, mm0				;p2(33,7)
	psubw		mm6, [esi + 8*3]		;p2(34,2)

	paddw		mm4, const_0x0808		;p2(34,3)
	movq		mm5, mm3				;p2(35,0)

	paddw		mm6, const_0x0808		;p2(34,4)
	psraw		mm4, 4					;p2(34,5)

	psraw		mm6, 4					;p2(34,6)
	paddw		mm3, [esi + 8*1]		;p2(35,1)

	packuswb	mm4, mm6				;p2(34,7)
	psubw		mm5, [esi + 8*1]		;p2(35,2)

	movq		mm0, mm1				;p2(36,0)
	paddw		mm3, const_0x0808		;p2(35,3)

	paddw		mm5, const_0x0808		;p2(35,4)
	punpcklbw	mm0, mm7				;p2(36,1)

	psraw		mm3, 4					;p2(35,5)
	movq		mm2, mm4				;p2(37,0)

	psraw		mm5, 4					;p2(35,6)
	movq		mm6, mm0				;p2(38,0)

	packuswb	mm3, mm5				;p2(35,7)
	mov			ebx, edi				;p2(42,0)

	punpckhbw	mm7, mm1				;p2(36,2)
	add                     edi, stride
	mov			ecx, edi			;p2(42,1)

	punpcklbw	mm2, mm3				;p2(37,1)
	add                     edi, stride
	mov			edx, edi			;p2(42,2)

	punpckhbw	mm3, mm4				;p2(37,2)

	punpcklwd	mm0, mm2				;p2(38,1)
	movq		mm5, mm3				;p2(39,0)

	punpckhwd	mm6, mm2				;p2(38,2)
	movq		mm1, mm0				;p2(40,0)

	punpcklwd	mm3, mm7				;p2(39,1)

	punpckldq	mm0, mm3				;p2(40,1)

	punpckhdq	mm1, mm3				;p2(40,2)
	movq		[ebx], mm0				;p2(43,0)

	punpckhwd	mm5, mm7				;p2(39,2)
	movq		[ecx], mm1				;p2(43,1)

	movq		mm4, mm6				;p2(41,0)
	add                     edi, stride
	mov			ebx, edi			;p2(43,3)

	punpckldq	mm4, mm5				;p2(41,1)

	punpckhdq	mm6, mm5				;p2(41,2)
	movq		[edx], mm4				;p2(43,2)

	movq		[ebx], mm6				;p2(43,5)

//************************************************************************

;	emms

	}
}
#endif
#endif
#endif
