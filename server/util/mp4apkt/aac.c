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

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <mpeg4ip.h>

#define ADTS_HEADER_MAX_SIZE 10 /* bytes */

static u_int8_t firstHeader[ADTS_HEADER_MAX_SIZE];

/*
 * compute ADTS frame size
 */
static u_int16_t getAdtsFrameSize(u_int8_t* hdr)
{
	/* extract the necessary fields from the header */
	u_int8_t isMpeg4 = hdr[1] & 0x08;
	u_int16_t frameLength;

	if (isMpeg4) {
		frameLength = (((u_int16_t)hdr[4]) << 5) | (hdr[5] >> 3); 
	} else { /* MPEG-2 */
		frameLength = (((u_int16_t)(hdr[3] & 0x3)) << 11) 
			| (((u_int16_t)hdr[4]) << 3) | (hdr[5] >> 5); 
	}
	return frameLength;
}

/*
 * Compute length of ADTS header in bits
 */
static u_int16_t getAdtsHeaderBitSize(u_int8_t* hdr)
{
	u_int8_t isMpeg4 = hdr[1] & 0x08;
	u_int8_t hasCrc = !(hdr[1] & 0x01);
	u_int16_t hdrSize;

	if (isMpeg4) {
		hdrSize = 58;
	} else {
		hdrSize = 56;
	}
	if (hasCrc) {
		hdrSize += 16;
	}
	return hdrSize;
}

static u_int16_t getAdtsHeaderByteSize(u_int8_t* hdr)
{
	u_int16_t hdrBitSize = getAdtsHeaderBitSize(hdr);
	
	if ((hdrBitSize % 8) == 0) {
		return (hdrBitSize / 8);
	} else {
		return (hdrBitSize / 8) + 1;
	}
}

/* 
 * hdr must point to at least ADTS_HEADER_MAX_SIZE bytes of memory 
 */
static bool loadNextAdtsHeader(FILE* inFile, u_int8_t* hdr)
{
	u_int state = 0;
	u_int dropped = 0;
	u_int hdrByteSize = ADTS_HEADER_MAX_SIZE;

	while (1) {
		/* read a byte */
		u_int8_t b;

		if (fread(&b, 1, 1, inFile) == 0) {
			return FALSE;
		}

		/* header is complete, return it */
		if (state == hdrByteSize - 1) {
			hdr[state] = b;
			if (dropped > 0) {
				fprintf(stdout, "Warning: dropped %u input bytes\n", dropped);
			}
			return TRUE;
		}

		/* collect requisite number of bytes, no constraints on data */
		if (state >= 2) {
			hdr[state++] = b;
		} else {
			/* have first byte, check if we have 1111X00X */
			if (state == 1) {
				if ((b & 0xF6) == 0xF0) {
					hdr[state] = b;
					state = 2;
					/* compute desired header size */
					hdrByteSize = getAdtsHeaderByteSize(hdr);
				} else {
					state = 0;
				}
			}
			/* initial state, looking for 11111111 */
			if (state == 0) {
				if (b == 0xFF) {
					hdr[state] = b;
					state = 1;
				} else {
					 /* else drop it */ 
					dropped++;
				}
			}
		}
	}
}

/*
 * Load the next frame from the file
 * into the supplied buffer, which better be large enough!
 *
 * Note: Frames are padded to byte boundaries
 */
bool loadNextAacFrame(FILE* inFile, u_int8_t* pBuf, u_int* pBufSize, bool stripAdts)
{
	u_int16_t frameSize;
	u_int16_t hdrBitSize, hdrByteSize;
	u_int8_t hdrBuf[ADTS_HEADER_MAX_SIZE];

	/* get the next AAC frame header, more or less */
	if (!loadNextAdtsHeader(inFile, hdrBuf)) {
		return FALSE;
	}
	
	/* get frame size from header */
	frameSize = getAdtsFrameSize(hdrBuf);

	/* get header size in bits and bytes from header */
	hdrBitSize = getAdtsHeaderBitSize(hdrBuf);
	hdrByteSize = getAdtsHeaderByteSize(hdrBuf);
	
	/* adjust the frame size to what remains to be read */
	frameSize -= hdrByteSize;

	if (stripAdts) {
		if ((hdrBitSize % 8) == 0) {
			/* header is byte aligned, i.e. MPEG-2 ADTS */
			/* read the frame data into the buffer */
			if (fread(pBuf, 1, frameSize, inFile) != frameSize) {
				return FALSE;
			}
			(*pBufSize) = frameSize;
		} else {
			/* header is not byte aligned, i.e. MPEG-4 ADTS */
			int i;
			u_int8_t newByte;
			int upShift = hdrBitSize % 8;
			int downShift = 8 - upShift;

			pBuf[0] = hdrBuf[hdrBitSize / 8] << upShift;

			for (i = 0; i < frameSize; i++) {
				if (fread(&newByte, 1, 1, inFile) != 1) {
					return FALSE;
				}
				pBuf[i] |= (newByte >> downShift);
				pBuf[i+1] = (newByte << upShift);
			}
			(*pBufSize) = frameSize + 1;
		}
	} else { /* don't strip ADTS headers */
		memcpy(pBuf, hdrBuf, hdrByteSize);
		if (fread(&pBuf[hdrByteSize], 1, frameSize, inFile) != frameSize) {
			return FALSE;
		}
	}

	return TRUE;
}

static bool getFirstHeader(FILE* inFile)
{
	/* read file until we find an audio frame */
	fpos_t curPos;

	/* already read first header */
	if (firstHeader[0] == 0xff) {
		return;
	}

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* go back to start of file */
	rewind(inFile);

	if (!loadNextAdtsHeader(inFile, firstHeader)) {
		return FALSE;
	}

	/* reposition the file to where we were */
	fsetpos(inFile, &curPos);

	return TRUE;
}

bool getAacProfile(FILE* inFile, u_int* pProfile)
{
	if (!getFirstHeader(inFile)) {
		return FALSE;
	}
	(*pProfile) = (firstHeader[2] & 0xc0) >> 6;
	return TRUE;
}

static u_int32_t aacSamplingRates[16] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
	16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

bool getAacSamplingRate(FILE* inFile, u_int* pSamplingRate)
{
	if (!getFirstHeader(inFile)) {
		return FALSE;
	}
	(*pSamplingRate) = aacSamplingRates[(firstHeader[2] & 0x3c) >> 2];
	return TRUE;
}

bool getAacSamplingRateIndex(FILE* inFile, u_int* pSamplingRateIndex)
{
	if (!getFirstHeader(inFile)) {
		return FALSE;
	}
	(*pSamplingRateIndex) = (firstHeader[2] & 0x3c) >> 2;
	return TRUE;
}

bool getAacChannelConfiguration(FILE* inFile, u_int* pChannelConfig)
{
	if (!getFirstHeader(inFile)) {
		return FALSE;
	}
	(*pChannelConfig) = 
		((firstHeader[2] & 0x1) << 2) | ((firstHeader[3] & 0xc0) >> 6);
	return TRUE;
}

