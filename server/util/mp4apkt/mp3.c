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

static bool loadNextMp3Header(FILE* inFile, u_int32_t* pHdr, bool allowLayer4)
{
	u_int state = 0;
	u_int dropped = 0;
	u_char bytes[4];

	while (1) {
		/* read a byte */
		u_char b;

		if (fread(&b, 1, 1, inFile) == 0) {
			return FALSE;
		}

		if (state == 3) {
			bytes[state] = b;
			(*pHdr) = (bytes[0] << 24) | (bytes[1] << 16) 
				| (bytes[2] << 8) | bytes[3];
			if (dropped > 0) {
				fprintf(stdout, "Warning dropped %u input bytes\n", dropped);
			}
			return TRUE;
		}
		if (state == 2) {
			if ((b & 0xF0) == 0 || (b & 0xF0) == 0xF0 || (b & 0x0C) == 0x0C) {
				if (bytes[1] == 0xFF) {
					state = 1; 
				} else {
					state = 0; 
				}
			} else {
				bytes[state] = b;
				state = 3;
			}
		}
		if (state == 1) {
			if ((b & 0xE0) == 0xE0 && (b & 0x18) != 0x08 && 
			  ((b & 0x06) != 0 || allowLayer4)) {
				bytes[state] = b;
				state = 2;
			} else {
				state = 0;
			}
		}
		if (state == 0) {
			if (b == 0xFF) {
				bytes[state] = b;
				state = 1;
			} else {
				/* else drop it */ 
				dropped++;
			}
		}
	}
}

static u_int16_t mp3BitRates[5][14] = {
	/* MPEG-1, Layer III */
	{ 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
	/* MPEG-1, Layer II */
	{ 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
	/* MPEG-1, Layer I */
	{ 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
	/* MPEG-2 or 2.5, Layer II or III */
	{ 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
	/* MPEG-2 or 2.5, Layer I */
	{ 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 }
};

static u_int16_t mp3SampleRates[4][3] = {
	{ 11025, 12000, 8000 },		/* MPEG-2.5 */
	{ 0, 0, 0 },
	{ 22050, 24000, 16000 },	/* MPEG-2 */
	{ 44100, 48000, 32000 }		/* MPEG-1 */
};

static u_int16_t getMp3HdrSamplingRate(u_int32_t hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = (hdr >> 19) & 0x3; 
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;

	return mp3SampleRates[version][sampleRateIndex];
}

/*
 * compute MP3 frame size
 */
static u_int16_t getMp3FrameSize(u_int32_t hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = (hdr >> 19) & 0x3; 
	u_int8_t layer = (hdr >> 17) & 0x3; 
	u_int8_t bitRateIndex1;
	u_int8_t bitRateIndex2 = (hdr >> 12) & 0xF;
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;
	bool isProtected = (hdr >> 16) & 0x1;
	bool isPadded = (hdr >> 9) & 0x1;
	u_int16_t frameSize = 0;

	if (version == 3) {
		/* MPEG-1 */
		bitRateIndex1 = layer - 1;
	} else {
		/* MPEG-2 or MPEG-2.5 */
		if (layer == 3) {
			/* layer 1 */
			bitRateIndex1 = 4;
		} else {
			bitRateIndex1 = 3;
		}
	}

	/* compute frame size */
	frameSize = (144 * 1000 * mp3BitRates[bitRateIndex1][bitRateIndex2-1]) 
		/ mp3SampleRates[version][sampleRateIndex];

	if (isProtected) {
		frameSize += 2;		/* 2 byte checksum is present */
	}
	if (isPadded) {
		frameSize++;		/* 1 byte pad is present */
	}

	return frameSize;
}

/*
 * Load the next frame from the file
 * into the supplied buffer, which better be large enough!
 *
 * Note: Frames are padded to byte boundaries
 */
bool loadNextMp3Frame(FILE* inFile, u_char* pBuf, u_int* pBufSize)
{
	u_int32_t header;
	u_int16_t frameSize;

	/* get the next MP3 frame header */
	if (!loadNextMp3Header(inFile, &header, FALSE)) {
		return FALSE;
	}
	
	/* get frame size from header */
	frameSize = getMp3FrameSize(header);

	/* adjust for header which has already been read */
	frameSize -= 4 + 2;
	
	/* put the header in the buffer */
	pBuf[0] = (header >> 24) & 0xFF;
	pBuf[1] = (header >> 16) & 0xFF;
	pBuf[2] = (header >> 8) & 0xFF;
	pBuf[3] = header & 0xFF;
	(*pBufSize) = 4;

	/* read the frame data into the buffer */
	if (fread(&pBuf[*pBufSize], 1, frameSize, inFile) != frameSize) {
		return FALSE;
	}
	(*pBufSize) += frameSize;

	return TRUE;
}

bool getMp3SamplingRate(FILE* inFile, u_int* pSamplingRate)
{
	/* read file until we find an audio frame */
	fpos_t curPos;
	u_int32_t header;

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* get the next MP3 frame header */
	if (!loadNextMp3Header(inFile, &header, FALSE)) {
		return FALSE;
	}

	(*pSamplingRate) = getMp3HdrSamplingRate(header);

	/* rewind the file to where we were */
	fsetpos(inFile, &curPos);

	return TRUE;
}

/*
 * Get MPEG layer from MP3 header
 * if it's really MP3, it should be layer 3, value 01
 * if it's really ADTS, it should be layer 4, value 00
 */
bool getMpegLayer(FILE* inFile, u_int* pLayer)
{
	/* read file until we find an audio frame */
	fpos_t curPos;
	u_int32_t header;

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* get the next MP3 frame header */
	if (!loadNextMp3Header(inFile, &header, TRUE)) {
		return FALSE;
	}

	(*pLayer) = (header >> 17) & 0x3; 

	/* rewind the file to where we were */
	fsetpos(inFile, &curPos);

	return TRUE;
}

