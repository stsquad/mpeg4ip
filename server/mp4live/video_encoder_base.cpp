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

#include "mp4live.h"
#include "video_encoder.h"
#include "video_encoder_base.h"
#include "mp4av.h"
#include "mp4av_h264.h"

#ifdef HAVE_XVID10
#include "video_xvid10.h"
#else
#ifdef HAVE_XVID_H
#include "video_xvid.h"
#endif
#endif

#ifdef HAVE_FFMPEG
#include "video_ffmpeg.h"
#endif

#ifdef HAVE_X264
#include "video_x264.h"
#endif

#include "h261/encoder-h261.h"
#include "rtp_transmitter.h"

void VideoProfileCheckBase (CVideoProfile *vp)
{
  const char *encoderName = vp->GetStringValue(CFG_VIDEO_ENCODER);
  if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef HAVE_FFMPEG
    return;
#else
    error_message("Profile:%s ffmpeg encoder not available in this build", 
		  vp->GetName());
#if defined(HAVE_XVID10) || defined(HAVE_XVID_H)
    error_message("It has been changed to xvid");
    vp->SetStringValue(CFG_VIDEO_ENCODER, VIDEO_ENCODER_XVID);
    return;
#endif
#endif
  } else if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {

#if defined(HAVE_XVID10) || defined(HAVE_XVID_H)
    return;
#else
    error_message("Profile %s:xvid encoder not available in this build",
		  vp->GetName());
#ifdef HAVE_FFMPEG
    error_message("It has been changed to ffmpeg");
    vp->SetStringValue(CFG_VIDEO_ENCODER, VIDEO_ENCODER_FFMPEG);
    return;
#endif
#endif
  } else if (!strcasecmp(encoderName, VIDEO_ENCODER_X264)) {
#ifdef HAVE_X264
    return;
#else
    error_message("Profile %s:X264 encoder is not available in this build", 
		  vp->GetName());
#endif
  } else if (!strcasecmp(encoderName, VIDEO_ENCODER_H261)) {
    return;
  } else {
    error_message("Profile %s: encoder %s not found", vp->GetName(), encoderName);
  }
  // if we reach here, we want to set the h.261 encoder
  error_message("It has been changed to H.261");
  vp->SetStringValue(CFG_VIDEO_ENCODER, VIDEO_ENCODER_H261);
  vp->SetStringValue(CFG_VIDEO_ENCODING, VIDEO_ENCODING_H261);
  vp->SetIntegerValue(CFG_VIDEO_WIDTH, 352);
  vp->SetIntegerValue(CFG_VIDEO_HEIGHT, 288);
  // called from update, so don't sweat update.
}
  
CVideoEncoder* VideoEncoderCreateBase(CVideoProfile *vp,
				      uint16_t mtu,
				      CVideoEncoder *next, 
				      bool realTime)
{
  const char *encoderName = vp->GetStringValue(CFG_VIDEO_ENCODER);
	if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef HAVE_FFMPEG
		return new CFfmpegVideoEncoder(vp, mtu, next, realTime);
#else
		error_message("ffmpeg encoder not available in this build");
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {

#if defined(HAVE_XVID10) || defined(HAVE_XVID_H)

#ifdef HAVE_XVID10
	  return new CXvid10VideoEncoder(vp, mtu, next, realTime);
#else
		return new CXvidVideoEncoder(vp, mtu, next, realTime);
#endif
#else
		error_message("xvid encoder not available in this build");
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_X264)) {
#ifdef HAVE_X264
	  return new CX264VideoEncoder(vp, mtu, next, realTime);
#else
	  error_message("X264 encoder is not available in this build");
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_H261)) {
	  
	  CH261PixelEncoder *ret;
	  ret = new CH261PixelEncoder(vp, mtu, next, realTime);
	  return ret;

	} else {
		error_message("unknown encoder specified");
	}

	return NULL;
}

