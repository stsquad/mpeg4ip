/*
 * Copyright (c) 1993 Regents of the University of California.
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
 *
 * This code is derived from (but bears little resemblance to)
 * the P64 software implementation by the Stanford PVRG group.
 * Their copyright applies herein:
 * 
 * Copyright (C) 1990, 1991, 1993 Andy C. Hung, all rights reserved.
 * PUBLIC DOMAIN LICENSE: Stanford University Portable Video Research
 * Group. If you use this software, you agree to the following: This
 * program package is purely experimental, and is licensed "as is".
 * Permission is granted to use, modify, and distribute this program
 * without charge for any purpose, provided this license/ disclaimer
 * notice appears in the copies.  No warranty or maintenance is given,
 * either expressed or implied.  In no event shall the author(s) be
 * liable to you or a third party for any special, incidental,
 * consequential, or other damages, arising out of the use or inability
 * to use the program for any purpose (or the loss of data), even if we
 * have been advised of such possibilities.  Any public reference or
 * advertisement of this source code should refer to it as the Portable
 * Video Research Group (PVRG) code, and not by any author(s) (or
 * Stanford University) name.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <sys/param.h>
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include "bsd-endian.h"
#include "p64.h"
#include "p64dump.h"
#include "p64-huff.h"
#include "dct.h"


P64Dumper::P64Dumper(int q)
{
	dump_quantized_ = q;
}

P64Dumper::P64Dumper()
{
	dump_quantized_ = 0;
}

#if BYTE_ORDER == LITTLE_ENDIAN
#define HUFFRQ(bs, bb) \
 { \
	register int t = *bs++; \
	bb <<= 16; \
	bb |= (t & 0xff) << 8; \
	bb |= t >> 8; \
}
#else
#define HUFFRQ(bs, bb) \
 { \
	bb <<= 16; \
	bb |= *bs++; \
}
#endif

#define P64MASK(s) ((1 << (s)) - 1)

#define HUFF_DECODE(bs, ht, nbb, bb, result) { \
	register int s__, v__; \
 \
	if (nbb < 16) { \
		HUFFRQ(bs, bb); \
		nbb += 16; \
	} \
	s__ = ht.maxlen; \
	v__ = (bb >> (nbb - s__)) & P64MASK(s__); \
	s__ = (ht.prefix)[v__]; \
	nbb -= (s__ & 0x1f); \
	result = s__ >> 5; \
 }

#define GET_BITS(bs, n, nbb, bb, result) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bs, bb); \
		nbb += 16; \
	} \
	(result) = ((bb >> nbb) & P64MASK(n)); \
}

#define SKIP_BITS(bs, n, nbb, bb) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bs, bb); \
		nbb += 16; \
	} \
}

#define DUMPBITS(c) dump_bits(c)

void P64Dumper::dump_bits(char c)
{
	int nbits = (bs_ - dbs_) * 16 + dnbb_ - nbb_;
	int v;
	printf("%d/", nbits);
	while (nbits > 16) {
		GET_BITS(dbs_, 16, dnbb_, dbb_, v);
		printf("%04x", v);
		nbits -= 16;
	}
	if (nbits > 0) {
		GET_BITS(dbs_, nbits, dnbb_, dbb_, v);
		if (nbits <= 4)
			printf("%01x%c", v, c);
		else if (nbits <= 8)
			printf("%02x%c", v, c);
		else if (nbits <= 12)
			printf("%03x%c", v, c);
		else
			printf("%04x%c", v, c);
	}
}

void P64Dumper::err(const char* msg ...) const
{
	va_list ap;
	va_start(ap, msg);
	printf("-err: ");
	vfprintf(stdout, msg, ap);
	printf(" @g%d m%d %ld/%d of %ld/%d: %04x %04x %04x %04x|%04x\n",
		gob_, mba_,
	       (long)((u_char*)bs_ - (u_char*)ps_), nbb_,
	       (long)((u_char*)es_ - (u_char*)ps_), pebit_,
	       bs_[-4], bs_[-3], bs_[-2], bs_[-1], bs_[0]);
}


/*
 * Decode the next block of transform coefficients
 * from the input stream.
 */
