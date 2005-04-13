/*
 * encoder-h261.h --
 *
 *      H.261 video encoder header file
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

//static const char rcsid[] =
//    "@(#) $Header: /cvsroot/mpeg4ip/mpeg4ip/server/mp4live/h261/encoder-h261.h,v 1.6 2005/04/13 22:23:03 wmaycisco Exp $";

#ifndef __ENCODER_H261_H__
#define __ENCODER_H261_H__ 1
#include "video_encoder.h"
#include "crdef.h"
// wmay - make sure we want uint64_t here...
#define INT_64 uint64_t

#ifdef INT_64
#define NBIT 64
#define BB_INT INT_64
#else
#define NBIT 32
#define BB_INT u_int
#endif

typedef struct pktbuf_t {
  struct pktbuf_t *next;
  uint32_t h261_rtp_hdr;
  uint8_t *data;
  uint32_t len;
} pktbuf_t;

class CH261Encoder : public CVideoEncoder {
	public:
		void setq(int q);
		MediaType GetFrameType (void) { return H261VIDEOFRAME; } ;
		bool Init(void);
		media_free_f GetMediaFreeFunction(void);
	protected:
		CH261Encoder(CVideoProfile *vp,
			     uint16_t mtu,
			     CVideoEncoder *next, 
			     bool realTime = true);
		void StopEncoder();
		virtual void encode(const u_int8_t*, const u_int8_t *crvec);
		void encode_blk(const short* blk, const char* lm);
		virtual int flush(pktbuf_t* pb, int nbit, pktbuf_t* npb);
		char* make_level_map(int q, u_int fthresh);
		void setquantizers(int lq, int mq, int hq);

		virtual void size(int w, int h) = 0;
		virtual void encode_mb(u_int mba, const u_char* frm,
				u_int loff, u_int coff, int how) = 0;

		// from EncoderModule
		int mtu_;
		uint32_t m_encodedBytes;
		// from FrameModule
		uint32_t width_;
		uint32_t height_;
		uint32_t framesize_;
		uint8_t *m_localbuffer;

		/* bit buffer */
		BB_INT m_bitCache;
		u_int m_bitsInCache;

		u_char* m_encoded_frame_buffer;
		u_char* m_pBufferCurrent;           /* buffer current pointer */
		int sbit_;

		u_char lq_;		/* low quality quantizer */
		u_char mq_;		/* medium quality quantizer */
		u_char hq_;		/* high quality quantizer */
		u_char mquant_;		/* the last quantizer we sent to other side */
		int quant_required_;	/* 1 if not quant'd in dct */
		u_int ngob_;
		u_int mba_;

		u_int cif_;		/* 1 for CIF, 0 for QCIF */
		u_int bstride_;
		u_int lstride_;
		u_int cstride_;

		u_int loffsize_;	/* amount of 1 luma block */
		u_int coffsize_;	/* amount of 1 chroma block */
		u_int bloffsize_;	/* amount of 1 block advance */

		char* llm_[32];	/* luma dct val -> level maps */
		char* clm_[32];	/* chroma dct val -> level maps */

		float lqt_[64];		/* low quality quantizer */
		float mqt_[64];		/* medium quality quantizer */
		float hqt_[64];		/* high quality quantizer */

		u_int coff_[12];	/* where to find U given gob# */
		u_int loff_[12];	/* where to find Y given gob# */
		u_int blkno_[12];	/* for CR */

		pktbuf_t *m_head;
		uint8_t *frame_data_; // for 
		Duration m_firstDuration;
		uint8_t m_framesEncoded;
		uint8_t m_framesForQualityCheck;
		uint32_t m_bitRate;
		uint8_t m_framerate;
		Duration m_bitsPerFrame;
};


class CH261PixelEncoder : public CH261Encoder, public ConditionalReplenisher {
	public:
		CH261PixelEncoder(CVideoProfile *vp,
				  uint16_t mtu,
				  CVideoEncoder *next,
				  bool realTime = true);
		virtual ~CH261PixelEncoder() {};
	bool EncodeImage(
		const u_int8_t* pY, const u_int8_t* pU, const u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame,
		Duration elapsedDuration,
		Timestamp srcFrameTimestamp);

	bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength,
		Timestamp *dts, Timestamp *pts);

	bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV);

		void size(int w, int h);
	protected:
		void encode_mb(u_int mba, const u_char* frm,
				u_int loff, u_int coff, int how);
		bool m_started;
		Timestamp m_srcFrameTimestamp;
};

void AddH261ConfigVariables(CVideoProfile *pConfig);
EXTERN_TABLE_F(h261_gui_options);
#endif
