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

#include "mp4live.h"
#include "mp3.h"

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

static u_int16_t Mp3GetHdrSamplingRate(u_int32_t hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = (hdr >> 19) & 0x3; 
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;

	return mp3SampleRates[version][sampleRateIndex];
}

static u_int16_t Mp3GetHdrSamplingWindow(u_int32_t hdr)
{
	u_int8_t version = (hdr >> 19) & 0x3; 
	u_int8_t layer = (hdr >> 17) & 0x3; 
	u_int16_t samplingWindow;

	if (layer == 1) {
		if (version == 3) {
			samplingWindow = MP3_SAMPLES_PER_FRAME;
		} else {
			samplingWindow = MP3_SAMPLES_PER_FRAME / 2;
		}
	} else if (layer == 2) {
		samplingWindow = MP3_SAMPLES_PER_FRAME;
	} else {
		samplingWindow = MP3_SAMPLES_PER_FRAME / 3;
	}

	return samplingWindow;
}

/*
 * compute MP3 frame size
 */
static u_int16_t Mp3GetFrameSize(u_int32_t hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = (hdr >> 19) & 0x3; 
	u_int8_t layer = (hdr >> 17) & 0x3; 
	u_int8_t bitRateIndex1;
	u_int8_t bitRateIndex2 = (hdr >> 12) & 0xF;
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;
	bool isProtected = !((hdr >> 16) & 0x1);
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
		/ (mp3SampleRates[version][sampleRateIndex] << !(version & 1));

	if (isProtected) {
		frameSize += 2;		/* 2 byte checksum is present */
	}
	if (isPadded) {
		frameSize++;		/* 1 byte pad is present */
	}

	return frameSize;
}

bool Mp3FindNextFrame(u_int8_t* src, u_int32_t srcLength,
	u_int8_t** frame, u_int32_t* frameSize, bool allowLayer4)
{
	u_int state = 0;
	u_int dropped = 0;
	u_char bytes[4];
	u_int32_t srcPos = 0;

	while (true) {
		/* read a byte */
		if (srcPos >= srcLength) {
			return false;
		}
		u_char b = src[srcPos++];

		if (state == 3) {
			bytes[state] = b;
			if (dropped > 0) {
				debug_message("Warning dropped %u input bytes\n", dropped);
			}
			*frame = src + dropped;
			u_int32_t header = (bytes[0] << 24) | (bytes[1] << 16) 
				| (bytes[2] << 8) | bytes[3];
			*frameSize = Mp3GetFrameSize(header);
			return true;
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
				if (dropped == 0 && 
				  ((b & 0xE0) == 0xE0 && 
				  (b & 0x18) != 0x08 && 
			  	  ((b & 0x06) != 0 || allowLayer4))) {
					/*
					 * HACK have seen files where previous frame 
					 * was marked as padded, but the byte was never added
					 * which results in the next frame's leading 0XFF being
					 * eaten. We attempt to repair that situation here.
					 */
					bytes[0] = 0xFF;
					bytes[1] = b;
					state = 2;
				} else {
					/* else drop it */ 
					dropped++;
				}
			}
		}
	}
}

