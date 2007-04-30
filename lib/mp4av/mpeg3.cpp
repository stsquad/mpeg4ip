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
 * Copyright (C) Cisco Systems Inc. 2002 - 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */

#include "mpeg4ip.h"
#include "mp4av.h"

//#define DEBUG_MPEG3_HINT 1

static double mpeg3_frame_rate_table[16] =
{
  0.0,   /* Pad */
  24000.0/1001.0,       /* Official frame rates */
  24.0,
  25.0,
  30000.0/1001.0,
  30.0,
  50.0,
  ((60.0*1000.0)/1001.0),
  60.0,

  1,                    /* Unofficial economy rates */
  5, 
  10,
  12,
  15,
  0,
  0,
};
#define MPEG3_START_CODE_PREFIX          0x000001
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_EXT_START_CODE             0x000001b5
#define MPEG3_SLICE_MIN_START            0x00000101
#define MPEG3_SLICE_MAX_START            0x000001af

#define SEQ_ID 1
extern "C" int MP4AV_Mpeg3ParseSeqHdr (const uint8_t *pbuffer,
				       uint32_t buflen,
				       int *have_mpeg2,
				       uint32_t *height,
				       uint32_t *width,
				       double *frame_rate,
				       double *bitrate,
				       double *aspect_ratio,
				       uint8_t *mpeg2_profile)
{
  uint32_t aspect_code;
  uint32_t framerate_code;
  uint32_t bitrate_int;
  uint32_t bitrate_ext;
#if 1
  uint32_t scode, ix;
  int found = -1;
  if (have_mpeg2 != NULL) *have_mpeg2 = 0;
  if (mpeg2_profile != NULL) *mpeg2_profile = 0;
  buflen -= 6;
  bitrate_int = 0;
  for (ix = 0; ix < buflen; ix++, pbuffer++) {
    scode = (pbuffer[0] << 24) | (pbuffer[1] << 16) | (pbuffer[2] << 8) | 
      pbuffer[3];

    if (scode == MPEG3_SEQUENCE_START_CODE) {
      pbuffer += sizeof(uint32_t);
      if (width != NULL) {
	*width = (pbuffer[0]);
	*width <<= 4;
	*width |= ((pbuffer[1] >> 4) &0xf);
      }
      if (height != NULL) {
	*height = (pbuffer[1] & 0xf);
	*height <<= 8;
	*height |= pbuffer[2];
      }
      aspect_code = (pbuffer[3] >> 4) & 0xf;
      if (aspect_ratio != NULL) {
	switch (aspect_code) {
	default: *aspect_ratio = 1.0; break;
	case 2: *aspect_ratio = 4.0 / 3.0; break;
	case 3: *aspect_ratio = 16.0 / 9.0; break;
	case 4: *aspect_ratio = 2.21; break;
	}
      }
	  
	
      framerate_code = pbuffer[3] & 0xf;
      if (frame_rate != NULL) {
	*frame_rate = mpeg3_frame_rate_table[framerate_code];
      }
      // 18 bits
      bitrate_int = (pbuffer[4] << 10) | 
	(pbuffer[5] << 2) | 
	((pbuffer[6] >> 6) & 0x3);
      if (bitrate != NULL) {
	*bitrate = bitrate_int;
	*bitrate *= 400.0;
      }
      ix += sizeof(uint32_t) + 7;
      pbuffer += 7;
      found = 0;
    } else if (found == 0) {
      if (scode == MPEG3_EXT_START_CODE) {
	pbuffer += sizeof(uint32_t);
	ix += sizeof(uint32_t);
	switch ((pbuffer[0] >> 4) & 0xf) {
	case SEQ_ID:
	  if (have_mpeg2 != NULL) *have_mpeg2 = 1;
	  if (mpeg2_profile != NULL) 
	    *mpeg2_profile = ((pbuffer[0] & 0xf) << 4) | 
	      ((pbuffer[1] >> 4) & 0xf);
	  if (height != NULL) {
	    *height = ((pbuffer[1] & 0x1) << 13) | 
	      ((pbuffer[2] & 0x80) << 5) |
	      (*height & 0x0fff);
	  }
	  if (width != NULL) 
	    *width = (((pbuffer[2] >> 5) & 0x3) << 12) | (*width & 0x0fff);
	  bitrate_ext = (pbuffer[2] & 0x1f) << 7;
	  bitrate_ext |= (pbuffer[3] >> 1) & 0x7f;
	  bitrate_int |= (bitrate_ext << 18);
	  if (bitrate != NULL) {
	    *bitrate = bitrate_int;
	    *bitrate *= 400.0;
	  }
	  break;
	default:
	  break;
	}
	pbuffer++;
	ix++;
      } else if (scode == MPEG3_PICTURE_START_CODE) {
	return found;
      }
    }
  }
  return found;

#else
  // if you want to do the whole frame
  int ix;
  CBitstream bs(pbuffer, buflen);

  
  try {

    while (bs.PeekBits(32) != MPEG3_SEQUENCE_START_CODE) {
      bs.GetBits(8);
    }

    bs.GetBits(32); // start code

    *height = bs.GetBits(12);
    *width = bs.GetBits(12);
    aspect_code = bs.GetBits(4);
    if (aspect_ratio != NULL) {
      switch (aspect_code) {
      default: *aspect_ratio = 1.0; break;
      case 2: *aspect_ratio = 4.0 / 3.0; break;
      case 3: *aspect_ratio = 16.0 / 9.0; break;
      case 4: *aspect_ratio = 2.21; break;
      }
    }
    framerate_code = bs.GetBits(4);
    *frame_rate = mpeg3_frame_rate_table[framerate_code];

    bs.GetBits(18); // bitrate
    bs.GetBits(1);  // marker bit
    bs.GetBits(10); // vbv buffer
    bs.GetBits(1); // constrained params
    if (bs.GetBits(1)) {  // intra_quantizer_matrix
      for (ix = 0; ix < 64; ix++) {
	bs.GetBits(8);
      }
    }
    if (bs.GetBits(1)) { // non_intra_quantizer_matrix
      for (ix = 0; ix < 64; ix++) {
	bs.GetBits(8);
      }
    }
  } catch (...) {
    return false;
  }
  return true;
#endif
}