void AddVideoProfileEncoderVariablesBase (CVideoProfile *pConfig)
{
#ifdef HAVE_XVID10
  AddXvid10ConfigVariables(pConfig);
#endif
#ifdef HAVE_X264
  AddX264ConfigVariables(pConfig);
#endif
#ifdef HAVE_FFMPEG
  AddFfmpegConfigVariables(pConfig);
#endif
  AddH261ConfigVariables(pConfig);
}
MediaType get_video_mp4_fileinfo_base (CVideoProfile *pConfig,
				       bool *createIod,
				       bool *isma_compliant,
				       uint8_t *videoProfile,
				       uint8_t **videoConfig,
				       uint32_t *videoConfigLen,
				       uint8_t *mp4_video_type)
{
  const char *encodingName = pConfig->GetStringValue(CFG_VIDEO_ENCODING);
  if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG4)) {
    *createIod = true;
    *isma_compliant = true;
    *videoProfile = pConfig->m_videoMpeg4ProfileId;
    *videoConfig = pConfig->m_videoMpeg4Config;
    *videoConfigLen = pConfig->m_videoMpeg4ConfigLength;

    if (mp4_video_type) {
      *mp4_video_type = MP4_MPEG4_VIDEO_TYPE;
    }
    return MPEG4VIDEOFRAME;
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H261)) {
      *createIod = false;
      *isma_compliant = false;
      *videoProfile = 0xff;
      *videoConfig = NULL;
      *videoConfigLen = 0;
    return H261VIDEOFRAME;
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG2)) {
    *createIod = false;
    *isma_compliant = false;
    *videoProfile = 0xff;
    *videoConfig = NULL;
    *videoConfigLen = 0;
    if (mp4_video_type) {
      *mp4_video_type = MP4_MPEG2_VIDEO_TYPE;
    }
    return MPEG2VIDEOFRAME;
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H263)) {
    *createIod = false;
    *isma_compliant = false;
    *videoProfile = 0xff;
    *videoConfig = NULL;
    *videoConfigLen = 0;
    return H263VIDEOFRAME;
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H264)) { 
    *createIod = false;
    *isma_compliant = false;
    *videoProfile = pConfig->m_videoMpeg4ProfileId;
    *videoConfig = pConfig->m_videoMpeg4Config;
    *videoConfigLen = 0;
    return H264VIDEOFRAME;
  }
  return UNDEFINEDFRAME;
}

media_desc_t *create_video_sdp_base(CVideoProfile *pConfig,
				    bool *createIod,
				    bool *isma_compliant,
				    bool *is3gp,
				    uint8_t *videoProfile,
				    uint8_t **videoConfig,
				    uint32_t *videoConfigLen,
				    uint8_t *pPayload_number)
{
  // do the work here for mpeg4 - we know pretty much everything
  media_desc_t *sdpMediaVideo;
  format_list_t *sdpMediaVideoFormat;
  char videoFmtpBuf[512];
  MediaType mtype;
  bool add_to_payload = true;
  uint8_t payload_number = pPayload_number == NULL ? 96 : *pPayload_number;

  mtype = get_video_mp4_fileinfo(pConfig,
				 createIod,
				 isma_compliant,
				 videoProfile,
				 videoConfig,
				 videoConfigLen,
				 NULL);

  sdpMediaVideo = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaVideo, 0, sizeof(*sdpMediaVideo));

  sdpMediaVideoFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaVideoFormat, 0, sizeof(*sdpMediaVideoFormat));
  sdpMediaVideo->fmt_list = sdpMediaVideoFormat;

  if (mtype == MPEG4VIDEOFRAME) {
    *is3gp = true;
    sdpMediaVideoFormat->media = sdpMediaVideo;
    sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines, 
			   "a=mpeg4-esid:20");
    sdpMediaVideoFormat->fmt = create_payload_number_string(payload_number);
	
    sdpMediaVideoFormat->rtpmap_name = strdup("MP4V-ES");
    sdpMediaVideoFormat->rtpmap_clock_rate = 90000;


    char* sConfig = MP4BinaryToBase16(*videoConfig,
				      *videoConfigLen);

    sprintf(videoFmtpBuf, 
	    "profile-level-id=%u; config=%s;",
	    *videoProfile,
	    sConfig); 
    free(sConfig);

    sdpMediaVideoFormat->fmt_param = strdup(videoFmtpBuf);
  } else if (mtype == H261VIDEOFRAME) {
    sdpMediaVideoFormat->fmt = create_payload_number_string(31);
    add_to_payload = false;
#if 0
    sdpMediaVideoFormat->encode_name = strdup("h261");
    sdpMediaVideoFormat->clock_rate = 90000;
#endif
  } else if (mtype == MPEG2VIDEOFRAME) {
    sdpMediaVideoFormat->fmt = create_payload_number_string(32);
    add_to_payload = false;
  } else if (mtype == H263VIDEOFRAME) {
    *is3gp = true;
    sdpMediaVideoFormat->fmt = create_payload_number_string(payload_number);
    sdpMediaVideoFormat->media = sdpMediaVideo;
    sdpMediaVideoFormat->rtpmap_clock_rate = 90000;
    sdpMediaVideoFormat->rtpmap_name = strdup("H263-2000");
    char cliprect[80];
    sprintf(cliprect, "a=cliprect:0,0,%d,%d",
 	    pConfig->GetIntegerValue(CFG_VIDEO_HEIGHT), 
	    pConfig->GetIntegerValue(CFG_VIDEO_WIDTH));
    sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines, cliprect);
  } else if (mtype == H264VIDEOFRAME) {
    sdpMediaVideoFormat->fmt = create_payload_number_string(payload_number);
    sdpMediaVideoFormat->media = sdpMediaVideo;
    sdpMediaVideoFormat->rtpmap_clock_rate = 90000;
    sdpMediaVideoFormat->rtpmap_name = strdup("H264");
    sprintf(videoFmtpBuf, 
	    "profile-level-id=%06x; sprop-parameter-sets=%s; packetization-mode=1",
	    *videoProfile,
	    (char *)*videoConfig);
    sdpMediaVideoFormat->fmt_param = strdup(videoFmtpBuf);
    
  }

  if (add_to_payload && pPayload_number != NULL) {
    *pPayload_number = *pPayload_number + 1;
  }
  return sdpMediaVideo;
}