#ifdef INT_64
int P64Dumper::parse_block(short* blk, INT_64* mask)
#else
int P64Dumper::parse_block(short* blk, u_int* mask)
#endif
{
#ifdef INT_64
	INT_64 m0 = 0;
#else
	u_int m1 = 0, m0 = 0;
#endif
	/*
	 * Cache bit buffer in registers.
	 */
	register int nbb = nbb_;
	register int bb = bb_;
	register short* qt = qt_;
	register int val = 0, k;

	if ((mt_ & MT_CBP) == 0) {
		int v;
		GET_BITS(bs_, 8, nbb, bb, v);
		val = v;
		if (v == 255)
			v = 128;
		if (mt_ & MT_INTRA)
			v <<= 3;
		else
			v = qt[v];
		blk[0] = v;
		k = 1;
		m0 |= 1;
	} else if ((bb >> (nbb - 1)) & 1) {
		/*
		 * In CBP blocks, the first block present must be
		 * non-empty (otherwise it's mask bit wouldn't
		 * be set), so the first code cannot be an EOB.
		 * CCITT optimizes this case by using a huffman
		 * table equivalent to ht_tcoeff_ but without EOB,
		 * in which 1 is coded as "1" instead of "11".
		 * We grab two bits, the first bit is the code
		 * and the second is the sign.
		 */
		int v;
		GET_BITS(bs_, 2, nbb, bb, v);
		val = v;
		/*XXX quantize?*/
		blk[0] = qt[(v & 1) ? 0xff : 1];
		k = 1;
		m0 |= 1;
	} else {
		k = 0;
#ifndef INT_64
		blk[0] = 0;/*XXX need this because the way we set bits below*/
#endif
	}

	if (k != 0) {
		printf("0:%d ", dump_quantized_? val : blk[0]);
	}
	int nc = 0;
	for (;;) {
		int r, v;
		HUFF_DECODE(bs_, ht_tcoeff_, nbb, bb, r);
		if (r <= 0) {
			/* SYM_EOB, SYM_ILLEGAL, or SYM_ESCAPE */
			if (r == SYM_ESCAPE) {
				GET_BITS(bs_, 14, nbb, bb, r);
				v = r & 0xff;
				r >>= 8;
				val = v;
			} else {
				if (r == SYM_ILLEGAL) {
					bb_ = bb;
					nbb_ = nbb;
					err("illegal symbol in block");
				}
				/* EOB */
				break;
			}
		} else {
			v = (r << 22) >> 27;
			r = r & 0x1f;
			val = v;
		}
		k += r;
		if (k >= 64) {
			bb_ = bb;
			nbb_ = nbb;
			err("bad run length %d (r %d, v %d)", k, r, v);
			break;
		}
		printf("%d:%d ", k, dump_quantized_? val : qt[v & 0xff]);
		r = COLZAG[k++];
		blk[r] = qt[v & 0xff];
		++nc;
#ifdef INT_64
		m0 |= (INT_64)1 << r;
#else
		/*
		 * This sets bit "r" if r < 32, otherwise
		 * it sets bit 0, but this is okay since
		 * we always set blk[0] XXX
		 */
		m0 |= 1 << (r & ((r-32) >> 31));
		/*
		 * If r >= 32, this sets bit 64-r in m1.
		 * Otherwise, it does nothing.
		 */
		r -= 32;
		m1 |= (~r >> 31 & 1) << r;
#endif
	}
	/*
	 * Done reading input.  Update bit buffer.
	 */
	bb_ = bb;
	nbb_ = nbb;
	*mask = m0;
#ifndef INT_64
	mask[1] = m1;
#endif
	DUMPBITS('\n');
	return (nc);
}

/*
 * Parse a picture header.  We assume that the
 * start code has already been snarfed.
 */