extern "C" int MP4AV_Mpeg3PictHdrType (const uint8_t *pbuffer)
{
  pbuffer += sizeof(uint32_t);
  return ((pbuffer[1] >> 3) & 0x7);
}

extern "C" uint16_t MP4AV_Mpeg3PictHdrTempRef (const uint8_t *pbuffer)
{
  pbuffer += sizeof(uint32_t);
  return ((pbuffer[0] << 2) | ((pbuffer[1] >> 6) & 0x3));
}

extern "C" int MP4AV_Mpeg3FindNextStart (const uint8_t *pbuffer, 
					 uint32_t buflen,
					 uint32_t *optr, 
					 uint32_t *scode)
{
  uint32_t value;
  uint32_t offset;

  if (buflen < 4) return -1;
  for (offset = 0; offset < buflen - 3; offset++, pbuffer++) {
#ifdef WORDS_BIGENDIAN
    value = *(uint32_t *)pbuffer >> 8;
#else
    value = (pbuffer[0] << 16) | (pbuffer[1] << 8) | (pbuffer[2] << 0); 
#endif

    if (value == MPEG3_START_CODE_PREFIX) {
      *optr = offset;
      *scode = (value << 8) | pbuffer[3];
      return 0;
    }
  }
  return -1;
}

extern "C" int MP4AV_Mpeg3FindNextSliceStart (const uint8_t *pbuffer,
					      uint32_t startoffset, 
					      uint32_t buflen,
					      uint32_t *slice_offset)
{
  uint32_t slicestart, code;
  while (MP4AV_Mpeg3FindNextStart(pbuffer + startoffset, 
		       buflen - startoffset, 
		       &slicestart, 
		       &code) >= 0) {
#ifdef DEBUG_MPEG3_HINT
    printf("Code %x at offset %d\n", 
	   code, startoffset + slicestart);
#endif
    if ((code >= MPEG3_SLICE_MIN_START) &&
	(code <= MPEG3_SLICE_MAX_START)) {
      *slice_offset = slicestart + startoffset;
      return 0;
    }
    startoffset += slicestart + 4;
  }
  return -1;
}
				  
extern "C" int MP4AV_Mpeg3FindPictHdr (const uint8_t *pbuffer,
				       uint32_t buflen,
				       int *frame_type)
{
  uint32_t value;
  uint32_t offset;
  int ftype;
  for (offset = 0; offset < buflen; offset++, pbuffer++) {
    value = (pbuffer[0] << 24) | (pbuffer[1] << 16) | (pbuffer[2] << 8) | 
      pbuffer[3];

    if (value == MPEG3_PICTURE_START_CODE) {
      ftype = MP4AV_Mpeg3PictHdrType(pbuffer);
      if (frame_type != NULL) *frame_type = ftype;
      return offset;
    } 
  }
  return -1;
}

