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

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4av_common.h>

static u_int16_t Mp3BitRates[5][14] = {
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

static u_int16_t Mp3SampleRates[4][3] = {
	{ 11025, 12000, 8000 },		/* MPEG-2.5 */
	{ 0, 0, 0 },
	{ 22050, 24000, 16000 },	/* MPEG-2 */
	{ 44100, 48000, 32000 }		/* MPEG-1 */
};

u_int8_t MP4AV_Mp3GetHdrVersion(MP4AV_Mp3Header hdr)
{
	/* extract the necessary field from the MP3 header */
	return ((hdr >> 19) & 0x3); 
}

u_int8_t MP4AV_Mp3GetHdrLayer(MP4AV_Mp3Header hdr)
{
	/* extract the necessary field from the MP3 header */
	return ((hdr >> 17) & 0x3); 
}

u_int16_t MP4AV_Mp3GetHdrSamplingRate(MP4AV_Mp3Header hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = MP4AV_Mp3GetHdrVersion(hdr);
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;

	return Mp3SampleRates[version][sampleRateIndex];
}

u_int16_t MP4AV_Mp3GetHdrSamplingWindow(MP4AV_Mp3Header hdr)
{
	u_int8_t version = MP4AV_Mp3GetHdrVersion(hdr);
	u_int8_t layer = MP4AV_Mp3GetHdrLayer(hdr);
	u_int16_t samplingWindow;

	if (layer == 1) {
		if (version == 3) {
			samplingWindow = 1152;
		} else {
			samplingWindow = 576;
		}
	} else if (layer == 2) {
		samplingWindow = 1152;
	} else {
		samplingWindow = 384;
	}

	return samplingWindow;
}

/*
 * compute MP3 frame size
 */
u_int16_t MP4AV_Mp3GetFrameSize(MP4AV_Mp3Header hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = MP4AV_Mp3GetHdrVersion(hdr);
	u_int8_t layer = MP4AV_Mp3GetHdrLayer(hdr);
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
	frameSize = (144 * 1000 * Mp3BitRates[bitRateIndex1][bitRateIndex2-1]) 
		/ (Mp3SampleRates[version][sampleRateIndex] << !(version & 1));

	if (isProtected) {
		frameSize += 2;		/* 2 byte checksum is present */
	}
	if (isPadded) {
		frameSize++;		/* 1 byte pad is present */
	}

	return frameSize;
}

u_int16_t MP4AV_Mp3GetAduOffset(u_int8_t* pFrame, u_int32_t frameSize)
{
	if (frameSize < 2) {
		return 0;
	}

	u_int8_t version = (pFrame[1] >> 3) & 0x3;
	u_int8_t layer = (pFrame[1] >> 1) & 0x3; 
	bool isProtected = !(pFrame[1] & 0x1);
	u_int8_t crcSize = isProtected ? 2 : 0;

	// protect against garbage input
	if (frameSize < (u_int32_t)(5 + crcSize + (version == 3 ? 1 : 0))) {
		return 0;
	}
	if (layer != 1) {
		return 0;
	}

	if (version == 3) {
		// MPEG-1
		return (pFrame[4 + crcSize] << 1) | (pFrame[5 + crcSize] >> 7);
	} else {
		// MPEG-2 or 2.5
		return pFrame[4 + crcSize];
	}
}

u_int8_t MP4AV_Mp3GetCrcSize(MP4AV_Mp3Header hdr)
{
	return ((hdr & 0x00010000) ? 0 : 2);
}

u_int8_t MP4AV_Mp3GetSideInfoSize(MP4AV_Mp3Header hdr)
{
	u_int8_t version = MP4AV_Mp3GetHdrVersion(hdr);
	u_int8_t layer = MP4AV_Mp3GetHdrLayer(hdr);
	u_int8_t mode = (hdr >> 6) & 0x3;
	u_int8_t channels = (mode == 3 ? 1 : 2);

	// check that this is layer 3
	if (layer != 1) {
		return 0;
	}

	if (version == 3) {
		// MPEG-1
		if (channels == 1) {
			return 17;
		} else { 
			return 32;
		}
	} else {
		// MPEG-2 or 2.5
		if (channels == 1) {
			return 9;
		} else { 
			return 17;
		}
	}
}

u_int8_t MP4AV_Mp3ToMp4AudioType(u_int8_t mpegVersion)
{
	u_int8_t audioType = MP4_INVALID_AUDIO_TYPE;

	switch (mpegVersion) {
	case 3:
		audioType = MP4_MPEG1_AUDIO_TYPE;
		break;
	case 2:
	case 0:
		audioType = MP4_MPEG2_AUDIO_TYPE;
		break;
	case 1:
		break;
	default:
		ASSERT(false);
	}
	return audioType;
}

