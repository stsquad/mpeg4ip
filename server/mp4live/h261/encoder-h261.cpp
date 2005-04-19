/*
 * encoder-h261.cc --
 *
 *      H.261 video encoder
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
 * modifications done 3/2003, wmay@cisco.com
 */

#include "mpeg4ip.h"
#include "dct.h"

#include "p64-huff.h"
#include "crdef.h"

#include "util.h"
#include "encoder-h261.h"
#include "mp4live_config.h"
#include "video_util_resize.h"

static config_index_t CFG_VIDEO_H261_QUALITY;
static config_index_t CFG_VIDEO_H261_QUALITY_ADJ_FRAMES;
static SConfigVariable H261ConfigVariables[] = {
  CONFIG_INT(CFG_VIDEO_H261_QUALITY, "videoH261Quality", 10),
  CONFIG_INT(CFG_VIDEO_H261_QUALITY_ADJ_FRAMES, "videoH261QualityAdjFrames", 8),
};

GUI_INT_RANGE(gui_q, CFG_VIDEO_H261_QUALITY, "H.261 Quality", 1, 32);
GUI_INT_RANGE(gui_qf, CFG_VIDEO_H261_QUALITY_ADJ_FRAMES, "Frames before Adjusting Quality", 1, 100);
DECLARE_TABLE(h261_gui_options) = {
  TABLE_GUI(gui_q),
  TABLE_GUI(gui_qf),
};
DECLARE_TABLE_FUNC(h261_gui_options);

void AddH261ConfigVariables (CVideoProfile *pConfig)
{
  pConfig->AddConfigVariables(H261ConfigVariables, 
			      NUM_ELEMENTS_IN_ARRAY(H261ConfigVariables));
}

//#define DEBUG_QUALITY_ADJUSTMENT 1

#define HLEN (4)
#define	CIF_WIDTH	352
#define	CIF_HEIGHT	288
#define	QCIF_WIDTH	176
#define	QCIF_HEIGHT	144
#define	BMB		6	/* # blocks in a MB */
#define MBPERGOB	33	/* # of Macroblocks per GOB */


#if BYTE_ORDER == LITTLE_ENDIAN
#if NBIT == 64
#define STORE_BITS(bb, bc) \
	bc[0] = bb >> 56; \
	bc[1] = bb >> 48; \
	bc[2] = bb >> 40; \
	bc[3] = bb >> 32; \
	bc[4] = bb >> 24; \
	bc[5] = bb >> 16; \
	bc[6] = bb >> 8; \
	bc[7] = bb;
#define LOAD_BITS(bc) \
	((BB_INT)bc[0] << 56 | \
	 (BB_INT)bc[1] << 48 | \
	 (BB_INT)bc[2] << 40 | \
	 (BB_INT)bc[3] << 32 | \
	 (BB_INT)bc[4] << 24 | \
	 (BB_INT)bc[5] << 16 | \
	 (BB_INT)bc[6] << 8 | \
	 (BB_INT)bc[7])
#else
#define STORE_BITS(bb, bc) \
	bc[0] = bb >> 24; \
	bc[1] = bb >> 16; \
	bc[2] = bb >> 8; \
	bc[3] = bb;
#define LOAD_BITS(bc) (ntohl(*(BB_INT*)(bc)))
#endif
#else
#define STORE_BITS(bb, bc) *(BB_INT*)bc = (bb);
#define LOAD_BITS(bc) (*(BB_INT*)(bc))
#endif

#define PUT_BITS(bits, n, nbb, bb, bc) \
{ \
	nbb += (n); \
	if (nbb > NBIT)  { \
		u_int extra = (nbb) - NBIT; \
		bb |= (BB_INT)(bits) >> extra; \
		STORE_BITS(bb, bc) \
		bc += sizeof(BB_INT); \
		bb = (BB_INT)(bits) << (NBIT - extra); \
		nbb = extra; \
	} else \
		bb |= (BB_INT)(bits) << (NBIT - (nbb)); \
}