int P64Dumper::parse_picture_hdr()
{
	int tr;
	GET_BITS(bs_, 5, nbb_, bb_, tr);
	int pt;
	GET_BITS(bs_, 6, nbb_, bb_, pt);
	u_int fmt = (pt >> 2) & 1;
	if (fmt_ != fmt) {
		err("unexpected picture type %d/%d", fmt, fmt_);
		return (-1);
	}
	int v;
	GET_BITS(bs_, 1, nbb_, bb_, v);
	printf("pic tr %d pt 0x%02x x%d ", tr, pt, v);
	while (v != 0) {
		GET_BITS(bs_, 9, nbb_, bb_, v);
		/*
		 * XXX from pvrg code: 0x8c in PSPARE means ntsc.
		 * this is a hack.  we don't support it.
		 */
		int pspare = v >> 1;
		if (pspare == 0x8c && (pt & 0x04) != 0) {
			static int first = 1;
			if (first) {
				err("pvrg ntsc not supported");
				first = 0;
			}
		}
		v &= 1;
	}
	return (0);
}

inline int P64Dumper::parse_sc()
{
	int v;
	GET_BITS(bs_, 16, nbb_, bb_, v);
	DUMPBITS('\n');
	if (v != 0x0001) {
		err("bad start code %04x", v);
		++bad_psc_;
		return (-1);
	}
	return (0);
}

/*
 * Parse a GOB header, which consists of the GOB quantiation
 * factor (GQUANT) and spare bytes that we ignore.
 */
int P64Dumper::parse_gob_hdr(int ebit)
{
	mba_ = -1;
	mvdh_ = 0;
	mvdv_ = 0;

	/*
	 * Get the next GOB number (or 0 for a picture header).
	 * The invariant at the top of this loop is that the
	 * bit stream is positioned immediately past the last
	 * start code.
	 */
	int gob;
	for (;;) {
		GET_BITS(bs_, 4, nbb_, bb_, gob);
		if (gob != 0)
			break;
		/*
		 * should happen only on first iteration
		 * (if at all).  pictures always start on
		 * packet boundaries per section 5 of the
		 * Internet Draft.
		 */
		if (parse_picture_hdr() < 0) {
			++bad_fmt_;
			DUMPBITS('\n');
			return (-1);
		}
		/*
		 * Check to see that the next 16 bits
		 * are a start code and throw them away.
		 * But first check that we have the bits.
		 */
		int nbit = ((es_ - bs_) << 4) + nbb_ - ebit;
		if (nbit < 20)
			return (0);

		if (parse_sc() < 0)
			return (-1);
	}
	gob -= 1;
	if (fmt_ == IT_QCIF)
		/*
		 * Number QCIF GOBs 0,1,2 instead of 0,2,4.
		 */
		gob >>= 1;

	int mq;
	GET_BITS(bs_, 5, nbb_, bb_, mq);
	qt_ = &quant_[mq << 8];

	int v;
	GET_BITS(bs_, 1, nbb_, bb_, v);
	printf("gob %d q %d x%d ", gob_, mq, v);
	while (v != 0) {
		GET_BITS(bs_, 9, nbb_, bb_, v);
		v &= 1;
	}
	DUMPBITS('\n');

	gob_ = gob;

	return (gob);
}

/*
 * Parse a macroblock header.  If there is no mb header because
 * we hit the next start code, return -1, otherwise 0.
 */
