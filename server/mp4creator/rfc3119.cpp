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

#include <mp4creator.h>
#include <hinters.h>
#include <mp3.h>

// file globals
static bool doInterleave;
static u_int32_t samplesPerPacket;
static u_int32_t samplesPerGroup;
static u_int32_t* pFrameHeaders = NULL;
static u_int8_t* pSideInfoSizes = NULL;
static u_int16_t* pAduOffsets = NULL;

static void GetFrameInfo(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId,
	u_int32_t** ppFrameHeaders,
	u_int8_t** ppSideInfoSizes,
	u_int16_t** ppAduOffsets)
{
	// allocate memory to hold the frame info that we need
	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	if (ppFrameHeaders) {
		*ppFrameHeaders = 
			(u_int32_t*)calloc((numSamples + 2), sizeof(u_int32_t));
		ASSERT(*ppFrameHeaders);
	};

	*ppSideInfoSizes = 
		(u_int8_t*)calloc((numSamples + 2), sizeof(u_int8_t));
	ASSERT(*ppSideInfoSizes);

	*ppAduOffsets = 
		(u_int16_t*)calloc((numSamples + 2), sizeof(u_int16_t));
	ASSERT(*ppAduOffsets);

	// for each sample
	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) { 
		u_int8_t* pSample = NULL;
		u_int32_t sampleSize = 0;

		// read it
		MP4ReadSample(
			mp4File,
			mediaTrackId,
			sampleId,
			&pSample,
			&sampleSize);

		// extract the MP3 frame header
		u_int32_t mp3hdr = (pSample[0] << 24) | (pSample[1] << 16)
			| (pSample[2] << 8) | pSample[3];

		// store what we need
		if (ppFrameHeaders) {
			(*ppFrameHeaders)[sampleId] = mp3hdr;
		}

		(*ppSideInfoSizes)[sampleId] =
			Mp3GetSideInfoSize(mp3hdr);

		(*ppAduOffsets)[sampleId] = 
			Mp3GetAduOffset(pSample, sampleSize);

		free(pSample);
	}
}

static u_int16_t GetFrameHeaderSize(MP4SampleId sampleId)
{
	return 4 + pSideInfoSizes[sampleId];
}

static u_int16_t GetFrameDataSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId)
{
	return MP4GetSampleSize(mp4File, mediaTrackId, sampleId)
		- GetFrameHeaderSize(sampleId);
}

u_int32_t Rfc3119GetAduSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId)
{
	u_int32_t sampleSize = MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

	return pAduOffsets[sampleId] + sampleSize - pAduOffsets[sampleId + 1];
}

static u_int16_t GetMaxAduSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId)
{
	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	u_int16_t maxAduSize = 0;

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) { 
		u_int16_t aduSize =
			Rfc3119GetAduSize(mp4File, mediaTrackId, sampleId);

		if (aduSize > maxAduSize) {
			maxAduSize = aduSize;
		}
	}

	return maxAduSize;
}

static u_int16_t GetAduDataSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId)
{
	return Rfc3119GetAduSize(mp4File, mediaTrackId, sampleId) 
		- GetFrameHeaderSize(sampleId);
}

static void AddAdu(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId)
{
	// when interleaving we replace the 11 bit mp3 frame sync
	if (doInterleave) {
		// compute interleave index and interleave cycle from sampleId
		u_int8_t interleaveIndex =
			(sampleId - 1) % samplesPerGroup;
		u_int8_t interleaveCycle =
			((sampleId - 1) / samplesPerGroup) & 0x7;

		ASSERT(pFrameHeaders);
		u_int8_t interleaveHeader[4];
		interleaveHeader[0] = 
			interleaveIndex;
		interleaveHeader[1] = 
			(interleaveCycle << 5) | ((pFrameHeaders[sampleId] >> 16) & 0x1F);
		interleaveHeader[2] = 
			(pFrameHeaders[sampleId] >> 8) & 0xFF;
		interleaveHeader[3] = 
			pFrameHeaders[sampleId] & 0xFF;

		MP4AddRtpImmediateData(mp4File, hintTrackId,
			interleaveHeader, 4);

		// add side info from current mp3 frame
		MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId, 4, pSideInfoSizes[sampleId]);
	} else {
		// add mp3 header and side info from current mp3 frame
		MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId, 0, GetFrameHeaderSize(sampleId));
	}

	// now go back from sampleId until 
	// accumulated data bytes can fill sample's ADU

	MP4SampleId sid = sampleId;
	u_int8_t numSamples = 1;
	u_int32_t prevDataBytes = 0;
	const u_int8_t maxSamples = 8;
	u_int32_t offsets[maxSamples];
	u_int32_t sizes[maxSamples];

	offsets[0] = GetFrameHeaderSize(sampleId);
	sizes[0] = GetFrameDataSize(mp4File, mediaTrackId, sid);

	while (true) {
		if (prevDataBytes >= pAduOffsets[sampleId]) {
			u_int32_t adjust =
				prevDataBytes - pAduOffsets[sampleId];

			offsets[numSamples-1] += adjust; 
			sizes[numSamples-1] -= adjust;

			break;
		}
		
		sid--;
		numSamples++;

		if (sid == 0 || numSamples > maxSamples) {
			throw;	// media bitstream error
		}

		offsets[numSamples-1] = GetFrameHeaderSize(sid);
		sizes[numSamples-1] = GetFrameDataSize(mp4File, mediaTrackId, sid);
		prevDataBytes += sizes[numSamples-1]; 
	}

	// now go forward, collecting the needed blocks of data
	u_int16_t dataSize = 0;
	u_int16_t aduDataSize = GetAduDataSize(mp4File, mediaTrackId, sampleId);

	for (int8_t i = numSamples - 1; i >= 0 && dataSize < aduDataSize; i--) {
		u_int32_t blockSize = sizes[i];

		if ((u_int32_t)(aduDataSize - dataSize) < blockSize) {
			blockSize = (u_int32_t)(aduDataSize - dataSize);
		}

		MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId - i, offsets[i], blockSize);

		dataSize += blockSize;
	}
}

