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

void Rfc2250Hinter(
	MP4FileHandle mp4File, MP4TrackId mediaTrackId, MP4TrackId hintTrackId)
{
	u_int8_t payloadNumber = 0; // use dynamic payload number

	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"MPA", &payloadNumber, 0);

	u_int32_t numSamples = 
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	MP4Duration sampleDuration = 
		MP4GetTrackFixedSampleDuration(mp4File, mediaTrackId);
	ASSERT(sampleDuration != MP4_INVALID_DURATION);

	u_int16_t bytesThisHint = 0;
	u_int16_t samplesThisHint = 0;

	MP4AddRtpHint(mp4File, hintTrackId);
	MP4AddRtpPacket(mp4File, hintTrackId, true);

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
		u_int32_t sampleSize = 
			MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

		if (samplesThisHint > 0) {
			if (bytesThisHint + sampleSize <= MaxPayloadSize) {
				// add the mp3 frame to current hint
				MP4AddRtpSampleData(mp4File, hintTrackId,
					sampleId, 0, sampleSize);

				samplesThisHint++;
				bytesThisHint += sampleSize;
				continue;
			} else {
				// write out current hint
				MP4WriteRtpHint(mp4File, hintTrackId, 
					samplesThisHint * sampleDuration);

				// start a new hint 
				samplesThisHint = 0;
				bytesThisHint = 0;

				MP4AddRtpHint(mp4File, hintTrackId);
				MP4AddRtpPacket(mp4File, hintTrackId, true);

				// fall thru
			}
		}

		ASSERT(samplesThisHint == 0);
		
		if (sampleSize + 4 <= MaxPayloadSize) {
			// add rfc 2250 payload header
			static u_int32_t zero32 = 0;

			MP4AddRtpImmediateData(mp4File, hintTrackId,
				(u_int8_t*)&zero32, sizeof(zero32));

			// add the mp3 frame to current hint
			MP4AddRtpSampleData(mp4File, hintTrackId,
				sampleId, 0, sampleSize);

			bytesThisHint += (4 + sampleSize);
		} else {
			// jumbo frame, need to fragment it
			u_int16_t sampleOffset = 0;

			while (sampleOffset < sampleSize) {
				u_int16_t fragLength = 
					MIN(sampleSize - sampleOffset, MaxPayloadSize) - 4;

				u_int8_t payloadHeader[4];
				payloadHeader[0] = payloadHeader[1] = 0;
				payloadHeader[2] = (sampleOffset >> 8);
				payloadHeader[3] = sampleOffset & 0xFF;

				MP4AddRtpImmediateData(mp4File, hintTrackId,
					(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

				MP4AddRtpSampleData(mp4File, hintTrackId,
					sampleId, sampleOffset, fragLength);

				sampleOffset += fragLength;

				// if we're not at the last fragment
				if (sampleOffset < sampleSize) {
					MP4AddRtpPacket(mp4File, hintTrackId, false);
				}
			}

			// lie to ourselves so as to force next frame to output 
			// our hint as is, and start a new hint for itself
			bytesThisHint = MaxPayloadSize;
		}

		samplesThisHint = 1;
	}

	// write out current (final) hint
	MP4WriteRtpHint(mp4File, hintTrackId, 
		samplesThisHint * sampleDuration);
}

