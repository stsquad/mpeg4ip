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

void RfcIsmaHinter(
	MP4FileHandle mp4File, MP4TrackId mediaTrackId, MP4TrackId hintTrackId,
	bool interleave)
{
	u_int8_t payloadNumber = 0;

	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"mpeg4-generic", &payloadNumber, 0);

	// TBD a=fmtp line

	u_int32_t numSamples = 
		MP4GetNumberOfTrackSamples(mp4File, mediaTrackId);

	MP4Duration sampleDuration = 
		MP4GetTrackFixedSampleDuration(mp4File, mediaTrackId);
	ASSERT(sampleDuration);

	u_int16_t bytesThisHint = 2;
	u_int16_t samplesThisHint = 0;

	if (!interleave) {
	MP4SampleId startSampleId = 1;

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
		u_int32_t sampleSize = 
			MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

		// sample won't fit in this packet
		if (sampleSize + 2 > MaxPayloadSize - bytesThisHint) {

			// construct and write new hint
			MP4AddRtpHint(mp4File, hintTrackId);
			MP4AddRtpPacket(mp4File, hintTrackId, true);

			u_int8_t payloadHeader[2];
			u_int16_t numHdrBits = 16 * samplesThisHint;
			payloadHeader[0] = numHdrBits >> 8;
			payloadHeader[1] = numHdrBits & 0xFF;

			MP4AddRtpImmediateData(mp4File, hintTrackId,
				(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

			// first the headers
			MP4SampleId sid;
			for (sid = startSampleId; sid < sampleId; sid++) {
				u_int32_t size = 
					MP4GetSampleSize(mp4File, mediaTrackId, sid);
				payloadHeader[0] = size >> 8;
				payloadHeader[1] = size & 0xFF;

				MP4AddRtpImmediateData(mp4File, hintTrackId,
					(u_int8_t*)&payloadHeader, sizeof(payloadHeader));
			}

			// then the samples
			for (sid = startSampleId; sid < sampleId; sid++) {
				u_int32_t size = 
					MP4GetSampleSize(mp4File, mediaTrackId, sid);
				MP4AddRtpSampleData(mp4File, hintTrackId, sid, 0, size);
			}

			MP4WriteRtpHint(mp4File, hintTrackId, 
				samplesThisHint * sampleDuration);

			// start a new hint 
			samplesThisHint = 0;
			bytesThisHint = 2;
			startSampleId = sampleId;

			// fall thru
		}

		// sample is less than remaining payload size
		if (sampleSize + 2 <= MaxPayloadSize - bytesThisHint) {
			bytesThisHint += (sampleSize + 2);
			samplesThisHint++;

		} else { // jumbo frame, need to fragment it

			MP4AddRtpHint(mp4File, hintTrackId);

			MP4AddRtpPacket(mp4File, hintTrackId, false);

			u_int8_t payloadHeader[4];
			payloadHeader[0] = 0;
			payloadHeader[1] = 16;
			payloadHeader[2] = sampleSize >> 8;
			payloadHeader[3] = sampleSize & 0xFF;

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

			// start a new hint 
			samplesThisHint = 0;
			bytesThisHint = 2;
			startSampleId = sampleId + 1;
		}
	}
	} else { // interleave
	}
}

