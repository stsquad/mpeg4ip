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
 *           Bill May wmay@cisco.com
 *           Laurent Aimar
 */

//#define DEBUG_H264 1

#include <mp4creator.h>
#include <mp4av_h264.h>

typedef struct nal_reader_t {
  FILE *ifile;
  uint8_t *buffer;
  uint32_t buffer_on;
  uint32_t buffer_size;
  uint32_t buffer_size_max;
} nal_reader_t;

typedef struct
{
  struct
  {
    int size_min;
    int next;
    int cnt;
    int idx[17];
    int poc[17];
  } dpb;

  int cnt;
  int cnt_max;
  int *frame;
} h264_dpb_t;

void DpbInit( h264_dpb_t *p )
{
  p->dpb.cnt = 0;
  p->dpb.next = 0;
  p->dpb.size_min = 0;

  p->cnt = 0;
  p->cnt_max = 0;
  p->frame = NULL;
}
void DpbClean( h264_dpb_t *p )
{
  free( p->frame );
}
static void DpbUpdate( h264_dpb_t *p, int is_forced )
{
  int i;
  int pos;
  if (!is_forced && p->dpb.cnt < 16)
    return;

  /* find the lowest poc */
  pos = 0;
  for (i = 1; i < p->dpb.cnt; i++)
  {
    if (p->dpb.poc[i] < p->dpb.poc[pos])
      pos = i;
  }
  //fprintf( stderr, "lowest=%d\n", pos );

  /* save the idx */
  if (p->cnt >= p->cnt_max)
  {
    p->cnt_max += 1000;
    p->frame = (int*)realloc( p->frame, sizeof(int)*p->cnt_max );
  }
  p->frame[p->cnt++] = p->dpb.idx[pos];

  /* Update the dpb minimal size */
  if (pos > p->dpb.size_min)
    p->dpb.size_min = pos;

  /* update dpb */
  for (i = pos; i < p->dpb.cnt-1; i++)
  {
    p->dpb.idx[i] = p->dpb.idx[i+1];
    p->dpb.poc[i] = p->dpb.poc[i+1];
  }
  p->dpb.cnt--;
}

void DpbFlush( h264_dpb_t *p )
{
  while (p->dpb.cnt > 0)
    DpbUpdate( p, true );
}

void DpbAdd( h264_dpb_t *p, int poc, int is_idr )
{
  if (is_idr)
    DpbFlush( p );

  p->dpb.idx[p->dpb.cnt] = p->dpb.next;
  p->dpb.poc[p->dpb.cnt] = poc;
  p->dpb.cnt++;
  p->dpb.next++;
  
  DpbUpdate( p, false );
}

int DpbFrameOffset( h264_dpb_t *p, int idx )
{
  if (idx >= p->cnt)
    return 0;

  return p->dpb.size_min + p->frame[idx] - idx;
}


static bool remove_unused_sei_messages (nal_reader_t *nal,
					uint32_t header_size)
{
  uint32_t buffer_on = header_size;
  buffer_on++; // increment past SEI message header

  while (buffer_on < nal->buffer_on) {
    uint32_t payload_type, payload_size, start, size;
    if (nal->buffer[buffer_on] == 0x80) {
      // rbsp_trailing_bits
      return true;
    }
    if (nal->buffer_on - buffer_on < 2) {
      fprintf(stderr, "extra bytes after SEI message\n");
      memset(nal->buffer + buffer_on, 0,
	     nal->buffer_on - buffer_on); 
      nal->buffer_on = buffer_on;
      
      return true;
    }
    start = buffer_on;
    payload_type = h264_read_sei_value(nal->buffer + buffer_on,
				       &size);
#ifdef DEBUG_H264
    printf("sei type %d size %d on %d\n", payload_type, 
	   size, buffer_on);
#endif
    buffer_on += size;
    payload_size = h264_read_sei_value(nal->buffer + buffer_on,
				       &size);
    buffer_on += size + payload_size;
#ifdef DEBUG_H264
    printf("sei size %d size %d on %d nal %d\n",
	   payload_size, size, buffer_on, nal->buffer_on);
#endif
    if (buffer_on > nal->buffer_on) {
      fprintf(stderr, "Error decoding sei message\n");
      return false;
    }
    switch (payload_type) {
    case 3:
    case 10:
    case 11:
    case 12:
      memmove(nal->buffer + start,
	      nal->buffer + buffer_on, 
	      nal->buffer_size - buffer_on);
      nal->buffer_size -= buffer_on - start;
      nal->buffer_on -= buffer_on - start;
      buffer_on = start;
      break;
    }
  }
  if (nal->buffer_on == header_size) return false;
  return true;
}