CH261Encoder::CH261Encoder(CVideoProfile *vp,
			   uint16_t mtu,
			   CVideoEncoder *next, 
			   bool realTime) :
  CVideoEncoder(vp, mtu, next, realTime), m_encoded_frame_buffer(0), m_pBufferCurrent(0), ngob_(12)
{
  m_head = NULL;
  frame_data_ = NULL;
  m_framesEncoded = 0;
  m_localbuffer = NULL;
	for (int q = 0; q < 32; ++q) {
		llm_[q] = 0;
		clm_[q] = 0;
	}
}
void CH261Encoder::StopEncoder (void)
{
	for (int q = 0; q < 32; ++q) {
		if (llm_[q] != 0) delete[] llm_[q];
		if (clm_[q] != 0) delete[] clm_[q];
	}
	CHECK_AND_FREE(m_localbuffer);
}

CH261PixelEncoder::CH261PixelEncoder(CVideoProfile *vp,
				     uint16_t mtu,
				     CVideoEncoder *next,
				     bool realTime) : 
  CH261Encoder(vp, mtu, next, realTime), 
  ConditionalReplenisher()
{
	quant_required_ = 0;
	setq(10);

	m_started = false;
}


/*
 * Set up the forward DCT quantization table for
 * INTRA mode operation.
 */
void
CH261Encoder::setquantizers(int lq, int mq, int hq)
{
	int qt[64];
	if (lq > 31)
		lq = 31;
	if (lq <= 0)
		lq = 1;
	lq_ = lq;

	if (mq > 31)
		mq = 31;
	if (mq <= 0)
		mq = 1;
	mq_ = mq;

	if (hq > 31)
		hq = 31;
	if (hq <= 0)
		hq = 1;
	hq_ = hq;

	/*
	 * quant_required_ indicates quantization is not folded
	 * into fdct [because fdct is not performed]
	 */
	if (quant_required_ == 0) {
		/*
		 * Set the DC quantizer to 1, since we want to do this
		 * coefficient differently (i.e., the DC is rounded while
		 * the AC terms are truncated).
		 */
		qt[0] = 1;
		int i;
		for (i = 1; i < 64; ++i)
			qt[i] = lq_ << 1;
		dct_fdct_fold_q(qt, lqt_);

		qt[0] = 1;
		for (i = 1; i < 64; ++i)
			qt[i] = mq_ << 1;
		dct_fdct_fold_q(qt, mqt_);

		qt[0] = 1;
		for (i = 1; i < 64; ++i)
			qt[i] = hq_ << 1;
		dct_fdct_fold_q(qt, hqt_);
	}
}

void
CH261Encoder::setq(int q)
{
	setquantizers(q, q / 2, 1);
}

void
CH261PixelEncoder::size(int w, int h)
{
  // next 3 lines is FrameModule::size(w, h);
		width_ = w;
		height_ = h;
		framesize_ = w * h;
	if (w == CIF_WIDTH && h == CIF_HEIGHT) {
		/* CIF */
		cif_ = 1;
		ngob_ = 12;
		bstride_ = 11;
		lstride_ = 16 * CIF_WIDTH - CIF_WIDTH / 2;
		cstride_ = 8 * 176 - 176 / 2;
		loffsize_ = 16;
		coffsize_ = 8;
		bloffsize_ = 1;
	} else if (w == QCIF_WIDTH && h == QCIF_HEIGHT) {
		/* QCIF */
		cif_ = 0;
		ngob_ = 6; /* not really number of GOBs, just loop limit */
		bstride_ = 0;
		lstride_ = 16 * QCIF_WIDTH - QCIF_WIDTH;
		cstride_ = 8 * 88 - 88;
		loffsize_ = 16;
		coffsize_ = 8;
		bloffsize_ = 1;
	} else {
		/*FIXME*/
		fprintf(stderr, "CH261PixelEncoder: H.261 bad geometry: %dx%d\n",
			w, h);
		exit(1);
	}
	u_int loff = 0;
	u_int coff = 0;
	u_int blkno = 0;
	for (u_int gob = 0; gob < ngob_; gob += 2) {
		loff_[gob] = loff;
		coff_[gob] = coff;
		blkno_[gob] = blkno;
		/* width of a GOB (these aren't ref'd in QCIF case) */
		loff_[gob + 1] = loff + 11 * 16;
		coff_[gob + 1] = coff + 11 * 8;
		blkno_[gob + 1] = blkno + 11;

		/* advance to next GOB row */
		loff += (16 * 16 * MBPERGOB) << cif_;
		coff += (8 * 8 * MBPERGOB) << cif_;
		blkno += MBPERGOB << cif_;
	}
}