extern "C" bool Mpeg12Hinter (MP4FileHandle mp4file,
			      MP4TrackId trackid,
			      uint16_t maxPayloadSize)
{
  uint32_t numSamples, maxSampleSize;
  uint8_t videoType;
  uint8_t rfc2250[4], rfc2250_2;
  uint32_t offset;
  uint32_t scode;
  int have_seq;
  bool stop;
  uint8_t *buffer, *pbuffer;
  uint8_t *pstart;
  uint8_t type;
  uint32_t next_slice, prev_slice;
  bool slice_at_begin;
  uint32_t sampleSize;
  MP4SampleId sid;

  MP4TrackId hintTrackId;

  numSamples = MP4GetTrackNumberOfSamples(mp4file, trackid);
  maxSampleSize = MP4GetTrackMaxSampleSize(mp4file, trackid);

  if (numSamples == 0) return false;

  videoType = MP4GetTrackEsdsObjectTypeId(mp4file, trackid);

  if (videoType != MP4_MPEG1_VIDEO_TYPE &&
      videoType != MP4_MPEG2_VIDEO_TYPE) {
    return false;
  }

  hintTrackId = MP4AddHintTrack(mp4file, trackid);
  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  uint8_t payload = 32;
  if (MP4SetHintTrackRtpPayload(mp4file, hintTrackId, 
				"MPV", &payload, 0) == false) {
    MP4DeleteTrack(mp4file, hintTrackId);
    return false;
  }


  buffer = (uint8_t *)malloc(maxSampleSize);
  if (buffer == NULL) {
    MP4DeleteTrack(mp4file, hintTrackId);
    return false;
  }

  maxPayloadSize -= 24; // this is for the 4 byte header

  for (sid = 1; sid <= numSamples; sid++) {
    sampleSize = maxSampleSize;
    MP4Timestamp startTime;
    MP4Duration duration;
    MP4Duration renderingOffset;
    bool isSyncSample;

    bool rc = MP4ReadSample(mp4file, trackid, sid,
			    &buffer, &sampleSize, 
			    &startTime, &duration, 
			    &renderingOffset, &isSyncSample);
#ifdef DEBUG_MPEG3_HINT
    printf("sid %d - sample size %d\n", sid, sampleSize);
#endif
    if (rc == false) {
      MP4DeleteTrack(mp4file, hintTrackId);
      return false;
    }

    // need to add rfc2250 header
    offset = 0;
    have_seq = 0;
    pbuffer = buffer;
    stop = false;
    do {
      uint32_t oldoffset;
      oldoffset = offset;
      if (MP4AV_Mpeg3FindNextStart(pbuffer + offset, 
			sampleSize - offset, 
			&offset, 
			&scode) < 0) {
	// didn't find the start code
#ifdef DEBUG_MPEG3_HINT
	printf("didn't find start code\n");
#endif
	stop = true;
      } else {
	offset += oldoffset;
#ifdef DEBUG_MPEG3_HINT
	printf("next sscode %x found at %d\n", scode, offset);
#endif
	if (scode == MPEG3_SEQUENCE_START_CODE) have_seq = 1;
	offset += 4; // start with next value
      }
    } while (scode != MPEG3_PICTURE_START_CODE && stop == false);
    
    pstart = pbuffer + offset; // point to inside of picture start
    type = (pstart[1] >> 3) & 0x7;

    rfc2250[0] = (*pstart >> 6) & 0x3;
    rfc2250[1] = (pstart[0] << 2) | ((pstart[1] >> 6) & 0x3); // temporal ref
    rfc2250[2] = type;
    rfc2250_2 = rfc2250[2];

    rfc2250[3] = 0;
    if (type == 2 || type == 3) {
      rfc2250[3] = pstart[3] << 5;
      if ((pstart[4] & 0x80) != 0) rfc2250[3] |= 0x10;
      if (type == 3) {
	rfc2250[3] |= (pstart[4] >> 3) & 0xf;
      }
    }

    if (MP4AddRtpVideoHint(mp4file, hintTrackId, 
			   type == 3, renderingOffset) == false)
      return false;
    // Find the next slice.  Then we can add the header if the next
    // slice will be in the start.  This lets us set the S bit in 
    // rfc2250[2].  Then we need to loop to find the next slice that's
    // not in the buffer size - this should be in the while loop.
    
    prev_slice = 0;
    if (MP4AV_Mpeg3FindNextSliceStart(pbuffer, offset, sampleSize, &next_slice) < 0) {
      slice_at_begin = false;
    } else {
      slice_at_begin = true;
    }

#ifdef DEBUG_MPEG3_HINT
    printf("starting slice at %d\n", next_slice);
#endif
    offset = 0;
    bool nomoreslices = false;
    bool found_slice = slice_at_begin;
    bool onfirst = true;

    while (sampleSize > 0) {
      bool isLastPacket;
      uint32_t len_to_write;

      if (sampleSize <= maxPayloadSize) {
	// leave started_slice alone
	len_to_write = sampleSize;
	isLastPacket = true;
	prev_slice = 0;
      } else {
	found_slice =  (onfirst == false) && (nomoreslices == false) && (next_slice <= maxPayloadSize);
	onfirst = false;
	isLastPacket = false;

	while (nomoreslices == false && next_slice <= maxPayloadSize) {
	  prev_slice = next_slice;
	  if (MP4AV_Mpeg3FindNextSliceStart(pbuffer, next_slice + 4, sampleSize, &next_slice) >= 0) {
#ifdef DEBUG_MPEG3_HINT
	    printf("prev_slice %u next slice %u %u\n", prev_slice, next_slice,
		   offset + next_slice);
#endif
	    found_slice = true;
	  } else {
	    // at end
	    nomoreslices = true;
	  }
	}
	// prev_slice should have the end value.  If it's not 0, we have
	// the end of the slice.
	if (found_slice) len_to_write = prev_slice;
	else len_to_write = MIN(maxPayloadSize, sampleSize);
      } 

      rfc2250[2] = rfc2250_2;
      if (have_seq != 0) {
	rfc2250[2] |= 0x20;
	have_seq = 0;
      }
      if (slice_at_begin) {
	rfc2250[2] |= 0x10; // set b bit
      }
      if (found_slice || isLastPacket) {
	rfc2250[2] |= 0x08; // set end of slice bit
	slice_at_begin = true; // for next time
      } else {
	slice_at_begin = false;
      }

#ifdef DEBUG_MPEG3_HINT
      printf("Adding packet, sid %u prev slice %u len_to_write %u\n",
	     sid, prev_slice, len_to_write);
      printf("Next slice %u offset %u %x %x %x %x\n\n", 
	     next_slice, offset, 
	     rfc2250[0], rfc2250[1], rfc2250[2], rfc2250[3]);
#endif

      // okay - we can now write out this packet.
      if (MP4AddRtpPacket(mp4file, hintTrackId, isLastPacket) == false ||
      // add the 4 byte header
	  MP4AddRtpImmediateData(mp4file, hintTrackId, 
				 rfc2250, sizeof(rfc2250)) == false ||

      // add the immediate data
	  MP4AddRtpSampleData(mp4file, hintTrackId, sid, 
			      offset, len_to_write) == false) {
	MP4DeleteTrack(mp4file, hintTrackId);
	return false;
      }
      offset += len_to_write;
      sampleSize -= len_to_write;
      prev_slice = 0;
      next_slice -= len_to_write;
      pbuffer += len_to_write;
    }
    if (MP4WriteRtpHint(mp4file, hintTrackId, duration, type == 1) == false)
      return false;
  }

  free(buffer);
  return true;
}