static bool RefreshReader (nal_reader_t *nal,
			   uint32_t nal_start)
{
  uint32_t bytes_left;
  uint32_t bytes_read;
#ifdef DEBUG_H264
  printf("refresh - start %u buffer on %u size %u\n", 
	 nal_start, nal->buffer_on, nal->buffer_size);
#endif
  if (nal_start != 0) {
    if (nal_start > nal->buffer_size) {
#ifdef DEBUG_H264
      printf("nal start is greater than buffer size\n");
#endif
      nal->buffer_on = 0;
    } else {
      bytes_left = nal->buffer_size - nal_start;
      if (bytes_left > 0) {
	memmove(nal->buffer, 
		nal->buffer + nal_start,
		bytes_left);
	nal->buffer_on -= nal_start;
#ifdef DEBUG_H264
	printf("move %u new on is %u\n", bytes_left, nal->buffer_on);
#endif
      } else {
	nal->buffer_on = 0;
      }
      nal->buffer_size = bytes_left;
    }
  } else {
    if (feof(nal->ifile)) {
      return false;
    }
    nal->buffer_size_max += 4096 * 4;
    nal->buffer = (uint8_t *)realloc(nal->buffer, nal->buffer_size_max);
  }
  bytes_read = fread(nal->buffer + nal->buffer_size,
		     1,
		     nal->buffer_size_max - nal->buffer_size,
		     nal->ifile);
  if (bytes_read == 0) return false;
#ifdef DEBUG_H264
  printf("read %u of %u\n", bytes_read, 
	  nal->buffer_size_max - nal->buffer_size);
#endif
  nal->buffer_size += bytes_read;
  return true;
}
	  
static bool LoadNal (nal_reader_t *nal)
{
  if (nal->buffer_on != 0 || nal->buffer_size == 0) {
    if (RefreshReader(nal, nal->buffer_on) == false) {
#ifdef DEBUG_H264
      printf("refresh returned 0 - buffer on is %u, size %u\n",
	     nal->buffer_on, nal->buffer_size);
#endif
      if (nal->buffer_on >= nal->buffer_size) return false;
      // continue
    }
  }
  // find start code
  uint32_t start;
  if (h264_is_start_code(nal->buffer) == false) {
    start = h264_find_next_start_code(nal->buffer,
				    nal->buffer_size);
    RefreshReader(nal, nal->buffer_on);
  }
  while ((start = h264_find_next_start_code(nal->buffer + 4,
					    nal->buffer_size - 4)) == 0) {
    if (RefreshReader(nal, 0) == false) {
      // end of file - use the last NAL
      nal->buffer_on = nal->buffer_size;
      return true;
    }
  }
  nal->buffer_on = start + 4;
  return true;
}