/*
 * Make a map to go from a 12 bit dct value to an 8 bit quantized
 * 'level' number.  The 'map' includes both the quantizer (for the
 * dct encoder) and the perceptual filter 'threshold' (for both
 * the pixel & dct encoders).  The first 4k of the map is for the
 * unfiltered coeff (the first 20 in zigzag order; roughly the
 * upper left quadrant) and the next 4k of the map are for the
 * filtered coef.
 */
char*
CH261Encoder::make_level_map(int q, u_int fthresh)
{
	/* make the luminance map */
	char* lm = new char[0x2000];
	char* flm = lm + 0x1000;
	int i;
	lm[0] = 0;
	flm[0] = 0;
	q = quant_required_? q << 1 : 0;
	for (i = 1; i < 0x800; ++i) {
		int l = i;
		if (q)
			l /= q;
		lm[i] = l;
		lm[-i & 0xfff] = -l;

		if ((u_int)l <= fthresh)
			l = 0;
		flm[i] = l;
		flm[-i & 0xfff] = -l;
	}
	return (lm);
}

/*
 * encode_blk:
 *	encode a block of DCT coef's
 */
void
CH261Encoder::encode_blk(const short* blk, const char* lm)
{
	BB_INT bb = m_bitCache;
	u_int nbb = m_bitsInCache;
	uint8_t * bc = m_pBufferCurrent;

	/*
	 * Quantize DC.  Round instead of truncate.
	 */
	int dc = (blk[0] + 4) >> 3;

	if (dc <= 0)
		/* shouldn't happen with CCIR 601 black (level 16) */
		dc = 1;
	else if (dc > 254)
		dc = 254;
	else if (dc == 128)
		/* per Table 6/H.261 */
		dc = 255;
	/* Code DC */
	PUT_BITS(dc, 8, nbb, bb, bc);
	int run = 0;
	const u_char* colzag = &DCT_COLZAG[0];
	for (int zag; (zag = *++colzag) != 0; ) {
		if (colzag == &DCT_COLZAG[20])
			lm += 0x1000;
		int level = lm[((const u_short*)blk)[zag] & 0xfff];
		if (level != 0) {
			int val, nb;
			huffent* he;
			if (u_int(level + 15) <= 30 &&
			    (nb = (he = &hte_tc[((level&0x1f) << 6)|run])->nb))
				/* we can use a VLC. */
				val = he->val;
			else {
				 /* Can't use a VLC.  Escape it. */
				val = (1 << 14) | (run << 8) | (level & 0xff);
				nb = 20;
			}
			PUT_BITS(val, nb, nbb, bb, bc);
			run = 0;
		} else
			++run;
	}
	/* EOB */
	PUT_BITS(2, 2, nbb, bb, bc);

	m_bitCache = bb;
	m_bitsInCache = nbb;
	m_pBufferCurrent = bc;
}

/*
 * CH261PixelEncoder::encode_mb
 *	encode a macroblock given a set of input YUV pixels
 */
