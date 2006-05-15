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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May   wmay@cisco.com
 */

#include <mp4av_common.h>

//#define DEBUG_RFC3267 1
static short blockSize[16] = 
  { 12, 13, 15, 17, 19, 20, 26, 31,  5, -1, -1, -1, -1, -1, -1, -1};
static short blockSizeWB[16] = 
  { 17, 23, 32, 36, 40, 46, 50, 58, 60,  5,  5, -1, -1, -1,  0,  0 };

/*
 * MP4AV_AmrFrameSize - given mode, give the number of bytes of speech
 * data.  This is NOT including the header byte.
 */
extern "C" int16_t MP4AV_AmrFrameSize (uint8_t mode, bool isAmrWb)
{
  uint8_t decMode = (mode >> 3) & 0xf;
  if (isAmrWb) return blockSizeWB[decMode];
  return blockSize[decMode];
}

extern "C" bool MP4AV_Rfc3267Hinter (MP4FileHandle mp4File,
				     MP4TrackId mediaTrackId,
				     uint16_t maxPayloadSize)
{
  uint32_t numSamples = 
    MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

  if (numSamples == 0) return false;

  const char *media_data_name = 
    MP4GetTrackMediaDataName(mp4File, mediaTrackId);

  bool isAmrWb = strcmp(media_data_name, "sawb") == 0;

  if (isAmrWb == false &&
      strcmp(media_data_name, "samr") != 0) {
    return false;
  }


  MP4TrackId hintTrackId = 
    MP4AddHintTrack(mp4File, mediaTrackId);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  uint8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;

  MP4SetHintTrackRtpPayload(mp4File, 
			    hintTrackId, 
			    isAmrWb ? "AMR-WB" : "AMR",
			    &payloadNumber,
			    0,
			    "1",
			    true,
			    false);

  char sdpBuf[80];
  sprintf(sdpBuf, "a=fmtp:%u octet-align=1;\015\012",
	  payloadNumber);

  MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);
  
  struct {
    MP4SampleId sid;
    uint32_t offset;
    uint32_t len;
  } PakBuffer[12];

  uint8_t toc[13];
  toc[0] = 0xf0; // (MP4GetAmrModeSet(mp4File, mediaTrackId) << 4);

  MP4SampleId sid = 0;
  uint32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
  uint32_t sampleSize = 0;
  uint32_t offset_on = 0;
  uint32_t toc_on = 0;
  uint8_t *buffer = (uint8_t *)malloc(maxSampleSize);
  MP4Timestamp startTime;
  MP4Duration duration;
  MP4Duration renderingOffset;
  bool isSyncSample;
  uint32_t bytes_in_pak = 0;

#ifdef DEBUG_RFC3267
  printf("max sample size is %u\n", maxSampleSize);
#endif
  while (sid < numSamples) {
    if (offset_on >= sampleSize) {
      sid++;
      if (sid > numSamples) continue; // will break out...
      sampleSize = maxSampleSize;
#ifdef DEBUG_RFC3267
      printf("reading sample %u\n", sid);
#endif
      MP4ReadSample(mp4File, mediaTrackId, sid,
		    &buffer, &sampleSize, 
		    &startTime, &duration, 
		    &renderingOffset, &isSyncSample);
      offset_on = 0;
    }
    uint16_t frameSize = MP4AV_AmrFrameSize(buffer[0], isAmrWb);

    if (bytes_in_pak + frameSize > maxPayloadSize || 
	toc_on >= 12) {
      // write it
      MP4AddRtpHint(mp4File, hintTrackId);
      MP4AddRtpPacket(mp4File, hintTrackId);
#ifdef DEBUG_RFC3267
      printf("pak - writing immediat %u\n", toc_on + 1);
#endif
      MP4AddRtpImmediateData(mp4File, hintTrackId,
			     toc, toc_on + 1);
      for (uint32_t ix = 0; ix < toc_on; ix++) {
#ifdef DEBUG_RFC3267
	printf("pak - writing sid %u %u %u\n", 
	       PakBuffer[ix].sid, 
	       PakBuffer[ix].offset, 
	       PakBuffer[ix].len);
#endif
	MP4AddRtpSampleData(mp4File, 
			    hintTrackId, 
			    PakBuffer[ix].sid, 
			    PakBuffer[ix].offset,
			    PakBuffer[ix].len);
      }
      MP4Duration duration = toc_on * (isAmrWb ? 320 : 160);
      MP4WriteRtpHint(mp4File, hintTrackId, duration);
      toc_on = 0;
      bytes_in_pak = 0;
    }
    if (toc_on > 0) toc[toc_on] |= 0x80;
#ifdef DEBUG_RFC3267
    printf("toc %d %x sample %d offset %d len %d\n", 
	   toc_on, buffer[0], sid, offset_on, frameSize);
#endif
    toc[toc_on + 1] = (buffer[0] & 0x78) | 0x04;

    PakBuffer[toc_on].sid = sid;
    // we don't want to include the 1 byte header - the framesize
    // returned doesn't include the header byte.
    PakBuffer[toc_on].offset = offset_on + 1;
    PakBuffer[toc_on].len = frameSize;
    offset_on += frameSize + 1;
    bytes_in_pak += frameSize + 1;
    toc_on++;
  }

  // finish it.  

  if (toc_on > 0) {
    MP4AddRtpHint(mp4File, hintTrackId);
    MP4AddRtpPacket(mp4File, hintTrackId, 1);
    MP4AddRtpImmediateData(mp4File, hintTrackId,
			   toc, toc_on + 1);
    for (uint32_t ix = 0; ix < toc_on; ix++) {
      MP4AddRtpSampleData(mp4File, 
			  hintTrackId, 
			  PakBuffer[ix].sid, 
			  PakBuffer[ix].offset,
			  PakBuffer[ix].len);
    }
    MP4Duration duration = toc_on * (isAmrWb ? 320 : 160);
    MP4WriteRtpHint(mp4File, hintTrackId, duration);
  }
  free(buffer);
  return true;
}
