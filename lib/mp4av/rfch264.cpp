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
#include <mp4av_h264.h>

//#define DEBUG_H264_HINT 1
//#define ADD_PICT_SEQ_TO_STREAM 1

extern "C" MP4TrackId MP4AV_H264_HintTrackCreate (MP4FileHandle mp4File,
						  MP4TrackId mediaTrackId,
						  uint16_t maxPayload)
{
  /* get the mpeg4 video configuration */
  u_int8_t **pSeq, **pPict ;
  u_int32_t *pSeqSize, *pPictSize;
  char *base64;
  uint32_t profile_level;
  char *sprop = NULL;
  uint32_t ix = 0;

  MP4GetTrackH264SeqPictHeaders(mp4File,
				mediaTrackId,
				&pSeq,
				&pSeqSize,
				&pPict,
				&pPictSize);
				      
  if (pSeqSize == NULL || pSeqSize == NULL ||
      pPict == NULL || pPictSize == NULL) {
    return MP4_INVALID_TRACK_ID;
  }
  // we have valid sequence and picture headers
  uint8_t *p = pSeq[0];
  if (*p == 0 && p[1] == 0 && 
      (p[2] == 1 || (p[2] == 0 && p[3] == 0))) {
    if (p[2] == 0) p += 4;
    else p += 3;
  }
  profile_level = p[0] << 16 |
    p[1] << 8 |
    p[2];
  while (pSeqSize[ix] != 0) {
    base64 = MP4BinaryToBase64(pSeq[ix], pSeqSize[ix]);
    if (sprop == NULL) {
      sprop = strdup(base64);
    } else {
      uint len = strlen(base64) + 1 + 1;
      if (sprop != NULL) len += strlen(sprop);
      sprop = (char *)realloc(sprop, len);
      
      if (sprop == NULL) return MP4_INVALID_TRACK_ID;
      strncat(sprop, ",", len - strlen(sprop));
      strncat(sprop, base64, len - strlen(sprop));
    }
    free(base64);
    free(pSeq[ix]);
    ix++;
  }
  free(pSeq);
  free(pSeqSize);
  
  ix = 0;
  while (pPictSize[ix] != 0) {
    base64 = MP4BinaryToBase64(pPict[ix], pPictSize[ix]);
    uint len = strlen(base64) + 1 + 1;
    if (sprop != NULL) len += strlen(sprop);
    sprop = (char *)realloc(sprop, len);

    if (sprop == NULL) return MP4_INVALID_TRACK_ID;
    strncat(sprop, ",", len - strlen(sprop));
    strncat(sprop, base64, len - strlen(sprop));

    free(base64);
    free(pPict[ix]);
    ix++;
  }
  free(pPict);
  free(pPictSize);

  if (sprop == NULL) return MP4_INVALID_TRACK_ID;

  MP4TrackId hintTrackId =
    MP4AddHintTrack(mp4File, mediaTrackId);
  
  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return MP4_INVALID_TRACK_ID;
  }
  
  u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;
  
  // don't include mpeg4-esid
  if (MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
				"H264", &payloadNumber, maxPayload,
				NULL, true, false) == false) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return MP4_INVALID_TRACK_ID;
  }
  
  
  /* create the appropriate SDP attribute */
  char* sdpBuf = (char*)malloc(strlen(sprop) + 128);
  if (sdpBuf == NULL) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return MP4_INVALID_TRACK_ID;
  }
  snprintf(sdpBuf,
	     strlen(sprop) + 128,
	   "a=fmtp:%u profile-level-id=%06x; sprop-parameter-sets=%s; packetization-mode=1\015\012",
	   payloadNumber,
	   profile_level,
	   sprop); 
  
  CHECK_AND_FREE(sprop);
  /* add this to the track's sdp */
  if (MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf) == false) {
    MP4DeleteTrack(mp4File, hintTrackId);
    hintTrackId = MP4_INVALID_TRACK_ID;
  }
  
  free(sprop);
  free(sdpBuf);

  return hintTrackId;
}