void
CH261PixelEncoder::encode_mb(u_int mba, const u_char* frm,
			    u_int loff, u_int coff, int how)
{
	register int q;
	float* qt;
	if (how == CR_LQ) {
		q = lq_;
		qt = lqt_;
	} else if (how == CR_HQ) {
		q = hq_;
		qt = hqt_;
	} else {
		/* must be medium quality */
		q = mq_;
		qt = mqt_;
	}

	/*
	 * encode all 6 blocks of the macro block to find the largest
	 * coef (so we can pick a new quantizer if gquant doesn't have
	 * enough range).
	 */
	/*FIXME this can be u_char instead of short but need smarts in fdct */
	short blk[64 * 6];
	register int stride = width_;
	/* luminance */
	const u_char* p = &frm[loff];
	dct_fdct(p, stride, blk + 0, qt);
	dct_fdct(p + 8, stride, blk + 64, qt);
	dct_fdct(p + 8 * stride, stride, blk + 128, qt);
	dct_fdct(p + (8 * stride + 8), stride, blk + 192, qt);
	/* chominance */
	int fs = framesize_;
	p = &frm[fs + coff];
	stride >>= 1;
	dct_fdct(p, stride, blk + 256, qt);
	dct_fdct(p + (fs >> 2), stride, blk + 320, qt);

	/*
	 * if the default quantizer is too small to handle the coef.
	 * dynamic range, spin through the blocks and see if any
	 * coef. would significantly overflow.
	 */
	if (q < 8) {
		register int cmin = 0, cmax = 0;
		register short* bp = blk;
		for (register int i = 6; --i >= 0; ) {
			++bp;	// ignore dc coef
			for (register int j = 63; --j >= 0; ) {
				register int v = *bp++;
				if (v < cmin)
					cmin = v;
				else if (v > cmax)
					cmax = v;
			}
		}
		if (cmax < -cmin)
			cmax = -cmin;
		if (cmax >= 128) {
			/* need to re-quantize */
			register int s;
			for (s = 1; cmax >= (128 << s); ++s) {
			}
			q <<= s;
			if (q > 31) q = 31;
			if (q < 1) q = 1;
			register short* bp = blk;
			for (register int i = 6; --i >= 0; ) {
				++bp;	// ignore dc coef
				for (register int j = 63; --j >= 0; ) {
					register int v = *bp;
					*bp++ = v >> s;
				}
			}
		}
	}

	u_int m = mba - mba_;
	mba_ = mba;
	huffent* he = &hte_mba[m - 1];
	/* MBA */
	PUT_BITS(he->val, he->nb, m_bitsInCache, m_bitCache, m_pBufferCurrent);
	if (q != mquant_) {
		/* MTYPE = INTRA + TC + MQUANT */
		PUT_BITS(1, 7, m_bitsInCache, m_bitCache, m_pBufferCurrent);
		PUT_BITS(q, 5, m_bitsInCache, m_bitCache, m_pBufferCurrent);
		mquant_ = q;
	} else {
		/* MTYPE = INTRA + TC (no quantizer) */
		PUT_BITS(1, 4, m_bitsInCache, m_bitCache, m_pBufferCurrent);
	}

	/* luminance */
	/*const*/ char* lm = llm_[q];
	if (lm == 0) {
		lm = make_level_map(q, 1);
		llm_[q] = lm;
		clm_[q] = make_level_map(q, 2);
	}
	encode_blk(blk + 0, lm);
	encode_blk(blk + 64, lm);
	encode_blk(blk + 128, lm);
	encode_blk(blk + 192, lm);
	/* chominance */
	lm = clm_[q];
	encode_blk(blk + 256, lm);
	encode_blk(blk + 320, lm);
}

int
CH261Encoder::flush(pktbuf_t* pb, int last_mb_start_bits, pktbuf_t* pNewBuffer)
{
	/* flush bit buffer */
	STORE_BITS(m_bitCache, m_pBufferCurrent);

	int cc = (last_mb_start_bits + 7) >> 3;
	int ebit = (cc << 3) - last_mb_start_bits;

	/*FIXME*/
	if (cc == 0 && pNewBuffer != 0)
		abort();

	pb->len = cc;
#if 0
	rtphdr* rh = (rtphdr*)pb->data;
	if (pNewBuffer == 0)
		rh->rh_flags |= htons(RTP_M);
#endif

	uint32_t h = pb->h261_rtp_hdr | (ebit << 26) | (sbit_ << 29);
	pb->h261_rtp_hdr = htonl(h);

	if (pNewBuffer != 0) {
		u_char* new_buffer_start = pNewBuffer->data;
		// bc is bit count including last macroblock that we're not
		// sending (in bytes)
		u_int bc = (m_pBufferCurrent - m_encoded_frame_buffer) << 3;
		// tbit is bit count including last macroblock, including bit
		// that we STORE_BITS above
		int tbit = bc + m_bitsInCache;
		// extra is # of bits in buffer that we want to save (pretty much	
		// the whole macro block
		int extra = ((tbit + 7) >> 3) - (last_mb_start_bits >> 3);
		if (extra > 0)
			memcpy(new_buffer_start, m_encoded_frame_buffer + (last_mb_start_bits >> 3), extra);
		m_encoded_frame_buffer = new_buffer_start;
		sbit_ = last_mb_start_bits & 7;
		tbit -= last_mb_start_bits &~ 7;
		bc = tbit &~ (NBIT - 1);
		m_bitsInCache = tbit - bc;
		m_pBufferCurrent = m_encoded_frame_buffer + (bc >> 3);
		/*
		 * Prime the bit buffer.  Be careful to set bits that
		 * are not yet in use to 0, since output bits are later
		 * or'd into the buffer.
		 */
		if (m_bitsInCache > 0) {
			u_int n = NBIT - m_bitsInCache;
			m_bitCache = (LOAD_BITS(m_pBufferCurrent) >> n) << n;
		} else
			m_bitCache = 0;
	}
#if 0
	target_->recv(pb);
#else
	pb->next = pNewBuffer;
#endif

	return (cc + HLEN);
}