MP4TrackId H264Creator (MP4FileHandle mp4File, 
			FILE* inFile)
{
    bool rc;

    // the current syntactical object
    // typically 1:1 with a sample
    // but not always, i.e. non-VOP's
    // the current sample
    MP4SampleId sampleId = 1;

    // track configuration info
    uint8_t AVCProfileIndication = 0;
    uint8_t profile_compat = 0;
    uint8_t AVCLevelIndication = 0;
    // first thing we need to do is read the headers
    bool have_seq = false;
    uint8_t nal_type;
    nal_reader_t nal;
    h264_decode_t h264_dec;
    MP4Timestamp lastTime = 0, thisTime;
    MP4SampleId samplesWritten = 0;

    memset(&nal, 0, sizeof(nal));
    nal.ifile = inFile;

    if (VideoFrameRate == 0.0) {
      fprintf(stderr, "%s: Must specify frame rate when reading H.264 files", 
	      ProgName);
      return MP4_INVALID_TRACK_ID;
    }
    while (have_seq == false) {
      if (LoadNal(&nal) == false) {
	fprintf(stderr, "%s: Could not find sequence header\n", ProgName);
	return MP4_INVALID_TRACK_ID;
      }
      nal_type = h264_nal_unit_type(nal.buffer);
      if (nal_type == H264_NAL_TYPE_SEQ_PARAM) {
	have_seq = true;
	uint32_t offset;
	if (nal.buffer[2] == 1) offset = 3;
	else offset = 4;
	// skip the nal type byte
	AVCProfileIndication = nal.buffer[offset + 1];
	profile_compat = nal.buffer[offset + 2];
	AVCLevelIndication = nal.buffer[offset + 3];
	if (h264_read_seq_info(nal.buffer, nal.buffer_on, &h264_dec) == -1) {
	  fprintf(stderr, "%s: Could not decode Sequence header\n", ProgName);
	  return MP4_INVALID_TRACK_ID;
	}
      }
    }	

    rewind(nal.ifile);
    nal.buffer_size = 0;
    nal.buffer_on = 0;
    nal.buffer_size_max = 0;
    free(nal.buffer);
    nal.buffer = NULL;

    u_int32_t mp4FrameDuration = 0;
    
    mp4FrameDuration = (u_int32_t)(((double)Mp4TimeScale) / VideoFrameRate);

#if 0
    ismacryp_session_id_t ismaCrypSId;
    mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
    memset(icPp, 0, sizeof(mp4v2_ismacrypParams));

    // initialize ismacryp session if encrypting
    if (doEncrypt) {

        if (ismacrypInitSession(&ismaCrypSId,KeyTypeVideo) != 0) {
            fprintf(stderr, "%s: could not initialize the ISMAcryp session\n",
                    ProgName);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetScheme(ismaCrypSId, &(icPp->scheme_type)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp scheme type. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetSchemeVersion(ismaCrypSId, &(icPp->scheme_version)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp scheme ver. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetKMSUri(ismaCrypSId, &(icPp->kms_uri)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp kms uri. sid %d\n",
                    ProgName, ismaCrypSId);
            if (icPp->kms_uri != NULL) free(icPp->kms_uri);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if ( ismacrypGetSelectiveEncryption(ismaCrypSId, &(icPp->selective_enc)) != ismacryp_rc_ok ) {
            fprintf(stderr, "%s: could not get ismacryp selec enc. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetKeyIndicatorLength(ismaCrypSId, &(icPp->key_ind_len)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp key ind len. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
        if (ismacrypGetIVLength(ismaCrypSId, &(icPp->iv_len)) != ismacryp_rc_ok) {
            fprintf(stderr, "%s: could not get ismacryp iv len. sid %d\n",
                    ProgName, ismaCrypSId);
            ismacrypEndSession(ismaCrypSId);
            return MP4_INVALID_TRACK_ID;
        }
    }
#endif
    // create the new video track
    MP4TrackId trackId;
    trackId =
      MP4AddH264VideoTrack(mp4File,
			   Mp4TimeScale,
			   mp4FrameDuration,
			   h264_dec.pic_width,
			   h264_dec.pic_height,
			   AVCProfileIndication,
			   profile_compat, 
			   AVCLevelIndication,
			   3);

    if (trackId == MP4_INVALID_TRACK_ID) {
        fprintf(stderr,
                "%s: can't create video track\n", ProgName);
        return MP4_INVALID_TRACK_ID;
    }

    if (MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE) == 1) {
      MP4SetVideoProfileLevel(mp4File, 0x7f);
    }

    uint8_t *nal_buffer;
    uint32_t nal_buffer_size, nal_buffer_size_max;
    nal_buffer = NULL;
    nal_buffer_size = 0;
    nal_buffer_size_max = 0;
    bool first = true;
    bool nal_is_sync = false;
    bool slice_is_idr = false;
    int32_t poc = 0;
    h264_dpb_t h264_dpb;

    // now process the rest of the video stream
    memset(&h264_dec, 0, sizeof(h264_dec));
    DpbInit(&h264_dpb);

    while ( LoadNal(&nal) != false ) {
      uint32_t header_size;
      header_size = nal.buffer[2] == 1 ? 3 : 4;
      bool boundary = h264_detect_boundary(nal.buffer, 
					   nal.buffer_on,
					   &h264_dec);
      if (boundary && first == false) {
	// write the previous sample
	if (nal_buffer_size != 0) {
#ifdef DEBUG_H264
	  printf("sid %d writing %u\n", samplesWritten, 
		 nal_buffer_size);
#endif
	  samplesWritten++;
	  double thiscalc;
	  thiscalc = samplesWritten;
	  thiscalc *= Mp4TimeScale;
	  thiscalc /= VideoFrameRate;
	  
 	  thisTime = (MP4Duration)thiscalc;
	  MP4Duration dur;
	  dur = thisTime - lastTime;
	  rc = MP4WriteSample(mp4File, 
			      trackId,
			      nal_buffer,
			      nal_buffer_size,
			      dur,
			      0, 
			      nal_is_sync);

	  lastTime = thisTime;
	  if ( !rc ) {
	    fprintf(stderr,
		    "%s: can't write video frame %u\n",
		    ProgName, sampleId);
	    MP4DeleteTrack(mp4File, trackId);
	    return MP4_INVALID_TRACK_ID;
	  }
	  sampleId++;
          DpbAdd( &h264_dpb, poc, slice_is_idr );
	  nal_is_sync = false;
#ifdef DEBUG_H264
	  printf("wrote frame %d "U64"\n", nal_buffer_size, thisTime);
#endif
	  nal_buffer_size = 0;
	} 
      }
      first = false;
      bool copy_nal_to_buffer = false;
      if (Verbosity & MP4_DETAILS_SAMPLE) {
	printf("H264 type %x size %u\n",
                    h264_dec.nal_unit_type, nal.buffer_on);
      }
      if (h264_nal_unit_type_is_slice(h264_dec.nal_unit_type)) {
	copy_nal_to_buffer = true;
        slice_is_idr = h264_dec.nal_unit_type == H264_NAL_TYPE_IDR_SLICE;
        poc = h264_dec.pic_order_cnt;

	nal_is_sync = h264_slice_is_idr(&h264_dec);
      } else {
	switch (h264_dec.nal_unit_type) {
	case H264_NAL_TYPE_SEQ_PARAM:
	  // doesn't get added to sample buffer
	  // remove header
	  
	  
	  MP4AddH264SequenceParameterSet(mp4File, trackId, 
					 nal.buffer + header_size, 
					 nal.buffer_on - header_size);
	  break;
	case H264_NAL_TYPE_PIC_PARAM:
	  // doesn't get added to sample buffer
	  MP4AddH264PictureParameterSet(mp4File, trackId, 
					nal.buffer + header_size, 
					nal.buffer_on - header_size);
	  break;
	case H264_NAL_TYPE_FILLER_DATA:
	  // doesn't get copied
	  break;
	case H264_NAL_TYPE_SEI:
	  copy_nal_to_buffer = remove_unused_sei_messages(&nal, header_size);
	  break;
	case H264_NAL_TYPE_ACCESS_UNIT: 
	  // note - may not want to copy this - not needed
	default:
	  copy_nal_to_buffer = true;
	  break;
	}
      }
      if (copy_nal_to_buffer) {
	uint32_t to_write;
	to_write = nal.buffer_on - header_size;
	if (to_write + 4 + nal_buffer_size > nal_buffer_size_max) {
	  nal_buffer_size_max += nal.buffer_on + 4;
	  nal_buffer = (uint8_t *)realloc(nal_buffer, nal_buffer_size_max);
	}
	nal_buffer[nal_buffer_size] = (to_write >> 24) & 0xff;
	nal_buffer[nal_buffer_size + 1] = (to_write >> 16) & 0xff;
	nal_buffer[nal_buffer_size + 2] = (to_write >> 8)& 0xff;
	nal_buffer[nal_buffer_size + 3] = to_write & 0xff;
	memcpy(nal_buffer + nal_buffer_size + 4,
	       nal.buffer + header_size,
	       to_write);
#ifdef DEBUG_H264
	printf("copy nal - to_write %u offset %u total %u\n",
	       to_write, nal_buffer_size, nal_buffer_size + 4 + to_write);
	printf("header size %d bytes after %02x %02x\n", 
	       header_size, nal.buffer[header_size], 
	       nal.buffer[header_size + 1]);
#endif
	nal_buffer_size += to_write + 4;
      }

    }

    if (nal_buffer_size != 0) {
      samplesWritten++;
      double thiscalc;
      thiscalc = samplesWritten;
      thiscalc *= Mp4TimeScale;
      thiscalc /= VideoFrameRate;
      
      thisTime = (MP4Duration)thiscalc;
      MP4Duration dur;
      dur = thisTime - lastTime;
      rc = MP4WriteSample(mp4File, 
			  trackId,
			  nal_buffer,
			  nal_buffer_size,
			  dur,
			  0, 
			  nal_is_sync);
      if ( !rc ) {
	fprintf(stderr,
		"%s: can't write video frame %u\n",
		ProgName, sampleId);
	MP4DeleteTrack(mp4File, trackId);
	return MP4_INVALID_TRACK_ID;
      }
      DpbAdd(&h264_dpb, h264_dec.pic_order_cnt, slice_is_idr);
    }

    DpbFlush(&h264_dpb);

    if (h264_dpb.dpb.size_min > 0) {
      unsigned int ix;
      for (ix = 0; ix < samplesWritten; ix++) {
	const int offset = DpbFrameOffset(&h264_dpb, ix);
	MP4SetSampleRenderingOffset(mp4File, trackId, 1 + ix, 
				    offset * mp4FrameDuration);
      }
    }

    DpbClean(&h264_dpb);
#if 0
    // terminate session if encrypting
    if (doEncrypt) {
        if (ismacrypEndSession(ismaCrypSId) != 0) {
            fprintf(stderr,
                    "%s: could not end the ISMAcryp session\n",
                    ProgName);
        }
    }
#endif

    return trackId;
}

