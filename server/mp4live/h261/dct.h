/*
 * dct.h --
 *
 *      Header file for the DCT code
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
 *
 * @(#) $Header: /cvsroot/mpeg4ip/mpeg4ip/server/mp4live/h261/dct.h,v 1.1 2003/04/09 00:44:42 wmaycisco Exp $
 */

/*
 * Ck = cos(k pi / 16)
 * Sk = sin(k pi / 16)
 */
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

/* the following line enables MMX code.  if your cpu supports MMX, change this to 1 */
#define MMX_DCT_ENABLED 0

void dct_fdct(const u_char* in, int stride, short* out, const float* qt);

#ifdef INT_64
void rdct(short* coef, INT_64 mask, u_char* out, int stride, const int* qt);
void rdct(short* coef, INT_64 mask, u_char* out, int stride, const u_char* in);
#else
void rdct(short* coef, u_int m0, u_int m1, u_char* out,
	  int stride, const int* qt);
void rdct(short* coef, u_int m0, u_int m1, u_char* out,
	  int stride, const u_char* in);
#endif

void dcfill(int dc, u_char* out, int stride);
void dcsum(int dc, u_char* in, u_char* out, int stride);
void dcsum2(int dc, u_char* in, u_char* out, int stride);
void dct_decimate(const short* in0, const short* in1, short* out);

/*FIXME*/
void rdct_fold_q(const int* in, int* qt);
void dct_fdct_fold_q(const int* in, float* qt);

// wmay extern const u_char ROWZAG[];
extern const u_char DCT_COLZAG[];

/*FIXME*/
extern void j_fwd_dct (short* data);

