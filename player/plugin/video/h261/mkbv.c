/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Network Research
 *	Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef sgi
#include <bstring.h>
#endif
#include "bsd-endian.h"

#define CONST_BITS 13
#define NFRACBITS CONST_BITS
#define CONST_SCALE (1 << CONST_BITS)
#define PASS1_BITS 1
#define DESCALE(x,n)  (((x) + (1 << ((n)-1))) >> (n))
#define FIX(x)	((int) ((x) * CONST_SCALE + 0.5))

#define C1 0.98078528
#define C2 0.92387953
#define C3 0.83146961
#define C4 0.70710678
#define C5 0.55557023
#define C6 0.38268343
#define C7 0.19509032
#define S1 C7
#define S3 C5
#define S6 C2
#define C_1 C1
#define S_1 (-S1)
#define C_3 C3
#define S_3 (-S3)

/*
 * A 2D Inverse DCT based on a column-row decomposition using
 * Vetterli's 8pt 1D Inverse DCT, from Fig. 4-7 Pennebaker & Mitchell
 * (i.e., the pink JPEG book).  This figure is the forward transform;
 * reverse the flowgraph for the inverse (you need to draw a picture).
 * To reverse the rotations, you must swap the pair of inputs and swap
 * the pair of outputs, in addition to reversing the flowgraph edges.
 *
 * The input coefficients are, counter to tradition, in column-order.
 * The bit mask indicates which coefficients are non-zero.  If the
 * corresponding bit is zero, then the coefficient is assumed zero
 * and the input coefficient is not referenced and need not be defined.
 * The 8-bit outputs are computed in row order and placed in the
 * output array pointed to by p, with each of the eight 8-byte lines
 * offset by "stride" bytes.
 *
 * outputs scaled by 4
 */
static void
v_rdct(short *bp)
{
	int i;
	for (i = 8; --i >= 0; ) {
		int x2, x3;
		int x0 = FIX(C4) * bp[8*0];
		int x1 = FIX(C4) * bp[8*4];
		int t3 = FIX(C6) * bp[8*6] + FIX(S6) * bp[8*2];
		int t2 = -FIX(S6) * bp[8*6] + FIX(C6) * bp[8*2];
		int x4 = -(FIX(C_1) * bp[8*7] + FIX(S_1) * bp[8*1]);
		int x5 = -FIX(S_1) * bp[8*7] + FIX(C_1) * bp[8*1];
		int x6 = FIX(C_3) * bp[8*5] + FIX(S_3) * bp[8*3];
		int t7 = -FIX(S_3) * bp[8*5] + FIX(C_3) * bp[8*3];

		int t0 = x0 + x1;
		int t1 = x0 - x1;
		int t4 = x4 + x6;
		int t5 = FIX(C4) * DESCALE(x5 - t7, CONST_BITS);
		int t6 = FIX(C4) * DESCALE(x4 - x6, CONST_BITS);
		t7 += x5;

		x0 = t0 + t3;
		x3 = t0 - t3;
		x1 = t1 + t5;
		x5 = t1 - t5;
		x2 = t6 + t2;
		x6 = t6 - t2;

		bp[8*0] = DESCALE(x0 + t7, CONST_BITS-PASS1_BITS);
		bp[8*7] = DESCALE(x0 - t7, CONST_BITS-PASS1_BITS);
		bp[8*1] = DESCALE(x1 + x2, CONST_BITS-PASS1_BITS);
		bp[8*2] = DESCALE(x1 - x2, CONST_BITS-PASS1_BITS);
		bp[8*3] = DESCALE(x3 + t4, CONST_BITS-PASS1_BITS);
		bp[8*4] = DESCALE(x3 - t4, CONST_BITS-PASS1_BITS);
		bp[8*5] = DESCALE(x5 + x6, CONST_BITS-PASS1_BITS);
		bp[8*6] = DESCALE(x5 - x6, CONST_BITS-PASS1_BITS);

		++bp;
	}
	bp -= 8;
	for (i = 8; --i >= 0; ) {
		int x2, x3;
		int x0 = FIX(C4) * bp[0];
		int x1 = FIX(C4) * bp[4];
		int t3 = FIX(C6) * bp[6] + FIX(S6) * bp[2];
		int t2 = -FIX(S6) * bp[6] + FIX(C6) * bp[2];
		int x4 = -(FIX(C_1) * bp[7] + FIX(S_1) * bp[1]);
		int x5 = -FIX(S_1) * bp[7] + FIX(C_1) * bp[1];
		int x6 = FIX(C_3) * bp[5] + FIX(S_3) * bp[3];
		int t7 = -FIX(S_3) * bp[5] + FIX(C_3) * bp[3];

		int t0 = x0 + x1;
		int t1 = x0 - x1;
		int t4 = x4 + x6;
		int t5 = FIX(C4) * DESCALE(x5 - t7, CONST_BITS);
		int t6 = FIX(C4) * DESCALE(x4 - x6, CONST_BITS);
		t7 += x5;

		x0 = t0 + t3;
		x3 = t0 - t3;
		x1 = t1 + t5;
		x5 = t1 - t5;
		x2 = t6 + t2;
		x6 = t6 - t2;

		bp[0] = DESCALE(x0 + t7, CONST_BITS+PASS1_BITS+2);
		bp[7] = DESCALE(x0 - t7, CONST_BITS+PASS1_BITS+2);
		bp[1] = DESCALE(x1 + x2, CONST_BITS+PASS1_BITS+2);
		bp[2] = DESCALE(x1 - x2, CONST_BITS+PASS1_BITS+2);
		bp[3] = DESCALE(x3 + t4, CONST_BITS+PASS1_BITS+2);
		bp[4] = DESCALE(x3 - t4, CONST_BITS+PASS1_BITS+2);
		bp[5] = DESCALE(x5 + x6, CONST_BITS+PASS1_BITS+2);
		bp[6] = DESCALE(x5 - x6, CONST_BITS+PASS1_BITS+2);

		bp += 8;
	}
}

