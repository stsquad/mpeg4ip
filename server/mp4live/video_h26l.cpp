/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4live.h"
#include "video_h26l.h"

CH26LVideoEncoder::CH26LVideoEncoder()
{
	m_vopBuffer = NULL;
	m_vopBufferLength = 0;
}

static char* H26LStaticConfig = 
"InputHeaderLength     = 0      # If the inputfile has a header, state it's length in byte here\n"
"TRModulus             = 256 # Modulus for TR, not yet used, MUST be 256\n"
"PicIdModulus          = 256 # Modulus for the PictureID (used for RPS), not yet used, MUST be 256\n"
"FrameSkip            =  0  # Number of frames to be skipped in input (e.g 2 will code every third frame)\n"
"MVResolution         =  0  # Motion Vector Resolution: 0: 1/4-pel, 1: 1/8-pel\n"
"UseHadamard          =  1  # Hadamard transform (0=not used, 1=used)\n"
"SearchRange          = 16  # Max search range\n"
"NumberRefereceFrames =  2  # Number of previous frames used for inter motion search (1-5)\n"
"MbLineIntraUpdate    =  0  # Error robustness(extra intra macro block updates)(0=off, N: One GOB every N frames are intra coded)\n"
"InterSearch16x16     =  1  # Inter block search 16x16 (0=disable, 1=enable)\n"
"InterSearch16x8      =  1  # Inter block search 16x8  (0=disable, 1=enable)\n"
"InterSearch8x16      =  1  # Inter block search  8x16 (0=disable, 1=enable)\n"
"InterSearch8x8       =  1  # Inter block search  8x8  (0=disable, 1=enable)\n"
"InterSearch8x4       =  1  # Inter block search  8x4  (0=disable, 1=enable)\n"
"InterSearch4x8       =  1  # Inter block search  4x8  (0=disable, 1=enable)\n"
"InterSearch4x4       =  1  # Inter block search  4x4  (0=disable, 1=enable)\n"
"SliceMode            = 0   # Slice mode (0=off 1=fixed #mb in slice 2=fixed #bytes in slice 3=use callback)\n"
"SliceArgument        = 11  # Slice argument (Arguments to modes 1 and 2 above)\n"
"NumberBFrames        =  0  # Number of B frames inserted (0=not used)  \n"
"QPBPicture           = 17  # Quant. param for B frames (0-31)\n"
"SPPicturePeriodicity =  0  # SP-Picture Periodicity (0=not used)\n"
"QPSPPicture          = 16  # Quant. param of SP-Pictures for Prediction Error (0-31)\n"
"QPSP2Picture         = 15  # Quant. param of SP-Pictures for Predicted Blocks (0-31)\n"
"SymbolMode           =  0  # Symbol mode (Entropy coding method: 0=UVLC, 1=CABAC)\n"
"OutFileMode          =  0  # Output file mode, 0:Bitstream, 1:RTP\n"
"PartitionMode        =  0  # Partition Mode, 0: no DP, 1: 3 Partitions per Slice (3 Partitions not yet supported)\n"
"SequenceHeaderType   =  0  # Type of Sequence HeaderType (0:none, 1:MiniBinary)\n"
"RestrictSearchRange  =  0  # restriction for (0: blocks and ref, 1: ref, 2: no restrictions)\n"
"RDOptimization       =  1  # rd-optimized mode decision (0:off, 1:on, 2: with losses)\n"
"LossRate             = 10  # expected packet loss rate of the channel, only valid if RDOptimization = 2\n"
"NumberOfDecoders     = 30  # Numbers of decoders used to simulate the channel, only valid if RDOptimization = 2\n"
"UseConstrainedIntraPred  =  0  # If 1, Inter pixels are not used for Intra macroblock prediction.\n"
"LastFrameNumber          =  0  # Last frame number that have to be coded (0: no effect)\n"
"ChangeQPP                = 16  # QP (P-frame) for second part of sequence\n"
"ChangeQPB                = 18  # QP (B-frame) for second part of sequence\n"
"ChangeQPStart            =  0  # Frame no. for second part of sequence (0: no second part)\n"
"AdditionalReferenceFrame =  0  # Additional ref. frame to check (news_a: 16; news_b,c: 24)\n"
"NumberofLeakyBuckets     =  8                      # Number of Leaky Bucket values\n"
;

bool CH26LVideoEncoder::Init(CLiveConfig* pConfig, bool realTime)
{
	m_pConfig = pConfig;

	// approximately relate desired bit rate to QP
	float ar = (float)(m_pConfig->m_videoWidth * m_pConfig->m_videoHeight) 
		/ (176.0 * 144.0);
	float fps = m_pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE);
	float br = (float)m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE);
	float b = 0.666667 * fps * ar;
	u_int8_t qp;
	for (qp = 30; qp > 0; qp--) {
		if (b >= br) {
			break;
		}
		b *= 1.135;
	}
	debug_message("H.26L QP %u\n", qp);

	u_int32_t keyFrameRate = (u_int32_t)
		(m_pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE)
			* m_pConfig->GetFloatValue(CONFIG_VIDEO_KEY_FRAME_INTERVAL));
	if (keyFrameRate == 0) {
		keyFrameRate = 1;
	}

	char config[8*1024];
	snprintf(config, sizeof(config),
		"SourceWidth = %u\nSourceHeight = %u\n"
		"IntraPeriod = %u\n"
		"QPFirstFrame = %u\nQPRemainingFrame = %u\n"
		"%s",
		m_pConfig->m_videoWidth, m_pConfig->m_videoHeight,
		keyFrameRate,
		qp, qp + 1,
		H26LStaticConfig);

	return (H26L_Init(config) == 0);
}

bool CH26LVideoEncoder::EncodeImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV,
	u_int32_t yStride, u_int32_t uvStride, bool wantKeyFrame,
	Duration elapsed)
{
	m_vopBuffer = (u_int8_t*)malloc(m_pConfig->m_videoMaxVopSize);
	if (m_vopBuffer == NULL) {
		return false;
	}

	if (yStride != m_pConfig->m_videoHeight) {
	  error_message("Y stride is not video height for h26l");
	  return false;
	}

	H26L_Encode(pY, pU, pV, m_vopBuffer, (int*)&m_vopBufferLength);

	return true;
}

bool CH26LVideoEncoder::GetEncodedImage(
	u_int8_t** ppBuffer, u_int32_t* pBufferLength)
{
	*ppBuffer = m_vopBuffer;
	*pBufferLength = m_vopBufferLength;

	m_vopBuffer = NULL;
	m_vopBufferLength = 0;

	return true;
}

bool CH26LVideoEncoder::GetReconstructedImage(
	u_int8_t* pY, u_int8_t* pU, u_int8_t* pV)
{
	return (H26L_GetReconstructed(pY, pU, pV) == 0);
}

void CH26LVideoEncoder::Stop()
{
	H26L_Close();
}