static uint32_t h264_get_nal_size (uint8_t *pData,
				   uint32_t sizeLength)
{
  if (sizeLength == 1) {
    return *pData;
  } else if (sizeLength == 2) {
    return (pData[0] << 8) | pData[1];
  } else if (sizeLength == 3) {
    return (pData[0] << 16) |(pData[1] << 8) | pData[2];
  }
  return (pData[0] << 24) |(pData[1] << 16) |(pData[2] << 8) | pData[3];
}

static uint8_t h264_get_sample_nal_type (uint8_t *pSampleBuffer, 
					 uint32_t sampleSize,
					 uint32_t sizeLength)
{
  uint32_t offset = 0;
  while (offset < sampleSize) {
    uint8_t nal_type = pSampleBuffer[sizeLength] & 0x1f;
    uint32_t add;

    if (h264_nal_unit_type_is_slice(nal_type)) {
      return nal_type;
    }
    add = h264_get_nal_size(pSampleBuffer, sizeLength) + sizeLength;
    offset += add;
    pSampleBuffer += add;
  }
  return 0;
}
extern "C" bool MP4AV_H264_HintAddSample (MP4FileHandle mp4File,
					  MP4TrackId mediaTrackId,
					  MP4TrackId hintTrackId,
					  MP4SampleId sampleId,
					  uint8_t *pSampleBuffer,
					  uint32_t sampleSize,
					  uint32_t sizeLength,
					  MP4Duration duration,
					  MP4Duration renderingOffset,
					  bool isSyncSample,
					  uint16_t maxPayloadSize)
{
  uint8_t nal_type = h264_get_sample_nal_type(pSampleBuffer, 
					      sampleSize, 
					      sizeLength);
  // for now, we don't know if we can drop frames, so don't indiate
  // that any are "b" frames
  bool isBFrame = false;
  uint32_t nal_size;
  uint32_t offset = 0;
  uint32_t remaining = sampleSize;
#ifdef DEBUG_H264_HINT
  printf("hint for sample %d %u\n", sampleId, remaining);
#endif
  if (MP4AddRtpVideoHint(mp4File, hintTrackId, 
			 isBFrame, renderingOffset) == false)
    return false;

#ifdef ADD_PICT_SEQ_TO_STREAM
  if (nal_type == H264_NAL_TYPE_IDR_SLICE) {
    u_int8_t **pSeq, **pPict ;
    u_int32_t *pSeqSize, *pPictSize;
    MP4GetTrackH264SeqPictHeaders(mp4File,
				  mediaTrackId,
				  &pSeq,
				  &pSeqSize,
				  &pPict,
				  &pPictSize);
    uint ix;
    for (ix = 0; pSeqSize[ix] != 0; ix++) {
      if (MP4AddRtpPacket(mp4File, hintTrackId, false) == false) return false;
      uint8_t *seq = pSeq[ix];
      uint len = pSeqSize[ix];
      do {
	uint wb = len > 14 ? 14 : len;
	if (MP4AddRtpImmediateData(mp4File, hintTrackId, seq, wb) == false) return false;
	seq += wb;
	len -= wb;
      } while (len > 0);
      free(pSeq[ix]);
    }
    for (ix = 0; pPictSize[ix] != 0; ix++) {
      if (MP4AddRtpPacket(mp4File, hintTrackId, false) == false) return false;
      uint8_t *seq = pPict[ix];
      uint len = pPictSize[ix];
      do {
	uint wb = len > 14 ? 14 : len;
	MP4AddRtpImmediateData(mp4File, hintTrackId, seq, wb);
	seq += wb;
	len -= wb;
      } while (len > 0);
      free(pPict[ix]);
    }
    free(pSeq);
    free(pPict);
    free(pSeqSize);
    free(pPictSize);
  }
#endif
  if (sampleSize - sizeLength < maxPayloadSize) {
    uint32_t first_nal = h264_get_nal_size(pSampleBuffer, sizeLength);
    if (first_nal + sizeLength == sampleSize) {
      // we have a single nal, less than the maxPayloadSize, 
      // so, we have Single Nal unit mode
      if (MP4AddRtpPacket(mp4File, hintTrackId, true) == false ||
	  MP4AddRtpSampleData(mp4File, hintTrackId, sampleId,
			      sizeLength, sampleSize - sizeLength) == false ||
	  MP4WriteRtpHint(mp4File, hintTrackId, duration, 
			  nal_type == H264_NAL_TYPE_IDR_SLICE) == false)
	return false;
      return true;
    }
  }

  // TBD should scan for resync markers (if enabled in ES config)
  // and packetize on those boundaries
  while (remaining) {
    nal_size = h264_get_nal_size(pSampleBuffer + offset,
					  sizeLength);
    // skip the sizeLength
#ifdef DEBUG_H264_HINT
    printf("offset %u nal size %u remain %u\n", offset, nal_size, remaining);
#endif

    offset += sizeLength;
    remaining -= sizeLength;
    if (nal_size > maxPayloadSize) {
      /*
       * We have fragmentation of a NAL here
       */
#ifdef DEBUG_H264_HINT
      printf("fragmentation units\n");
#endif
      uint8_t head = pSampleBuffer[offset];
      offset++;
      nal_size--;
      remaining--;
      uint8_t fu_header[2];
      fu_header[0] = (head & 0xe0) | 28; 
      fu_header[1] = 0x80;
      head &= 0x1f;
      while (nal_size > 0) {
	fu_header[1] |= head;
	uint32_t write_size;
	if (nal_size + 2 <= maxPayloadSize) {
	  fu_header[1] |= 0x40;
	  write_size = nal_size;
	} else {
	  write_size = maxPayloadSize - 2;
	}
#ifdef DEBUG_H264_HINT
	printf("frag off %u write %u left in nal %u remain %u\n",
	       offset, write_size, nal_size, remaining);
#endif
	remaining -= write_size;

	if (MP4AddRtpPacket(mp4File, hintTrackId, remaining == 0) == false ||
	    MP4AddRtpImmediateData(mp4File, hintTrackId, 
				   fu_header, 2) == false) return false;
	fu_header[1] = 0;

	if (MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
				offset, write_size) == false) return false;
	offset += write_size;
	nal_size -= write_size;
      }
    } else {
      // we have a smaller than MTU nal.  check the next sample
      // see if the next one fits;
      uint32_t next_size_offset;
      bool have_stap = false;
      next_size_offset = offset + nal_size;
      if (next_size_offset < remaining) {
	// we have a remaining NAL
	uint32_t next_nal_size = 
	  h264_get_nal_size(pSampleBuffer + next_size_offset, sizeLength);
#ifdef DEBUG_H264_HINT
	printf("next nal size %u\n", next_nal_size);
#endif
	if (next_nal_size + nal_size + 4 + 1 <= maxPayloadSize) {
	  have_stap = true;
	} 
      } 
      if (have_stap == false) {
	// we have to fit this nal into a packet - the next one is too big
#ifdef DEBUG_H264_HINT
	printf("have single NAL packet \n");
#endif
	if (MP4AddRtpPacket(mp4File, hintTrackId, 
			    next_size_offset >= remaining) == false ||
	    MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
				offset, nal_size) == false) return false;
	offset += nal_size;
	remaining -= nal_size;
      } else {
	// we can fit multiple NALs into this packet
	uint32_t bytes_in_stap = 1 + 2 + nal_size;
	uint8_t max_nri = pSampleBuffer[offset] & 0x70;
#ifdef DEBUG_H264_HINT
	printf("Start processing stap\n");
#endif
	while (next_size_offset < remaining && bytes_in_stap < maxPayloadSize) {
	  uint8_t nri;
	  nri = pSampleBuffer[next_size_offset + sizeLength] & 0x70;
	  if (nri > max_nri) max_nri = nri;
	  
	  uint32_t next_nal_size = 
	    h264_get_nal_size(pSampleBuffer + next_size_offset, sizeLength);
	  bytes_in_stap += 2 + next_nal_size;
	  next_size_offset += sizeLength + next_nal_size;
#ifdef DEBUG_H264_HINT
	  printf("next nal %u bytes in stap %u next offset %u\n",
		 next_nal_size, bytes_in_stap, next_size_offset);
#endif
	}
	bool last;
#ifdef DEBUG_H264_HINT
	printf("done - next_size offset %u remain %u bytes %u\n",
	       next_size_offset, remaining, bytes_in_stap);
#endif
	if (next_size_offset >= (offset +  remaining) && 
	    bytes_in_stap <= maxPayloadSize) {
	  // stap is last frame
	  last = true;
	} else last = false;
	uint8_t data[3];
	data[0] = max_nri | 24;
	data[1] = nal_size >> 8;
	data[2] = nal_size & 0xff;
	if (MP4AddRtpPacket(mp4File, hintTrackId, last) == false ||
	    MP4AddRtpImmediateData(mp4File, hintTrackId, 
				   data, 3) == false ||
	    MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
				offset, nal_size) == false) return false;
	offset += nal_size;
	remaining -= nal_size;
	bytes_in_stap = 1 + 2 + nal_size;
	nal_size = h264_get_nal_size(pSampleBuffer + offset, sizeLength);
	while (bytes_in_stap + nal_size <= maxPayloadSize &&
	       remaining) {
	  offset += sizeLength;
	  remaining -= sizeLength;
	  data[0] = nal_size >> 8;
	  data[1] = nal_size & 0xff;
	  if (MP4AddRtpImmediateData(mp4File, hintTrackId, data, 2) == false ||
	      MP4AddRtpSampleData(mp4File, hintTrackId, 
				  sampleId, offset, nal_size) == false) 
	    return false;

	  offset += nal_size;
	  remaining -= nal_size;
	  bytes_in_stap += nal_size + 2;
	  if (remaining) {
	    nal_size = h264_get_nal_size(pSampleBuffer + offset, sizeLength);
	  }
	} // end while stap
      } // end have stap
    } // end check size
  }

  return MP4WriteRtpHint(mp4File, hintTrackId, duration, 
			 nal_type == H264_NAL_TYPE_IDR_SLICE);
}