/* column order */
static int COLZAG[] = {
	0, 8, 1, 2, 9, 16, 24, 17,
	10, 3, 4, 11, 18, 25, 32, 40,
	33, 26, 19, 12, 5, 6, 13, 20,
	27, 34, 41, 48, 56, 49, 42, 35,
	28, 21, 14, 7, 15, 22, 29, 36,
	43, 50, 57, 58, 51, 44, 37, 30,
	23, 31, 38, 45, 52, 59, 60, 53,
	46, 39, 47, 54, 61, 62, 55, 63,
};
static int ROWZAG[] = {
	0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63,
};

/*
 * True if v is in a[0..n-1]
 */
static int
inlist(int* a, int n, int v)
{
	while (--n >= 0)
		if (a[n] == v)
			return (1);
	return (0);
}

static int
find(int* a, int n, int v)
{
	while (--n >= 0)
		if (a[n] == v)
			return (n);
	abort();
}

int nmul;
int multab[100];

void
compute_multab()
{
	int k, m;
	short blk[64];

	for (k = 1; k < 64; ++k) {
		memset((char*)blk, 0, sizeof(blk));
		blk[ROWZAG[k]] = FIX(1);
		v_rdct(blk);
		for (m = 0; m < 64; ++m) {
			if (inlist(multab, nmul, blk[m]))
				continue;
			multab[nmul++] = blk[m];
		}
	}
}

void
gen_basis()
{
	int k, m;
	short blk[64];

	printf("u_int dct_basis[64][64/sizeof(u_int)] = {\n");
	for (k = 0; k < 64; ++k) {
		printf("{");
		memset((char*)blk, 0, sizeof(blk));
		blk[COLZAG[k]] = FIX(1);
		v_rdct(blk);
		for (m = 0; m < 64; ) {
			printf("%d,%d,\n",
			       find(multab, nmul, blk[m+0]) << 24 |
			       find(multab, nmul, blk[m+1]) << 16 |
			       find(multab, nmul, blk[m+2]) << 8 |
			       find(multab, nmul, blk[m+3]),
			       find(multab, nmul, blk[m+4]) << 24 |
			       find(multab, nmul, blk[m+5]) << 16 |
			       find(multab, nmul, blk[m+6]) << 8 |
			       find(multab, nmul, blk[m+7]));
			m += 8;
		}
		printf("},\n");
	}
	printf("};\n\n");
}

void
gen_multab()
{
	int k, m;

	printf("u_char multab[] = {");
	for (m = 0; m < 256; ++m) {
		for (k = 0; k < nmul; ++k) {
			/*
			 * input multiplier is scaled down
			 * by 4 in runtime code
			 */
			int s = m << 24 >> 24;
			s <<= 2;
			s *= multab[k];
			s >>= NFRACBITS;

			if (s < -128)
				s = -128;
			else if (s > 127)
				s = 127;

			if ((k & 15) == 0)
				printf("\n\t");
			printf("%d,", s & 0xff);
		}
		/*
		 * Pad out rest of table with 0's so we get
		 * dimensions right (there are 66 unique
		 * coefficients -- 33 if you consider only
		 * magnitude).
		 */
		for (; k < 128; ++k) {
			if ((k & 15) == 0)
				printf("\n\t");
			printf("0,");
		}
	}
	printf("\n};\n\n");
}

int
main(int argc, char **argv)
{
	printf("#include <sys/types.h>\n");
#ifdef WIN32
	printf("#include <winsock.h>\n");
#endif
	compute_multab();
	gen_basis();
	gen_multab();
	return (0);
}