// mpeg3_find_dts_from_pts - given a pts, a frame type, and the
// temporal reference from the frame type, calculate the dts
// pts = presentation time stamp 
// dts = decode time stamp.
int mpeg3_find_dts_from_pts (mpeg3_pts_to_dts_t *ptr,
			     uint64_t pts_in_msec,
			     int frame_type,
			     uint16_t temp_ref,
			     uint64_t *return_value)
{
  uint64_t calc;
  double dcalc;
  double msec_per_frame = 1000.0 / ptr->frame_rate;
  int64_t diff;

  switch (frame_type) {
  case 1: // i frame
    // I frame calculation - take the temporal reference + 1, multiple it
    // times frame rate, subtract from pts to get dts.
    dcalc = (temp_ref + 1);
    dcalc *= msec_per_frame;
    calc = pts_in_msec - (uint64_t)dcalc;
    ptr->last_i_temp_ref = temp_ref;
    ptr->last_i_pts = pts_in_msec;
    ptr->last_i_dts = calc;
    *return_value = calc;
    break;
  case 2: 
    // P frames suck.
    // see if the difference between temporal references in the last
    // frame is the difference between pts of the last I and this P frame
    // if they are, we most likely haven't lost a frame
    dcalc = (temp_ref - ptr->last_i_temp_ref); 
    dcalc *= msec_per_frame;
    diff = pts_in_msec - ptr->last_i_pts;
    calc = (uint64_t)dcalc;
    diff -= calc;
    if (diff > TO_D64(10) || diff < TO_D64(-10)) {
      // out of range - we really can't guess
      // we could probably do more work here - record the number of 
      // consectutive b frames, etc, but what it really means is that
      // we need to wait until the next I frame to display sanely.
      // We'll still decode, and may even display if B frames are 
      // present, but we'll jerk a bit on these frames.
      // It all comes down to me being lazy...
      printf("pts out of range - diff "D64", temps %u %u\n",
	     diff, temp_ref, ptr->last_i_temp_ref);
      printf("our pts "U64" last "U64"\n", pts_in_msec, ptr->last_i_pts);
      return -1;
    }
    if (ptr->last_i_temp_ref != 0) {
      // straight forward calculation
      *return_value = ptr->last_i_dts + calc;
    } else {
      // if the temp ref of the I frame was 0, there's no good calculation
      // of how to get to the P frame time.  
      // a less straight forward calculation - just increment from the
      // last dts - there's no real way to calculate this accurately
      *return_value = ptr->last_dts + (uint64_t)msec_per_frame;
    }
    break;
  case 3:
    *return_value = pts_in_msec;
    break;
  }
  ptr->last_dts = *return_value;
  return 0;
}