int P64Dumper::parse_mb_hdr(u_int& cbp)
{
	/*
	 * Read the macroblock address (MBA), throwing
	 * away any prefixed stuff bits.
	 */
	int v;
	HUFF_DECODE(bs_, ht_mba_, nbb_, bb_, v);
	if (v <= 0) {
		if (v == SYM_STUFFBITS) {
			printf("pad ");
			DUMPBITS('\n');
		}
		/*
		 * (probably) hit a start code; either the
		 * next GOB or the next picture header.
		 */
		return (v);
	}

	/*
	 * MBA is differentially encoded.
	 */
	mba_ += v;
	if (mba_ >= MBPERGOB) {
		printf("mba? %d ", mba_);
		DUMPBITS('\n');
		err("mba too big %d", mba_);
		return (SYM_ILLEGAL);
	}

	u_int omt = mt_;
	HUFF_DECODE(bs_, ht_mtype_, nbb_, bb_, mt_);
	printf("mba %d ", mba_);
	if (mt_ & MT_INTRA) 
		printf("intra ");
	if (mt_ & MT_FILTER)
		printf("filter ");
	if (mt_ & MT_MQUANT) {
		int mq;
		GET_BITS(bs_, 5, nbb_, bb_, mq);
		qt_ = &quant_[mq << 8];
		printf("q %d ", mq);
	}
	if (mt_ & MT_MVD) {
		/*
		 * Read motion vector.
		 */
		int dh;
		int dv;
		HUFF_DECODE(bs_, ht_mvd_, nbb_, bb_, dh);
		HUFF_DECODE(bs_, ht_mvd_, nbb_, bb_, dv);
		printf("mv(%d,%d) ", dh, dv);
		/*
		 * Section 4.2.3.4
		 * The vector is differentially coded unless any of:
		 *   - the current mba delta isn't 1
		 *   - the current mba is 1, 12, or 23 (mba mod 11 = 1)
		 *   - the last block didn't have motion vectors.
		 *
		 * This arithmetic is twos-complement restricted
		 * to 5 bits.  XXX this code is broken
		 */
		if ((omt & MT_MVD) != 0 && v == 1 &&
		    mba_ != 0 && mba_ != 11 && mba_ != 22) {
			dh += mvdh_;
			dv += mvdv_;
		}
		mvdh_ = (dh << 27) >> 27;
		mvdv_ = (dv << 27) >> 27;
	}
	/*
	 * Coded block pattern.
	 */
	if (mt_ & MT_CBP) {
		HUFF_DECODE(bs_, ht_cbp_, nbb_, bb_, cbp);
		printf("cbp %02x ", cbp);
		if (cbp > 63) {
			DUMPBITS('\n');
			err("cbp invalid %x", cbp);
			return (SYM_ILLEGAL);
		}
	} else
		cbp = 0x3f;

	DUMPBITS('\n');
	return (1);
}

/*
 * Handle the next block in the current macroblock.
 * If tc is non-zero, then coeffcients are present
 * in the input stream and they are parsed.  Otherwise,
 * coefficients are not present, but we take action
 * according to the type macroblock that we have.
 */
void P64Dumper::decode_block(u_int tc, u_int x, u_int y, u_int stride,
			      u_char* front, u_char* back, int sf, int n)
{
	if (tc != 0)
		printf("blk %d ", n);
	short blk[64];
#ifdef INT_64
	INT_64 mask;
#define MASK_VAL	mask
#define MASK_REF	&mask
#else
	u_int mask[2];
#define MASK_VAL	mask[0], mask[1]
#define MASK_REF	mask
#endif
	if (tc != 0)
		parse_block(blk, MASK_REF);

	int off = y * stride + x;
	u_char* out = front + off;

	if (mt_ & MT_INTRA) {
		if (tc != 0)
			rdct(blk, MASK_VAL, out, stride, (u_char*)0);
		else {
			u_char* in = back + off;
			mvblka(in, out, stride);
		}
		return;
	}
	if ((mt_ & MT_MVD) == 0) {
		u_char* in = back + off;
		if (tc != 0)
			rdct(blk, MASK_VAL, out, stride, in);
		else
			mvblka(in, out, stride);
		return;
	}
	u_int sx = x + (mvdh_ / sf);
	u_int sy = y + (mvdv_ / sf);
	u_char* in = back + sy * stride + sx;
	if (mt_ & MT_FILTER) {
		filter(in, out, stride);
		if (tc != 0)
			rdct(blk, MASK_VAL, out, stride, out);
	} else {
		if (tc != 0)
			rdct(blk, MASK_VAL, out, stride, in);
		else
			mvblk(in, out, stride);
	}
}

/*
 * Decompress the next macroblock.  Return 0 if the macroblock
 * was present (with no errors).  Return SYM_STARTCODE (-1),
 * if there was no macroblock but instead the start of the
 * next GOB or picture (in which case the start code has
 * been consumed).  Return SYM_ILLEGAL (-2) if there was an error.
 */
