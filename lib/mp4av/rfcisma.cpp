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

#include <mp4av.h>

void MP4AV_RfcIsmaConcatenator(
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
	MP4AddRtpPacket(mp4File, hintTrackId, true);

	u_int8_t payloadHeader[2];
	u_int16_t numHdrBits = 16 * samplesThisHint;
	payloadHeader[0] = numHdrBits >> 8;
	payloadHeader[1] = numHdrBits & 0xFF;

	MP4AddRtpImmediateData(mp4File, hintTrackId,
		(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

	u_int8_t i;

	// first the headers
	for (i = 0; i < samplesThisHint; i++) {
		MP4SampleId sampleId = pSampleIds[i];

		u_int32_t sampleSize = 
			MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

		// AU payload header is 13 bits of size
		// follow by 3 bits of the difference between sampleId's - 1
		payloadHeader[0] = sampleSize >> 5;
		payloadHeader[1] = (sampleSize & 0x1F) << 3;
		if (i > 0) {
			payloadHeader[1] |= ((sampleId - pSampleIds[i-1]) - 1); 
		}

		MP4AddRtpImmediateData(mp4File, hintTrackId,
			(u_int8_t*)&payloadHeader, sizeof(payloadHeader));
	}

	// then the samples
	for (i = 0; i < samplesThisHint; i++) {
		MP4SampleId sampleId = pSampleIds[i];

		u_int32_t sampleSize = 
			MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

		MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 0, sampleSize);
	}

	// write the hint
	MP4WriteRtpHint(mp4File, hintTrackId, hintDuration);
}

void MP4AV_RfcIsmaFragmenter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t sampleSize, 
	MP4Duration sampleDuration)
{
	MP4AddRtpHint(mp4File, hintTrackId);

	MP4AddRtpPacket(mp4File, hintTrackId, false);

	u_int8_t payloadHeader[4];
	payloadHeader[0] = 0;
	payloadHeader[1] = 16;
	payloadHeader[2] = sampleSize >> 5;
	payloadHeader[3] = (sampleSize & 0x1F) << 3;

	MP4AddRtpImmediateData(mp4File, hintTrackId,
		(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

	u_int16_t sampleOffset = 0;
	u_int16_t fragLength = MaxPayloadSize - 4;

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

	MP4WriteRtpHint(mp4File, hintTrackId, sampleDuration);
}

void MP4AV_RfcIsmaHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	bool interleave)
{
	u_int8_t payloadNumber = 0;

	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"mpeg4-generic", &payloadNumber, 0);

	/* get the aac configuration */
	u_int8_t* pConfig;
	u_int32_t configSize;

	MP4GetTrackESConfiguration(mp4File, mediaTrackId, &pConfig, &configSize);

	if (pConfig) {
		/* convert it into ASCII form */
		char* sConfig = MP4BinaryToBase16(pConfig, configSize);
		ASSERT(sConfig);

		/* create the appropriate SDP attribute */
		char* sdpBuf = (char*)malloc(strlen(sConfig) + 256);

		sprintf(sdpBuf,
			"a=fmtp:%u "
			"streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s; "
			"SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;"
			"\015\012",
				payloadNumber,
				sConfig); 

		/* add this to the track's sdp */
		MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);

		free(sConfig);
		free(sdpBuf);
	}

	// 500 ms max latency for ISMA profile 1
	MP4Duration maxLatency = 
		MP4GetTrackTimeScale(mp4File, hintTrackId) / 2;
	ASSERT(maxLatency);

	MP4Duration sampleDuration = 
		MP4GetSampleDuration(mp4File, mediaTrackId, 1);
	ASSERT(sampleDuration != MP4_INVALID_DURATION);

	u_int32_t samplesPerPacket = 0;
 
	if (interleave) {
		u_int32_t maxSampleSize =
			MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);

		// compute how many maximum size samples would fit in a packet
		samplesPerPacket = 
			(MaxPayloadSize - 2) / (maxSampleSize + 2);

		// can't interleave if this number is 0 or 1
		if (samplesPerPacket < 2) {
			interleave = false;
		}
	}

	if (interleave) {
		u_int32_t samplesPerGroup = maxLatency / sampleDuration;

		MP4AV_AudioInterleaveHinter(
			mp4File, 
			mediaTrackId, 
			hintTrackId,
			sampleDuration, 
			samplesPerGroup / samplesPerPacket,		// stride
			samplesPerPacket,						// bundle
			MP4AV_RfcIsmaConcatenator);

	} else {
		MP4AV_AudioConsecutiveHinter(
			mp4File, 
			mediaTrackId, 
			hintTrackId,
			sampleDuration, 
			2,										// perPacketHeaderSize
			2,										// perSampleHeaderSize
			maxLatency / sampleDuration,			// maxSamplesPerPacket
			MP4GetSampleSize,
			MP4AV_RfcIsmaConcatenator,
			MP4AV_RfcIsmaFragmenter);
	}
}