uint8_t mpeg2_profile_to_mp4_track_type (uint8_t profile)
{
  if (profile == 0) {
    return MP4_MPEG2_VIDEO_TYPE;
  }
  if ((profile & 0x80) == 0) {
    switch ((profile & 0x70) >> 4) {
    case 5:
      return MP4_MPEG2_SIMPLE_VIDEO_TYPE;
    case 4:
      return MP4_MPEG2_MAIN_VIDEO_TYPE;
    case 3:
      return MP4_MPEG2_SNR_VIDEO_TYPE;
    case 2:
      return MP4_MPEG2_SPATIAL_VIDEO_TYPE;
    case 1:
      return MP4_MPEG2_HIGH_VIDEO_TYPE;
    default:
      return MP4_MPEG2_VIDEO_TYPE;
    }
  } 
  if (profile == 0x82 || profile == 0x85) {
    return MP4_MPEG2_442_VIDEO_TYPE;
  }
  return MP4_MPEG2_VIDEO_TYPE;
}

static const char *profile_names[] = {
  "Mpeg-2 High@<unk>",
  "Mpeg-2 High@High",
  "Mpeg-2 High@High 1440",
  "Mpeg-2 High@Main",
  "Mpeg-2 High@Low"
  "Mpeg-2 Spatially Scalable@<unk>",
  "Mpeg-2 Spatially Scalable@High",
  "Mpeg-2 Spatially Scalable@High 1440",
  "Mpeg-2 Spatially Scalable@Main",
  "Mpeg-2 Spatially Scalable@Low",
  "Mpeg-2 SNR Scalable@<unk>",
  "Mpeg-2 SNR Scalable@High",
  "Mpeg-2 SNR Scalable@High 1440",
  "Mpeg-2 SNR Scalable@Main",
  "Mpeg-2 SNR Scalable@Low",
  "Mpeg-2 Main@High<unk>",
  "Mpeg-2 Main@High",
  "Mpeg-2 Main@High 1440",
  "Mpeg-2 Main@Main",
  "Mpeg-2 Main@Low",
  "Mpeg-2 Simple@<unk>",
  "Mpeg-2 Simple@High",
  "Mpeg-2 Simple@High 1440",
  "Mpeg-2 Simple@Main",
  "Mpeg-2 Simple@Low", 
};

const char *mpeg2_type (uint8_t profile) 
{
  if (profile == 0) {
    return "Mpeg-2";
  }
  
  if ((profile & 0x80) == 0) {
    uint8_t index;
    index = ((profile & 0x70) >> 4);
    if (index == 0 || index > 5) {
      return "Mpeg-2 unknown profile";
    }
    index--;
    index *= 5;
    
    uint8_t level = profile & 0xf;
    if ((level & 1) != 0 ||
	(level > 0xb)) {
      // no change to index:
      return profile_names[index];
    }
    level >>= 1;
    level -= 2;
    index += level;
    return profile_names[index];
  } 
  if (profile == 0x82) 
    return "Mpeg-2 4:2:2@High";
  if (profile == 0x85) {
    return "Mpeg-2 4:2:2@Main";
  }
  if (profile == 0x8a) 
    return "Mpeg-2 Multiview@High";
  if (profile == 0x8b) 
    return "Mpeg-2 Multiview@High 1440";
  if (profile == 0x8d) 
    return "Mpeg-2 Multiview@Main";
  if (profile == 0x8e) 
    return "Mpeg-2 Multiview@Low";

  return "Mpeg-2 unknown escape profile";
}

    