static pktbuf_t *alloc_pktbuf(uint32_t mtu)
{
  pktbuf_t *ret = MALLOC_STRUCTURE(pktbuf_t);
  ret->data = (uint8_t *)malloc(mtu * 2);
  ret->next = NULL;
  ret->len = 0;
  return ret;

}

void free_pktbuf (pktbuf_t *pkt)
{
  free(pkt->data);
  free(pkt);
}


void free_pktbuf_list(pktbuf_t *p)
{
  pktbuf_t *q;
  while (p != NULL) {
    q = p->next;
    free_pktbuf(p);
    p = q;
  }
}

media_free_f CH261Encoder::GetMediaFreeFunction(void)
{
  return (media_free_f)free_pktbuf_list;
}

void CH261Encoder::encode(const uint8_t* vf, const u_int8_t *crvec)
{
  pktbuf_t* pb = alloc_pktbuf(mtu_);
  if (m_head != NULL) {
    free_pktbuf_list(m_head);
  }
  m_head = pb;
	m_encoded_frame_buffer = pb->data;
	m_pBufferCurrent = m_encoded_frame_buffer;
	u_int max_bits_in_buffer = (mtu_ - HLEN) << 3;
	m_bitCache = 0;
	m_bitsInCache = 0;
	sbit_ = 0;
	/* RTP/H.261 header */
	pb->h261_rtp_hdr = (1 << 25) | (lq_ << 10);

	/* PSC */
	PUT_BITS(0x0001, 16, m_bitsInCache, m_bitCache, m_pBufferCurrent);
	/* GOB 0 -> picture header */
	PUT_BITS(0, 4, m_bitsInCache, m_bitCache, m_pBufferCurrent);
	/* TR (FIXME should do this right) */
	PUT_BITS(0, 5, m_bitsInCache, m_bitCache, m_pBufferCurrent);
	/* PTYPE = CIF */
	int pt = cif_ ? 6 : 2;
	PUT_BITS(pt, 6, m_bitsInCache, m_bitCache, m_pBufferCurrent);
	/* PEI */
	PUT_BITS(0, 1, m_bitsInCache, m_bitCache, m_pBufferCurrent);

	int step = cif_ ? 1 : 2;

	const u_int8_t* frm = vf;
	for (u_int gob = 0; gob < ngob_; gob += step) {
		u_int loff = loff_[gob];
		u_int coff = coff_[gob];
		u_int blkno = blkno_[gob];
		u_int last_mb_start_bits = ((m_pBufferCurrent - m_encoded_frame_buffer) << 3) + m_bitsInCache;

		/* GSC/GN */
		PUT_BITS(0x10 | (gob + 1), 20, m_bitsInCache, m_bitCache, m_pBufferCurrent);
		/* GQUANT/GEI */
		mquant_ = lq_;
		PUT_BITS(mquant_ << 1, 6, m_bitsInCache, m_bitCache, m_pBufferCurrent);

		mba_ = 0;
		int line = 11;
		for (u_int mba = 1; mba <= 33; ++mba) {
			/*
			 * If the conditional replenishment algorithm
			 * has decided to send any of the blocks of
			 * this macroblock, code it.
			 */
			u_int s = crvec[blkno];
			if ((s & CR_SEND) != 0) {
				u_int mbpred = mba_;
				encode_mb(mba, frm, loff, coff, CR_QUALITY(s));
				u_int cbits = ((m_pBufferCurrent - m_encoded_frame_buffer) << 3) + m_bitsInCache;
				if (cbits > max_bits_in_buffer) {
					pktbuf_t* npb;
					npb = alloc_pktbuf(mtu_);
					m_encodedBytes += 
					  flush(pb, last_mb_start_bits, npb);
					cbits -= last_mb_start_bits;
					pb = npb;
					/* RTP/H.261 header */
					u_int m = mbpred;
					u_int g;
					if (m != 0) {
						g = gob + 1;
						m -= 1;
					} else
						g = 0;

					pb->h261_rtp_hdr = 
						1 << 25 |
						m << 15 |
						g << 20 |
						mquant_ << 10;
				}
				last_mb_start_bits = cbits;
			}

			loff += loffsize_;
			coff += coffsize_;
			blkno += bloffsize_;
			if (--line <= 0) {
				line = 11;
				blkno += bstride_;
				loff += lstride_;
				coff += cstride_;
			}

		}
	}
	m_encodedBytes += flush(pb, ((m_pBufferCurrent - m_encoded_frame_buffer) << 3) + m_bitsInCache, 0);
}


