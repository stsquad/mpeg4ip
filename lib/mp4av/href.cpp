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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May   wmay@cisco.com
 */

#include <mp4av_common.h>

extern "C" bool HrefHinter (MP4FileHandle mp4file, 
			    MP4TrackId trackid,
			    uint16_t maxPayloadSize)
{
  uint32_t numSamples;
  MP4SampleId sampleId;
  uint32_t sampleSize;
  MP4TrackId hintTrackId;
  uint8_t payload;

  numSamples = MP4GetTrackNumberOfSamples(mp4file, trackid);

  if (numSamples == 0) return false;


  hintTrackId = MP4AddHintTrack(mp4file, trackid);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  payload = MP4_SET_DYNAMIC_PAYLOAD;

  MP4SetHintTrackRtpPayload(mp4file, 
			    hintTrackId, 
			    "X-HREF", 
			    &payload, 
			    0,
			    NULL,
			    true, 
			    false);

  for (sampleId = 1; sampleId <= numSamples; sampleId++) {
    sampleSize = MP4GetSampleSize(mp4file, trackid, sampleId);
    MP4AddRtpHint(mp4file, hintTrackId);
    MP4AddRtpPacket(mp4file, hintTrackId, true);
    uint8_t payloadHeader[4];
    payloadHeader[0] = 0;
    payloadHeader[1] = 0;
    payloadHeader[2] = sampleSize >> 8;
    payloadHeader[3] = sampleSize & 0xff;

    MP4AddRtpImmediateData(mp4file, hintTrackId, payloadHeader, sizeof(payloadHeader));

    MP4AddRtpSampleData(mp4file, hintTrackId, sampleId, 0, sampleSize);
    MP4WriteRtpHint(mp4file, 
		    hintTrackId, 
		    MP4GetSampleDuration(mp4file, trackid, sampleId));
  }

  return true; // will never reach here
}
  