void create_mp4_video_hint_track_base (CVideoProfile *pConfig,
				       MP4FileHandle mp4file,
				       MP4TrackId trackId,
				       uint16_t mtu)
{
  const char *encodingName = pConfig->GetStringValue(CFG_VIDEO_ENCODING);

  if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG4)) {
    MP4AV_Rfc3016Hinter(mp4file, 
			trackId,
			mtu);
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG2)) {
    Mpeg12Hinter(mp4file, 
		 trackId,
		 mtu);
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H263)) {
    MP4AV_Rfc2429Hinter(mp4file, 
			trackId, 
			mtu);
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H264)) {
    MP4AV_H264Hinter(mp4file, trackId, mtu);
  }

}

static void H261SendVideo (CMediaFrame *pFrame, CRtpDestination *list,
			   uint32_t rtpTimestamp,
			   uint16_t mtu)
{
  CRtpDestination *rdptr;
  
  pktbuf_t *pData = (pktbuf_t*)pFrame->GetData();
  struct iovec iov[2];
  while (pData != NULL) {
    iov[0].iov_base = &pData->h261_rtp_hdr;
    iov[0].iov_len = sizeof(uint32_t);
    iov[1].iov_base = pData->data;
    iov[1].iov_len = pData->len;
    
    rdptr = list;
    while (rdptr != NULL) {
      //error_message("h.261 - sending %d", pData->len + 4);
      rdptr->send_iov(iov, 2, rtpTimestamp, pData->next == NULL);
      rdptr = rdptr->get_next();
    }
   
    pktbuf_t *n = pData->next;
    pData = n;
  }
  if (pFrame->RemoveReference())
    delete pFrame;
}