void Rfc3119Concatenator(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration)
{
	// handle degenerate case
	if (samplesThisHint == 0) {
		return;
	}

	// construct the new hint
	MP4AddRtpHint(mp4File, hintTrackId);
	MP4AddRtpPacket(mp4File, hintTrackId, false);

	// rfc 3119 per adu payload header
	u_int8_t payloadHeader[2];

	// for each given sample
	for (u_int8_t i = 0; i < samplesThisHint; i++) {
		MP4SampleId sampleId = pSampleIds[i];

		u_int16_t aduSize = 
			Rfc3119GetAduSize(mp4File, mediaTrackId, sampleId);

		payloadHeader[0] = 0x40 | ((aduSize >> 8) & 0x3F);
		payloadHeader[1] = aduSize & 0xFF;

		MP4AddRtpImmediateData(mp4File, hintTrackId,
			(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

		AddAdu(mp4File, mediaTrackId, hintTrackId, sampleId);
	}

	// write the hint
	MP4WriteRtpHint(mp4File, hintTrackId, hintDuration);
}

void Rfc3119Fragmenter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t aduSize, 
	MP4Duration sampleDuration)
{
	printf("Error: Fragmentation not support for this payload yet\n");
	throw;

#ifdef NOTDEF
	MP4AddRtpHint(mp4File, hintTrackId);
	MP4AddRtpPacket(mp4File, hintTrackId, false);

	// rfc 3119 payload header
	u_int8_t payloadHeader[2];

	payloadHeader[0] = 0x40 | ((aduSize >> 8) & 0x3F);
	payloadHeader[1] = aduSize & 0xFF;

	MP4AddRtpImmediateData(mp4File, hintTrackId,
		(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

	payloadHeader[0] |= 0x80;	// mark future packets as continuations

	u_int16_t sampleOffset = 0;
	u_int16_t fragLength = MaxPayloadSize - 2;

	do {
		MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId, sampleOffset, fragLength);

		sampleOffset += fragLength;

		if (sampleSize - sampleOffset > MaxPayloadSize) {
			fragLength = MaxPayloadSize; 
			MP4AddRtpPacket(mp4File, hintTrackId, false);
		} else {
			fragLength = sampleSize - sampleOffset; 
			if (fragLength) {
				MP4AddRtpPacket(mp4File, hintTrackId, true);
			}
		}
	} while (sampleOffset < sampleSize);

	// write the hint
	MP4WriteRtpHint(mp4File, hintTrackId, sampleDuration);
#endif
}

void Rfc3119Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	bool interleave)
{
	doInterleave = interleave;

	u_int8_t payloadNumber = 0;

	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"mpa-robust", &payloadNumber, 0);

	// choose 500 ms max latency
	MP4Duration maxLatency = 
		MP4GetTrackTimeScale(mp4File, hintTrackId) / 2;
	ASSERT(maxLatency);

	MP4Duration sampleDuration = 
		MP4GetSampleDuration(mp4File, mediaTrackId, 1);
	ASSERT(sampleDuration != MP4_INVALID_DURATION);

	// load mp3 frame information into memory
	GetFrameInfo(
		mp4File, 
		mediaTrackId, 
		(doInterleave ? &pFrameHeaders : NULL), 
		&pSideInfoSizes, 
		&pAduOffsets);
 
	if (doInterleave) {
		u_int32_t maxAduSize =
			GetMaxAduSize(mp4File, mediaTrackId);

		// compute how many maximum size samples would fit in a packet
		samplesPerPacket = 
			MaxPayloadSize / (maxAduSize + 2);

		// can't interleave if this number is 0 or 1
		if (samplesPerPacket < 2) {
			doInterleave = false;
		}
	}

	if (doInterleave) {
		samplesPerGroup = maxLatency / sampleDuration;

		AudioInterleaveHinter(
			mp4File, 
			mediaTrackId, 
			hintTrackId,
			sampleDuration, 
			samplesPerGroup / samplesPerPacket,		// stride
			samplesPerPacket,						// bundle
			Rfc3119Concatenator);

	} else {
		AudioConsecutiveHinter(
			mp4File, 
			mediaTrackId, 
			hintTrackId,
			sampleDuration, 
			0,										// perPacketHeaderSize
			2,										// perSampleHeaderSize
			maxLatency / sampleDuration,			// maxSamplesPerPacket
			Rfc3119GetAduSize,
			Rfc3119Concatenator,
			Rfc3119Fragmenter);
	}

	// cleanup
	free(pFrameHeaders);
	pFrameHeaders = NULL;
	free(pSideInfoSizes);
	pSideInfoSizes = NULL;
	free(pAduOffsets);
	pAduOffsets = NULL;
}

