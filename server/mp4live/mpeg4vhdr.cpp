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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4live.h"

typedef struct {
	u_int8_t*	pData;
	u_int32_t	numbits;
	u_int32_t	numbytes;
} BitBuffer; 

/* forward declarations */
static bool init_putbits(BitBuffer* pBitBuf, u_int32_t maxNumBits);
static bool putbits(BitBuffer* pBitBuf, u_int32_t dataBits, u_int8_t numBits);
static void destroy_putbits(BitBuffer* pBitBuf);

void GenerateMpeg4VideoConfig(CLiveConfig* pConfig)
{
	// OPTION to make these configurable
	bool want_vosh = false;			
	bool want_vo = true;		
	bool want_vol = true;	
	bool want_short_time = 
		!pConfig->GetBoolValue(CONFIG_VIDEO_USE_DIVX_ENCODER);
	bool want_variable_rate = true;

	BitBuffer config;
	init_putbits(&config, (5 + 9 + 20) * 8);

	if (want_vosh) {
		/* VOSH - Visual Object Sequence Header */
		putbits(&config, 0x000001B0, 32);

		// profile_level_id, default is 3, Simple Profile @ Level 3
		putbits(&config, 
			pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_LEVEL_ID), 8);
	}

	if (want_vo) {
#ifdef NOTDEF
		// These should really be inserted,
		// but current divx decoder barfs if they are present

		// VO - Visual Object
		putbits(&config, 0x000001B5, 32);

		// no verid, priority or signal type
		putbits(&config, 0x08, 8);
#endif
		// video object 1
		putbits(&config, 0x00000100, 32);
	}
	
	if (want_vol) {
		/* VOL - Video Object Layer */

		/* VOL 1 */
		putbits(&config, 0x00000120, 32);
		/* 1 bit - random access = 0 (1 only if every VOP is an I frame) */
		putbits(&config, 0, 1);
		/*
		 * 8 bits - type indication 
		 * 		= 1 (simple profile)
		 * 		= 4 (main profile)
		 */
		putbits(&config, pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID), 8);
		/* 1 bit - is object layer id = 1 */
		putbits(&config, 1, 1);
		/* 4 bits - visual object layer ver id = 2 */
		putbits(&config, 2, 4); 
		/* 3 bits - visual object layer priority = 1 */
		putbits(&config, 1, 3); 

		/* 4 bits - aspect ratio info = 1 (square pixels) */
		putbits(&config, 1, 4);
		/* 1 bit - VOL control params = 0 */
		putbits(&config, 0, 1);
		/* 2 bits - VOL shape = 0 (rectangular) */
		putbits(&config, 0, 2);
		/* 1 bit - marker = 1 */
		putbits(&config, 1, 1);

		u_int16_t frameRate = pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE);
		u_int16_t ticks;
		if (want_short_time /*&& frameRate == (float)((int)frameRate)*/) {
			ticks = (u_int16_t)frameRate;
		} else {
			ticks = 30000;
		}
		/* 16 bits - VOP time increment resolution */
		putbits(&config, ticks, 16);
		/* 1 bit - marker = 1 */
		putbits(&config, 1, 1);
		/* 1 bit - fixed vop rate = 0 or 1 */
		if (want_variable_rate) {
			putbits(&config, 0, 1);
		} else {
			u_int16_t frameDuration = 
				(u_int16_t)((float)ticks / frameRate);
			u_int8_t rangeBits = 1;

			putbits(&config, 1, 1);

			/* 1-16 bits - fixed vop time increment in ticks */
			while (ticks > (1 << rangeBits)) {
				rangeBits++;
			}
			putbits(&config, frameDuration, rangeBits);
		}
		/* 1 bit - marker = 1 */
		putbits(&config, 1, 1);
		/* 13 bits - VOL width */
		putbits(&config, pConfig->m_videoWidth, 13);
		/* 1 bit - marker = 1 */
		putbits(&config, 1, 1);
		/* 13 bits - VOL height */
		putbits(&config, pConfig->m_videoHeight, 13);
		/* 1 bit - marker = 1 */
		putbits(&config, 1, 1);
		/* 1 bit - interlaced = 0 */
		putbits(&config, 0, 1);

		/* 1 bit - overlapped block motion compensation disable = 1 */
		putbits(&config, 1, 1);
		/* 2 bits - sprite usage = 0 */
		putbits(&config, 0, 2);
		/* 1 bit - not 8 bit pixels = 0 */
		putbits(&config, 0, 1);
		/* 1 bit - quant type = 0 */
		putbits(&config, 0, 1);
		/* 1 bit - quarter pixel = 0 */
		putbits(&config, 0, 1);
		/* 1 bit - complexity estimation disable = 1 */
		putbits(&config, 1, 1);
		/* 1 bit - resync marker disable = 1 */
		putbits(&config, 1, 1);
		/* 1 bit - data partitioned = 0 */
		putbits(&config, 0, 1);
		/* 1 bit - newpred = 0 */
		putbits(&config, 0, 1);
		/* 1 bit - reduced resolution vop = 0 */
		putbits(&config, 0, 1);
		/* 1 bit - scalability = 0 */
		putbits(&config, 0, 1);

		/* pad to byte boundary with 0 then as many 1's as needed */
		putbits(&config, 0, 1);
		if ((config.numbits % 8) != 0) {
			putbits(&config, 0xFF, 8 - (config.numbits % 8));
		}
	}

	free(pConfig->m_videoMpeg4Config);
	pConfig->m_videoMpeg4Config = config.pData;
	pConfig->m_videoMpeg4ConfigLength = (config.numbits + 7) / 8;
}

static bool init_putbits(BitBuffer* pBitBuf, u_int32_t maxNumBits)
{
	pBitBuf->pData = (u_int8_t*)malloc((maxNumBits + 7) / 8);
	if (pBitBuf->pData == NULL) {
		return false;
	}
	memset(pBitBuf->pData, 0, (maxNumBits + 7) / 8);
	pBitBuf->numbits = 0;
	return true;
}

static bool putbits(BitBuffer* pBitBuf, u_int32_t dataBits, u_int8_t numBits)
{
	u_int32_t byteIndex;
	u_int8_t freebits;
	int i;

	if (numBits == 0) {
		return true;
	}
	if (numBits > 32) {
		return false;
	}
	
	byteIndex = (pBitBuf->numbits / 8);
	freebits = 8 - (pBitBuf->numbits % 8);
	for (i = numBits; i > 0; i--) {
		pBitBuf->pData[byteIndex] |= 
			(((dataBits >> (i - 1)) & 1) << (--freebits));
		if (freebits == 0) {
			pBitBuf->pData[++byteIndex] = 0;
			freebits = 8;
		}
	}

	pBitBuf->numbits += numBits;
	return true;
}

static void destroy_putbits(BitBuffer* pBitBuf)
{
	free(pBitBuf->pData);
	pBitBuf->pData = NULL;
	pBitBuf->numbits = 0;	
}

char* BinaryToAscii(u_int8_t* pBuf, u_int32_t bufSize)
{
	char* s;
	u_int i, j;

	s = (char*)malloc((2 * bufSize) + 1);
	if (s == NULL) {
		return NULL;
	}

	for (i = 0, j = 0; i < bufSize; i++) {
		sprintf(&s[j], "%02x", pBuf[i]);
		j += 2;
	}

	return s;	/* N.B. caller is responsible for free'ing s */
}

