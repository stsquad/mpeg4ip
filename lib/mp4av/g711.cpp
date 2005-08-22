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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May   wmay@cisco.com
 */

#include <mp4av_common.h>

//#define DEBUG_G711 1
extern "C" bool G711Hinter (MP4FileHandle mp4file, 
			    MP4TrackId trackid,
			    uint16_t maxPayloadSize)
{
  uint32_t numSamples;
  uint8_t audioType;
  MP4SampleId sampleId;
  uint32_t sampleSize;
  MP4TrackId hintTrackId;
  uint8_t payload;
  uint32_t bytes_this_hint;
  uint32_t sampleOffset;

  numSamples = MP4GetTrackNumberOfSamples(mp4file, trackid);

  if (numSamples == 0) return false;

  audioType = MP4GetTrackEsdsObjectTypeId(mp4file, trackid);

  if (audioType != MP4_ALAW_AUDIO_TYPE &&
      audioType != MP4_ULAW_AUDIO_TYPE) return false;

  hintTrackId = MP4AddHintTrack(mp4file, trackid);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }
  const char *type;

  if (audioType == MP4_ALAW_AUDIO_TYPE) {
    payload = 8;
    type = "PCMA";
  } else {
    payload = 0;
    type = "PCMU";
  }

  MP4SetHintTrackRtpPayload(mp4file, hintTrackId, type, &payload, 0,NULL,
			    false);

  MP4Duration sampleDuration;
  bool have_skip;
  sampleId = 1;
  sampleSize = MP4GetSampleSize(mp4file, trackid, sampleId);
  sampleDuration = MP4GetSampleDuration(mp4file, trackid, sampleId);
  have_skip = sampleDuration != sampleSize;
  sampleOffset = 0;
  bytes_this_hint = 0;

  if (maxPayloadSize > 160) maxPayloadSize = 160;

  while (1) {
    if (bytes_this_hint == 0) {
#ifdef DEBUG_G711
      printf("Adding hint/packet\n");
#endif
      MP4AddRtpHint(mp4file, hintTrackId);
      MP4AddRtpPacket(mp4file, hintTrackId, false); // marker bit 0
    }
    uint16_t bytes_left_this_packet;
    bytes_left_this_packet = maxPayloadSize - bytes_this_hint;
    if (sampleSize >= bytes_left_this_packet) {
      MP4AddRtpSampleData(mp4file, hintTrackId, 
			  sampleId, sampleOffset, bytes_left_this_packet);
      bytes_this_hint += bytes_left_this_packet;
      sampleSize -= bytes_left_this_packet;
      sampleOffset += bytes_left_this_packet;
#ifdef DEBUG_G711
      printf("Added sample with %u bytes\n", bytes_left_this_packet);
#endif
    } else {
      MP4AddRtpSampleData(mp4file, hintTrackId, 
			  sampleId, sampleOffset, sampleSize);
      bytes_this_hint += sampleSize;
#ifdef DEBUG_G711
      printf("Added sample with %u bytes\n", sampleSize);
#endif
      sampleSize = 0;
    }

    if (bytes_this_hint >= maxPayloadSize) {
      // Write the hint
      // duration is bytes written
      MP4WriteRtpHint(mp4file, hintTrackId, bytes_this_hint);
#ifdef DEBUG_G711
      printf("Finished packet - bytes %u\n", bytes_this_hint);
#endif
      bytes_this_hint = 0;
    }
    if (sampleSize == 0) {
      // next sample
      if (have_skip && bytes_this_hint != 0) {
#ifdef DEBUG_G711
	printf("duration - ending packet - bytes %u\n", bytes_this_hint);
#endif
	MP4WriteRtpHint(mp4file, hintTrackId, bytes_this_hint);
	bytes_this_hint = 0;
      }
      sampleId++;
      if (sampleId > numSamples) {
	// finish it and exit
	if (bytes_this_hint != 0) {
	  MP4WriteRtpHint(mp4file, hintTrackId, bytes_this_hint);
	}
	return true;
      }
      sampleSize = MP4GetSampleSize(mp4file, trackid, sampleId);
      sampleDuration = MP4GetSampleDuration(mp4file, trackid, sampleId);
      have_skip = sampleDuration != sampleSize;
#ifdef DEBUG_G711
      printf("Next sample %u - size %u %u\n", sampleId, sampleSize,
	     have_skip);
#endif
      sampleOffset = 0;
    }
  }
	
  return true; // will never reach here
}
  
