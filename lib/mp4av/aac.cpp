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

#define ADTS_HEADER_MAX_SIZE 10 /* bytes */

#define NUM_AAC_SAMPLING_RATES	16

static u_int32_t AacSamplingRates[NUM_AAC_SAMPLING_RATES] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
	16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

/*
 * compute ADTS frame size
 */
extern "C" u_int16_t MP4AV_AacAdtsGetFrameSize(u_int8_t* pHdr)
{
	/* extract the necessary fields from the header */
	u_int8_t isMpeg4 = !(pHdr[1] & 0x08);
	u_int16_t frameLength;

	if (isMpeg4) {
		frameLength = (((u_int16_t)pHdr[4]) << 5) | (pHdr[5] >> 3); 
	} else { /* MPEG-2 */
		frameLength = (((u_int16_t)(pHdr[3] & 0x3)) << 11) 
			| (((u_int16_t)pHdr[4]) << 3) | (pHdr[5] >> 5); 
	}
	return frameLength;
}

/*
 * Compute length of ADTS header in bits
 */
extern "C" u_int16_t MP4AV_AacAdtsGetHeaderBitSize(u_int8_t* pHdr)
{
	u_int8_t isMpeg4 = !(pHdr[1] & 0x08);
	u_int8_t hasCrc = !(pHdr[1] & 0x01);
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

extern "C" u_int16_t MP4AV_AacAdtsGetHeaderByteSize(u_int8_t* pHdr)
{
	u_int16_t hdrBitSize = MP4AV_AacAdtsGetHeaderBitSize(pHdr);
	
	if ((hdrBitSize % 8) == 0) {
		return (hdrBitSize / 8);
	} else {
		return (hdrBitSize / 8) + 1;
	}
}

extern "C" u_int8_t MP4AV_AacAdtsGetVersion(u_int8_t* pHdr)
{
	return (pHdr[1] & 0x08) >> 3;
}

extern "C" u_int8_t MP4AV_AacAdtsGetProfile(u_int8_t* pHdr)
{
	return (pHdr[2] & 0xc0) >> 6;
}

extern "C" u_int8_t MP4AV_AacAdtsGetSamplingRateIndex(u_int8_t* pHdr)
{
	return (pHdr[2] & 0x3c) >> 2;
}

extern "C" u_int8_t MP4AV_AacGetSamplingRateIndex(u_int32_t samplingRate)
{
	for (u_int8_t i = 0; i < NUM_AAC_SAMPLING_RATES; i++) {
		if (samplingRate == AacSamplingRates[i]) {
			return i;
		}
	}
	return NUM_AAC_SAMPLING_RATES - 1;
}

extern "C" u_int32_t MP4AV_AacAdtsGetSamplingRate(u_int8_t* pHdr)
{
	return AacSamplingRates[MP4AV_AacAdtsGetSamplingRateIndex(pHdr)];
}

extern "C" u_int8_t MP4AV_AacAdtsGetChannels(u_int8_t* pHdr)
{
	return ((pHdr[2] & 0x1) << 2) | ((pHdr[3] & 0xc0) >> 6);
}

/*
 * AAC Config in ES:
 *
 * AudioObjectType 			5 bits
 * samplingFrequencyIndex 	4 bits
 * if (samplingFrequencyIndex == 0xF)
 *	samplingFrequency	24 bits 
 * channelConfiguration 	4 bits
 * GA_SpecificConfig
 * 	FrameLengthFlag 		1 bit 1024 or 960
 * 	DependsOnCoreCoder		1 bit (always 0)
 * 	ExtensionFlag 			1 bit (always 0)
 */

extern "C" u_int8_t MP4AV_AacConfigGetSamplingRateIndex(u_int8_t* pConfig)
{
	return ((pConfig[0] << 1) | (pConfig[1] >> 7)) & 0xF;
}

extern "C" u_int32_t MP4AV_AacConfigGetSamplingRate(u_int8_t* pConfig)
{
	u_int8_t index =
		MP4AV_AacConfigGetSamplingRateIndex(pConfig);

	if (index == 0xF) {
		return (pConfig[1] & 0x7F) << 17
			| pConfig[2] << 9
			| pConfig[3] << 1
			| (pConfig[4] >> 7);
	}
	return AacSamplingRates[index];
}

extern "C" u_int16_t MP4AV_AacConfigGetSamplingWindow(u_int8_t* pConfig)
{
	u_int8_t adjust = 0;

	if (MP4AV_AacConfigGetSamplingRateIndex(pConfig) == 0xF) {
		adjust = 3;
	}

	if ((pConfig[1 + adjust] >> 2) & 0x1) {
		return 960;
	}
	return 1024;
}

extern "C" u_int8_t MP4AV_AacConfigGetChannels(u_int8_t* pConfig)
{
	u_int8_t adjust = 0;

	if (MP4AV_AacConfigGetSamplingRateIndex(pConfig) == 0xF) {
		adjust = 3;
	}
	return (pConfig[1 + adjust] >> 3) & 0xF;
}

extern "C" bool MP4AV_AacGetConfigurationHdr(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t* pHdr)
{
	return MP4AV_AacGetConfiguration(
		ppConfig,
		pConfigLength,
		MP4AV_AacAdtsGetProfile(pHdr),
		MP4AV_AacAdtsGetSamplingRate(pHdr),
		MP4AV_AacAdtsGetChannels(pHdr));
}

extern "C" bool MP4AV_AacGetConfiguration(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t profile,
	u_int32_t samplingRate,
	u_int8_t channels)
{
	/* create the appropriate decoder config */

	u_int8_t* pConfig = (u_int8_t*)malloc(2);

	if (pConfig == NULL) {
		return false;
	}


	u_int8_t samplingRateIndex = 
		MP4AV_AacGetSamplingRateIndex(samplingRate);

	pConfig[0] =
		((profile + 1) << 3) | ((samplingRateIndex & 0xe) >> 1);
	pConfig[1] =
		((samplingRateIndex & 0x1) << 7) | (channels << 3);

	/* LATER this option is not currently used in MPEG4IP
	if (samplesPerFrame == 960) {
		pConfig[1] |= (1 << 2);
	}
	*/

	*ppConfig = pConfig;
	*pConfigLength = 2;

	return true;
}