bool
CH261PixelEncoder::EncodeImage(const uint8_t *pY, 
			       const uint8_t *pU,
			       const uint8_t *pV,
			       uint32_t yStride,
			       uint32_t uvStride,
			       bool wantKeyFrame,
			       Duration elapsedDuration,
			       Timestamp srcTimestamp)
{
  uint32_t y;
  const uint8_t *pFrame;
  // check if everything is all together
  m_srcFrameTimestamp = srcTimestamp;
  pFrame = pY;
  if (m_localbuffer == NULL) {
    m_localbuffer = (uint8_t *)malloc(framesize_ + (framesize_ / 2));
  }
  CopyYuv(pY, pU, pV, yStride, uvStride, uvStride,
	  m_localbuffer, m_localbuffer + framesize_, 
	  m_localbuffer + framesize_ + (framesize_ / 4),
	  width_, width_ / 2, width_ / 2, 
	  width_, height_);
  pFrame = m_localbuffer;
  pY = m_localbuffer;
  pU = m_localbuffer + framesize_;
  pV = pU + framesize_ / 4;

  // check if we should adjust quality
  if (m_framesEncoded == 0) {
    m_firstDuration = elapsedDuration;
    m_framesEncoded++;
  } else {
    if (m_framesEncoded >= m_framesForQualityCheck) {
      Duration dur8frames = elapsedDuration - m_firstDuration;
      dur8frames /= 1000;

      Duration realBps = (m_encodedBytes * 8);
      Duration calcBps;
      calcBps = m_bitRate;

      realBps *= 1000;
      realBps /= dur8frames;

      int adj = 0;
      Duration diff = 0;
      if (calcBps > realBps) {
	// we can up the quality a bit
	diff = (calcBps - realBps) / m_framerate;
	if (diff > m_bitsPerFrame / 2) {
	  adj = 2;
	} else if (diff > m_bitsPerFrame / 4) {
	  adj = 1;
	}
      } else if (calcBps < realBps) {
	diff = (realBps - calcBps) / m_framerate;
	if (diff > m_bitsPerFrame / 2) {
	  adj = -6;
	} else if (diff > m_bitsPerFrame / 4) {
	  adj = -4;
	} else if (diff > m_bitsPerFrame / 5) {
	  adj = -3;
	} else if (diff > m_bitsPerFrame / 8) {
	  adj = -2;
	} else if (diff > 0) {
	  adj = -1;
	}
	// lower quality - we're sending more
      }
#ifdef DEBUG_QUALITY_ADJUSTMENT
	error_message("dur "D64" calc bps "D64" should %u "D64" diff "D64" cmp "D64", adjust %d %d", 
		      dur8frames, calcBps, m_encodedBytes, realBps, diff,
		      m_bitsPerFrame,
		      adj, lq_ - adj);
#endif
      if (adj != 0) {
	setq(lq_ - adj);
      }
      m_firstDuration = elapsedDuration;
      m_encodedBytes = 0;
      m_framesEncoded = 0;
    }
    m_framesEncoded++;
  }
  //debug_message("encoding h261 image");
  if (m_started == false) {
    /* Frame size changed. Reallocate frame data space and reinit crvec */

    if (frame_data_ != 0) {
      delete [] frame_data_;
    }
    frame_data_ = new u_int8_t[width_*height_*3/2];

    if (pY != 0) {
      memcpy(frame_data_, pY, framesize_);
    }

    if (pU != 0) {
      memcpy(frame_data_+framesize_,
	     pU, framesize_/4);
    }

    if (pV != 0) {
      memcpy(frame_data_+ framesize_*5/4,
	     pU, framesize_/ 4);
    }

    crinit(width_, height_);

    m_started = true;
  } else {
    /* Frame size is the same, so do conditional replenishment. */

    int mark = age_blocks() | CR_MOTION_BIT | CR_LQ;

    register int _stride = width_;

    const u_char* rb = &(frame_data_[scan_ * _stride]);
    const u_char* lb = &(pY[scan_ * _stride]);

    u_char* crv = crvec_;

    uint32_t bw = width_/16;
    uint32_t bh = height_/16;

    for (y = 0; y < bh; y++) {
      const u_char* nrb = rb;
      const u_char* nlb = lb;
      u_char* ncrv = crv;

      for (uint32_t x = 0; x < bw; x++) {
	int tl = 0;
	int tc1 = 0;
	int tc2 = 0;
	int tr = 0;
	int bl = 0;
	int bc1 = 0;
	int bc2 = 0;
	int br = 0;

	tl = lb[0] - rb[0] + lb[1] - rb[1] + lb[2] - rb[2] + lb[3] - rb[3];
	if (tl < 0) tl = -tl;

	tc1 = lb[4] - rb[4] + lb[5] - rb[5] + lb[6] - rb[6] + lb[7] - rb[7];
	if (tc1 < 0) tc1 = -tc1;

	tc2 = lb[8] - rb[8] + lb[9] - rb[9] + lb[10] - rb[10] + lb[11] -rb[11];
	if (tc2 < 0) tc2 = -tc2;

	tr = lb[12] - rb[12] + lb[13] - rb[13] + lb[14] - rb[14] +
	  lb[15] - rb[15];
	if (tr < 0) tr = -tr;

	lb += _stride << 3;
	rb += _stride << 3;

	bl = lb[0] - rb[0] + lb[1] - rb[1] + lb[2] - rb[2] + lb[3] - rb[3];
	if (bl < 0) bl = -bl;

	bc1 = lb[4] - rb[4] + lb[5] - rb[5] + lb[6] - rb[6] + lb[7] - rb[7];
	if (bc1 < 0) bc1 = -bc1;

	bc2 = lb[8] - rb[8] + lb[9] - rb[9] + lb[10] - rb[10] + lb[11] -rb[11];
	if (bc2 < 0) bc2 = -bc2;

	br = lb[12] - rb[12] + lb[13] - rb[13] + lb[14] - rb[14] +
	  lb[15] - rb[15];
	if (br < 0) br = -br;

	lb -= _stride << 3;
	rb -= _stride << 3;

	if (scan_ < 4) {
	  /* north-west */
	  if ((tl >= 24) && (x > 0) && (y > 0)) {
	    crv[-bw-1] = mark;
	  }
	  /* north */
	  if (((tl >= 24) || (tc1 >= 24) || (tc2 >= 24) || (tr >= 24)) &&
	      (y > 0)) {
	    crv[-bw] = mark;
	  }
	  /* north-east */
	  if ((tr >= 24) && (x < bw - 1) && (y > 0)) {
	    crv[-bw+1] = mark;
	  }
	  /* west */
	  if (((tl >= 24) || (bl >= 24)) && (x > 0)) {
	    crv[-1] = mark;
	  }
	  /* middle */
	  if ((tl >= 24) || (tc1 >= 24) || (tc2 >= 24) || (tr >= 24) ||
	      (bl >= 24) || (bc1 >= 24) || (bc2 >= 24) || (br >= 24)) {
	    crv[0] = mark;
	  }
	  /* east */
	  if (((tr >= 24) || (br >=24)) && (x < bw - 1)) {
	    crv[1] = 0;
	  }
	} else {
	  /* south-west */
	  if ((bl >= 24) && (x > 0) && (y < bh-1)) {
	    crv[bw-1] = mark;
	  }
	  /* south */
	  if (((bl >= 24) || (bc1 >= 24) || (bc2 >= 24) || (br >= 24)) &&
	      (y < bh-1)) {
	    crv[bw] = mark;
	  }
	  /* south-east */
	  if ((br >= 24) && (x < bw - 1) && (y < bh - 1)) {
	    crv[bw+1] = mark;
	  }
	  /* west */
	  if (((bl >= 24) || (tl >= 24)) && (x > 0)) {
	    crv[-1] = mark;
	  }
	  /* middle */
	  if ((bl >= 24) || (bc1 >= 24) || (bc2 >= 24) || (br >= 24) ||
	      (tl >= 24) || (tc1 >= 24) || (tc2 >= 24) || (tr >= 24)) {
	    crv[0] = mark;
	  }
	  /* east */
	  if (((br >= 24) || (tr >=24)) && (x < bw - 1)) {
	    crv[1] = 0;
	  }
	}
	lb += 16;
	rb += 16;
	crv++;
      }
      lb = nlb + (_stride << 4);
      rb = nrb + (_stride << 4);
      crv = ncrv + bw;
    }

    /* Copy blocks into frame based on conditional replenishment */

    crv = crvec_;
    uint32_t off = framesize_;
    u_char* dest_lum = frame_data_;
    u_char* dest_cr = frame_data_+off;
    u_char* dest_cb = frame_data_+off+(off/4);
    const u_char* src_lum = pFrame;
    const u_char* src_cr = pFrame + off;
    const u_char* src_cb = pFrame + off + (off / 4);

    //debug_message("Sending start");
    for (y = 0; y < bh; y++) {
      int i;
      for (uint32_t x = 0; x < bw; x++) {
	int s = *crv++;
	if ((s & CR_SEND) != 0) {
	  //  debug_message("Sending %u %u", y, x);
	  int idx = y*_stride*16+x*16;
	  u_int32_t* sl = (u_int32_t*) &(src_lum[idx]);
	  u_int32_t* dl = (u_int32_t*) &(dest_lum[idx]);
	  for(i=0; i<16; i++) {
	    dl[0] = sl[0];
	    dl[1] = sl[1];
	    dl[2] = sl[2];
	    dl[3] = sl[3];
	    dl += (_stride / 4);
	    sl += (_stride / 4);
	  }

	  idx = y*(_stride/2)*8+x*8;
	  u_int32_t* scr = (u_int32_t*) &(src_cr[idx]);
	  u_int32_t* scb = (u_int32_t*) &(src_cb[idx]);
	  u_int32_t* dcr = (u_int32_t*) &(dest_cr[idx]);
	  u_int32_t* dcb = (u_int32_t*) &(dest_cb[idx]);
	  for(i=0; i<8; i++) {
	    dcr[0] = scr[0];
	    dcr[1] = scr[1];
	    dcb[0] = scb[0];
	    dcb[1] = scb[1];
	    dcr += _stride / 8;
	    dcb += _stride / 8;
	    scr += _stride / 8;
	    scb += _stride / 8;
	  }
	}
      }
    }
  }

  encode(pFrame, crvec_);

  return true;
}