extern "C" bool MP4AV_H264Hinter(
				 MP4FileHandle mp4File, 
				 MP4TrackId mediaTrackId, 
				 u_int16_t maxPayloadSize)
{
  u_int32_t numSamples = MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
  u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
	
  uint32_t sizeLength;

  if (numSamples == 0 || maxSampleSize == 0) {
    return false;
  }

  if (MP4GetTrackH264LengthSize(mp4File, mediaTrackId, &sizeLength) == false) {
    return false;
  }

  MP4TrackId hintTrackId = 
    MP4AV_H264_HintTrackCreate(mp4File, mediaTrackId);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  u_int8_t* pSampleBuffer = (u_int8_t*)malloc(maxSampleSize);
  if (pSampleBuffer == NULL) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return false;
  }
  for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
    u_int32_t sampleSize = maxSampleSize;
    MP4Timestamp startTime;
    MP4Duration duration;
    MP4Duration renderingOffset;
    bool isSyncSample;

    bool rc = MP4ReadSample(
			    mp4File, mediaTrackId, sampleId, 
			    &pSampleBuffer, &sampleSize, 
			    &startTime, &duration, 
			    &renderingOffset, &isSyncSample);

    if (!rc) {
      MP4DeleteTrack(mp4File, hintTrackId);
      CHECK_AND_FREE(pSampleBuffer);
      return false;
    }

    if (MP4AV_H264_HintAddSample(mp4File,
				 mediaTrackId,
				 hintTrackId,
				 sampleId,
				 pSampleBuffer,
				 sampleSize,
				 sizeLength,
				 duration,
				 renderingOffset,
				 isSyncSample,
				 maxPayloadSize) == false) {
      MP4DeleteTrack(mp4File, hintTrackId);
      CHECK_AND_FREE(pSampleBuffer);
      return false;
    }
  }
  CHECK_AND_FREE(pSampleBuffer);

  return true;
}