static void Mpeg43016SendVideo (CMediaFrame *pFrame, CRtpDestination *list,
				uint32_t rtpTimestamp,
				uint16_t mtu)
{
  CRtpDestination *rdptr;

  u_int8_t* pData;
  u_int32_t bytesToSend = pFrame->GetDataLength();
  struct iovec iov;
  // This will remove any headers other than the VOP header, if a VOP
  // header appears in the stream
  pData = MP4AV_Mpeg4FindVop((uint8_t *)pFrame->GetData(),
			     bytesToSend);
  if (pData) {
    bytesToSend -= (pData - (uint8_t *)pFrame->GetData());
  } else {
    pData = (uint8_t *)pFrame->GetData();
  }

  while (bytesToSend) {
    u_int32_t payloadLength;
    bool lastPacket;

    if (bytesToSend <= mtu) {
      payloadLength = bytesToSend;
      lastPacket = true;
    } else {
      payloadLength = mtu;
      lastPacket = false;
    }

    iov.iov_base = pData;
    iov.iov_len = payloadLength;

    rdptr = list;
    while (rdptr != NULL) {
      int rc = rdptr->send_iov(&iov, 1, rtpTimestamp, lastPacket);
      if (rc != (int)payloadLength) {
	error_message("send_iov error - returned %d %d", rc, payloadLength);
      }
      rdptr = rdptr->get_next();
    }

    pData += payloadLength;
    bytesToSend -= payloadLength;
  }
  if (pFrame->RemoveReference())
    delete pFrame;
}