bool CH261PixelEncoder::GetEncodedImage(uint8_t **ppBuffer, 
				       uint32_t *pBufferLength,
					Timestamp *dts, 
					Timestamp *pts)
{
  *dts = *pts = m_srcFrameTimestamp;
  *ppBuffer = (uint8_t *)m_head;
  m_head = NULL;
  *pBufferLength = 0;
  return true;
}

bool CH261PixelEncoder::GetReconstructedImage(uint8_t *pY, 
					     uint8_t *pU, 
					     uint8_t *pV)
{
  uint32_t uvsize = framesize_ / 4;
  memcpy(pY,frame_data_, framesize_);
  memcpy(pU, frame_data_ + framesize_, uvsize);
  memcpy(pV, frame_data_ + framesize_ + uvsize, uvsize);
  return true;
}

bool CH261Encoder::Init(void)
{
  float value;
  setq(Profile()->GetIntegerValue(CFG_VIDEO_H261_QUALITY));
  m_bitRate = Profile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000;

  value = Profile()->GetFloatValue(CFG_VIDEO_FRAME_RATE);
  value = value + 0.5;
  m_framerate = (uint8_t)value;
  m_bitsPerFrame = m_bitRate / m_framerate;
  m_framesForQualityCheck = Profile()->GetIntegerValue(CFG_VIDEO_H261_QUALITY_ADJ_FRAMES);
  size(Profile()->m_videoWidth, Profile()->m_videoHeight);
  mtu_ = m_mtu;
  //  mtu_ = m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE);
  return true;
}