int P64Dumper::decode_mb()
{
	u_int cbp;
	//register int mba = mba_ + 1;
	register int v;

	if ((v = parse_mb_hdr(cbp)) <= 0)
		return (v);

	/*
	 * Lookup the base coordinate for this MBA.
	 * Convert from a block to a pixel coord.
	 */
	register u_int x, y;
	x = coord_[mba_];
	y = (x & 0xff) << 3;
	x >>= 8;
	x <<= 3;

	/* Update bounding box */
	if (x < minx_)
		minx_ = x;
	if (x > maxx_)
		maxx_ = x;
	if (y < miny_)
		miny_ = y;
	if (y > maxy_)
		maxy_ = y;

	/*
	 * Decode the six blocks in the MB (4Y:1U:1V).
	 * (This code assumes MT_TCOEFF is 1.)
	 */
	register u_int tc = mt_ & MT_TCOEFF;
	register u_int s = width_;
	decode_block(tc & (cbp >> 5), x, y, s, front_, back_, 1, 1);
	decode_block(tc & (cbp >> 4), x + 8, y, s, front_, back_, 1, 2);
	decode_block(tc & (cbp >> 3), x, y + 8, s, front_, back_, 1, 3);
	decode_block(tc & (cbp >> 2), x + 8, y + 8, s, front_, back_, 1, 4);
	s >>= 1;
	int off = size_;
	decode_block(tc & (cbp >> 1), x >> 1, y >> 1, s,
		     front_ + off, back_ + off, 2, 5);
	off += size_ >> 2;
	decode_block(tc & (cbp >> 0), x >> 1, y >> 1, s,
		     front_ + off, back_ + off, 2, 6);

	mbst_[mba_] = MBST_NEW;
	/*
	 * If a marking table was attached, take note.
	 * This allows us to dither only the blocks that have changed,
	 * rather than the entire image on each frame.
	 */
	if (marks_) {
		/* convert to 8x8 block offset */
		off = (x >> 3) + (y >> 3) * (width_ >> 3);
		int m = mark_;
		marks_[off] = m;
		marks_[off + 1] = m;
		off += width_ >> 3;
		marks_[off] = m;
		marks_[off + 1] = m;
	}
	return (0);
}

/*
 * Decode H.261 stream.  Decoding can begin on either
 * a GOB or macroblock header.  All the macroblocks of
 * a given frame can be decoded in any order, but chunks
 * cannot be reordered across frame boundaries.  Since data
 * can be decoded in any order, this entry point can't tell
 * when a frame is fully decoded (actually, we could count
 * macroblocks but if there is loss, we would not know when
 * to sync).  Instead, the callee should sync the decoder
 * by calling the sync() method after the entire frame 
 * has been decoded (modulo loss).
 *
 * This routine should not be called with more than
 * one frame present since there is no callback mechanism
 * for renderering frames (i.e., don't call this routine
 * with a buffer that has a picture header that's not
 * at the front).
 */
int P64Dumper::decode(const u_char* bp, int cc, int sbit, int ebit,
		      int mba, int gob, int mq, int mvdh, int mvdv)
{
	ps_ = (u_short*)bp;
	/*
	 * If cc is even, ignore 8 extra bits in last short.
	 */
	int odd = cc & 1;
	ebit += (odd ^ 1) << 3;
	pebit_ = ebit;
	cc -= odd;
	es_ = (u_short*)(bp + cc);

	/*
	 * If input buffer not aligned, prime bit-buffer
	 * with 8 bits; otherwise, prime it with a 16.
	 */
	if ((long)bp & 1) {
		bs_ = (u_short*)(bp + 1);
		bb_ = *bp;
		nbb_ = 8 - sbit;
	} else {
		bs_ = (u_short*)bp;
		HUFFRQ(bs_, bb_);
		nbb_ = 16 - sbit;
	}
	dbs_ = bs_;
	dnbb_ = nbb_;
	dbb_ = bb_;

	mba_ = mba;
	qt_ = &quant_[mq << 8];
	mvdh_ = mvdh;
	mvdv_ = mvdv;
	/*XXX don't rely on this*/
	if (gob != 0) {
		gob -= 1;
		if (fmt_ == IT_QCIF)
			gob >>= 1;
	}

	while (bs_ < es_ || (bs_ == es_ && nbb_ > ebit)) {
		mbst_ = &mb_state_[gob << 6];
		coord_ = &base_[gob << 6];

		int v = decode_mb();
		if (v == 0)
			continue;

		if (v != SYM_STARTCODE) {
			++bad_bits_;
			return (0);
		}
		gob = parse_gob_hdr(ebit);
		if (gob < 0) {
			/*XXX*/
			++bad_bits_;
			return (0);
		}
	}
	fflush(stdout);
	return (1);
}
