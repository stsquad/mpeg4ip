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
#include <mp3.h>

static u_int16_t* GetAduOffsets(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId)
{
	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	u_int16_t* pAduOffsets = 
		(u_int16_t*)calloc((numSamples + 2), sizeof(u_int16_t));
	ASSERT(pAduOffsets);

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) { 
		u_int8_t* pSample = NULL;
		u_int32_t sampleSize = 0;

		MP4ReadSample(
			mp4File,
			mediaTrackId,
			sampleId,
			&pSample,
			&sampleSize);

		pAduOffsets[sampleId] = 
			Mp3GetAduOffset(pSample, sampleSize);

printf("sample %u size %u aduOffset %u\n", 
sampleId, sampleSize, pAduOffsets[sampleId]);

		free(pSample);
	}

	return pAduOffsets;
}

static u_int16_t GetAduSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId, 
	u_int16_t* pAduOffsets)
{
	u_int32_t sampleSize = MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

	return pAduOffsets[sampleId] + sampleSize - pAduOffsets[sampleId+1];
}

static u_int16_t GetMaxAduSize(
	MP4FileHandle mp4File, MP4TrackId mediaTrackId, u_int16_t* pAduOffsets)
{
	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	u_int16_t maxAduSize = 0;

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) { 
		u_int16_t aduSize =
			GetAduSize(mp4File, mediaTrackId, sampleId, pAduOffsets);

printf("sample %u aduSize %u\n",
sampleId, aduSize);
		if (aduSize > maxAduSize) {
			maxAduSize = aduSize;
		}
	}

	return maxAduSize;
}

#ifdef NOTDEF
	"Build ADU"
	packetize with ADU header (immed)
	adu data sample data 
		current mp3 frame 
		previous mp3 frame(s)
		remainder of current mp3 frame up to next frames back pointer
#endif

void Rfc3119Hinter(
	MP4FileHandle mp4File, MP4TrackId mediaTrackId, MP4TrackId hintTrackId,
	bool interleave)
{
printf("rfc 3119 support is under development\n"); 
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

	u_int16_t* pAduOffsets =
		GetAduOffsets(mp4File, mediaTrackId);

	u_int32_t samplesPerPacket = 0;
 
	if (interleave) {
		u_int32_t maxAduSize =
			GetMaxAduSize(mp4File, mediaTrackId, pAduOffsets);

		// compute how many maximum size samples would fit in a packet
		samplesPerPacket = 
			MaxPayloadSize / (maxAduSize + 2);

		// can't interleave if this number is 0 or 1
		if (samplesPerPacket < 2) {
			interleave = false;
		}
	}

#ifdef NOTDEF
	if (interleave) {
		u_int32_t samplesPerGroup = maxLatency / sampleDuration;

		InterleaveHinter(mp4File, mediaTrackId, hintTrackId,
			sampleDuration, 
			samplesPerGroup / samplesPerPacket,		// stride
			samplesPerPacket);						// bundle
	} else {
		ConsecutiveHinter(mp4File, mediaTrackId, hintTrackId,
			 sampleDuration);
	}
#endif
}