static void Mpeg2SendVideo (CMediaFrame *pFrame, 
			    CRtpDestination *list,
			    uint32_t rtpTimestamp,
			    uint16_t maxPayloadSize)
{
  uint8_t rfc2250[4], rfc2250_2;
  uint32_t sampleSize;
  uint8_t *pData, *pbuffer;
  uint32_t scode;
  int have_seq;
  bool stop;
  uint32_t offset;
  uint8_t *pstart;
  uint8_t type;
  uint32_t next_slice, prev_slice;
  bool slice_at_begin;
  CRtpDestination *rdptr;

  pData = (uint8_t *)pFrame->GetData();
  sampleSize = pFrame->GetDataLength();

  offset = 0;
  have_seq = 0;
  pbuffer = pData;
  stop = false;
  do {
    uint32_t oldoffset;
    oldoffset = offset;
    if (MP4AV_Mpeg3FindNextStart(pData + offset, 
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

  prev_slice = 0;
  if (MP4AV_Mpeg3FindNextSliceStart(pbuffer, offset, sampleSize, &next_slice) < 0) {
    slice_at_begin = false;
  } else {
    slice_at_begin = true;
  }
  offset = 0;
  bool nomoreslices = false;
  bool found_slice = slice_at_begin;
  bool onfirst = true;
  
  maxPayloadSize -= sizeof(rfc2250); // make sure we have room
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
      
  struct iovec iov[2];
  iov[0].iov_base = rfc2250;
  iov[0].iov_len = sizeof(rfc2250);
  iov[1].iov_base = pData + offset;
  iov[1].iov_len = len_to_write;
  rdptr = list;
  while (rdptr != NULL) {
    int rc = rdptr->send_iov(iov, 2, rtpTimestamp, 
			     len_to_write >= sampleSize ? 1 : 0);
    rc -= sizeof(rfc2250);
    if (rc != (int)len_to_write) {
      error_message("send_iov error - returned %d %d", rc, len_to_write);
    }
    rdptr = rdptr->get_next();
  }

  offset += len_to_write;
  sampleSize -= len_to_write;
  prev_slice = 0;
  next_slice -= len_to_write;
  pbuffer += len_to_write;

  }

  if (pFrame->RemoveReference())
    delete pFrame;
}

// we're going to assume that we get complete frames here...
static void H263SendVideoRfc2429 (CMediaFrame *pFrame, CRtpDestination *list,
				  uint32_t rtpTimestamp,
				  uint16_t mtu)
{
  struct iovec iov[2];
  uint8_t* pBuf = (uint8_t*)pFrame->GetData();
  uint32_t dataLength = pFrame->GetDataLength();
  uint8_t mode_header[2];
  bool first = true;
  uint32_t tosend;

  if (pBuf[0] == 0 &&
      pBuf[1] == 0 && 
      (pBuf[2] & 0xfc) == 0x80) {
    first = true;
  } else {
    first = false;
  }

  mtu -= 2; // subtract off header.

  while (dataLength > 0) {
    if (first) {
      pBuf += 2;
      dataLength -= 2;
      mode_header[0] = 0x4;
      first = false;
    } else 
      mode_header[0] = 0;

    mode_header[1] = 0;

    iov[0].iov_base = mode_header;
    iov[0].iov_len = 2;

    tosend = MIN(mtu, dataLength);
    iov[1].iov_base = pBuf;
    iov[1].iov_len = tosend;

    pBuf += tosend;
    dataLength -= tosend;

    //error_message("sending %d", iov[1].iov_len);
    CRtpDestination *rdptr = list;
    while (rdptr != NULL) {
      rdptr->send_iov(iov, 2, rtpTimestamp, dataLength > 0 ? 0 : 1);
      rdptr = rdptr->get_next();
    }
  }
  if (pFrame->RemoveReference())
    delete pFrame;
}

/*
 * H264SendVideo - send h264 video according to rfc proposal
 */
static void H264SendVideo (CMediaFrame *pFrame, CRtpDestination *list,
			   uint32_t rtpTimestamp,
			   uint16_t mtu)
{
  h264_media_frame_t *mf = (h264_media_frame_t *)pFrame->GetData();
  uint32_t nal_on = 0;
  struct iovec iov[32];
  //#define DEBUG_H264_TX 1
#ifdef DEBUG_H264_TX
  debug_message("send h264 - %u nals", mf->nal_number);
#endif
  while (nal_on < mf->nal_number) {
    uint32_t nal_len = mf->nal_bufs[nal_on].nal_length;
    const uint8_t *nal_ptr = mf->buffer + mf->nal_bufs[nal_on].nal_offset;
    /*
     * We have 3 types of packets we can send:
     * fragmentation units - if the NAL is > mtu
     * single nal units - if the NAL is < mtu, and can only fit 1 NAL
     * single time aggregation units - if we can put multiple NALs 
     *
     * We don't send multiple time aggregation units
     */
    bool have_non_unique_seq_or_pic;
#if 1
    have_non_unique_seq_or_pic = 
      (mf->nal_bufs[nal_on].unique == false &&
       (mf->nal_bufs[nal_on].nal_type == H264_NAL_TYPE_SEQ_PARAM ||
	 mf->nal_bufs[nal_on].nal_type == H264_NAL_TYPE_PIC_PARAM));
#else
    have_non_unique_seq_or_pic = false;
#endif
    if (have_non_unique_seq_or_pic) {
      nal_on++;
    } else if (nal_len > mtu) {
      // fragmentation unit - break up into mtu size chunks
#ifdef DEBUG_H264_TX
      debug_message("%u fu %u", nal_on, nal_len);
#endif
      uint8_t header[2], head;
      header[0] = (*nal_ptr & 0x60) | 28;
      header[1] = 0x80; // s indication
      head = *nal_ptr & 0x1f;
      nal_ptr++; // remove the first byte
      nal_len--;
      // increment nal_on, so we can compare against mf->nal_number for end
      // of buffer
      nal_on++; 
      while (nal_len > 0) {
	uint32_t write_size;
	bool last = false;
	header[1] |= head;
	if ((nal_len + 2) <= mtu) {
	  header[1] |= 0x40;
	  write_size = nal_len;
	  last = true;
	} else {
	  write_size = mtu - 2;
	}
	iov[0].iov_base = header;
	iov[0].iov_len = 2;
	iov[1].iov_base = (void *)nal_ptr;
	iov[1].iov_len = write_size;
	// send
#ifdef DEBUG_H264_TX
	debug_message("frag %u %u %02x %02x", write_size, 
		      (last && nal_on >= mf->nal_number) ? 1 : 0,
		      header[0], header[1]);
#endif
		      
	CRtpDestination *rdptr = list;
	while (rdptr != NULL) {
	  rdptr->send_iov(iov, 2, rtpTimestamp, 
			  (last && nal_on >= mf->nal_number) ? 1 : 0);
	  rdptr = rdptr->get_next();
	}
	header[1] = 0;
	nal_ptr += write_size;
	nal_len -= write_size;
      } // end while (nal_len > 0)
    } else if (((nal_on + 1) >= mf->nal_number)  ||
	       ((nal_len + mf->nal_bufs[nal_on].nal_length + 5) > mtu)) {
      // single nal unit packet
      iov[0].iov_base = (void *)nal_ptr;
      iov[0].iov_len = nal_len;
      nal_on++; // needs to be before, so we trigger on M bit setting
#ifdef DEBUG_H264_TX
      debug_message("%u snup %u %u", nal_on, nal_len, 
		    nal_on >= mf->nal_number ? 1 : 0);
#endif
      CRtpDestination *rdptr = list;
      while (rdptr != NULL) {
	rdptr->send_iov(iov, 1, rtpTimestamp, 
			nal_on >= mf->nal_number ? 1 : 0);
	rdptr = rdptr->get_next();
      }
    } else {
      // single time aggregation packet (stap) - first check how
      // many nals we want to put into the packet
      uint32_t nal_check = nal_on;
      uint32_t size = 1;
      do {
	size += 2 + mf->nal_bufs[nal_check].nal_length;
	nal_check++;
      } while (nal_check < mf->nal_number && size < mtu);
#ifdef DEBUG_H264_TX
      debug_message("nal on %u check %u size %u", 
		    nal_on, nal_check, size);
#endif
      if (size > mtu) nal_check--;
      if (nal_check - nal_on > 15) {
	// so we fit in iov
	nal_check = nal_on + 15;
      }
      uint8_t stap = 24;
      uint8_t max_nri = 0;
      // first, put the stap header
      iov[0].iov_base = &stap;
      iov[0].iov_len = 1;
      uint8_t lens[30];
      uint32_t iov_on = 1;
      uint len_on = 0;
      while (nal_on < nal_check) {
	nal_len = mf->nal_bufs[nal_on].nal_length;
	nal_ptr = mf->buffer + mf->nal_bufs[nal_on].nal_offset;
	// ready the length
	lens[len_on] = nal_len >> 8;
	lens[len_on + 1] = nal_len & 0xff;
	iov[iov_on].iov_base = &lens[len_on];
	len_on += 2;
	iov[iov_on].iov_len = 2;
	iov_on++;
	// then store the nal
	iov[iov_on].iov_base = (void *)nal_ptr;
	iov[iov_on].iov_len = nal_len;
	if ((*nal_ptr & 0x60) > max_nri) max_nri = *nal_ptr & 0x60;
#ifdef DEBUG_H264_TX
	debug_message("%u stap %u", nal_on, nal_len);
#endif
	nal_on++;
	iov_on++;
      }
      // set the nri value in the stap header
      stap |= max_nri;
#ifdef DEBUG_H264_TX
      debug_message("stap %u %u", iov_on,
		      nal_on >= mf->nal_number ? 1 : 0);
#endif
      // send the packet
      CRtpDestination *rdptr = list;
      while (rdptr != NULL) {
	rdptr->send_iov(iov, iov_on, rtpTimestamp, 
			nal_on >= mf->nal_number ? 1 : 0);
	rdptr = rdptr->get_next();
      }
    }
  }
		 
  if (pFrame->RemoveReference())
    delete pFrame;
}

static void DummySendVideo (CMediaFrame *pFrame, CRtpDestination *list,
			    uint32_t rtpTimestamp,
			    uint16_t mtu)
{
  if (pFrame->RemoveReference())
    delete pFrame;
}
 
rtp_transmitter_f GetVideoRtpTransmitRoutineBase(CVideoProfile *pConfig,
						 MediaType *pType,
						 uint8_t *pPayload)
{
  const char *encodingName = pConfig->GetStringValue(CFG_VIDEO_ENCODING);
  if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG4)) {
    *pType = MPEG4VIDEOFRAME;
    *pPayload = 96;
    return Mpeg43016SendVideo;
  } else if (strcasecmp(encodingName, VIDEO_ENCODING_H261) == 0) {
    *pPayload = 31;
    *pType = H261VIDEOFRAME;
    return H261SendVideo;
  } else if (strcasecmp(encodingName, VIDEO_ENCODING_MPEG2) == 0) {
    *pPayload =32;
    *pType = MPEG2VIDEOFRAME;
    return Mpeg2SendVideo;
  } else if (strcasecmp(encodingName, VIDEO_ENCODING_H263) == 0) {
    *pPayload = 96;
    *pType = H263VIDEOFRAME;
    return H263SendVideoRfc2429;
  } else if (strcasecmp(encodingName, VIDEO_ENCODING_H264) == 0) {
    *pPayload = 96;
    *pType = H264VIDEOFRAME;
    return H264SendVideo;
  }

  return DummySendVideo;
}
