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

#ifdef ADD_XVID_ENCODER
#ifdef HAVE_XVID10
#include "video_xvid10.h"
#else
#ifdef HAVE_XVID_H
#include "video_xvid.h"
#endif
#endif
#endif

#ifdef ADD_H26L_ENCODER
#include "video_h26l.h"
#endif

#ifdef HAVE_FFMPEG
#include "video_ffmpeg.h"
#endif

#include "h261/encoder-h261.h"
#include "rtp_transmitter.h"
CVideoEncoder* VideoEncoderCreateBase(CLiveConfig *pConfig)
{
  const char *encoderName = pConfig->GetStringValue(CONFIG_VIDEO_ENCODER);
	if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef HAVE_FFMPEG
		return new CFfmpegVideoEncoder();
#else
		error_message("ffmpeg encoder not available in this build");
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {

#if defined(HAVE_XVID10) || defined(HAVE_XVID_H)

#ifdef HAVE_XVID10
	  return new CXvid10VideoEncoder();
#else
		return new CXvidVideoEncoder();
#endif
#else
		error_message("xvid encoder not available in this build");
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_H26L)) {
#ifdef ADD_H26L_ENCODER
		return new CH26LVideoEncoder();
#else
		error_message("H.26L encoder not available in this build");
#endif
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_H261)) {
	  
	  CH261PixelEncoder *ret;
	  ret = new CH261PixelEncoder();
	  return ret;

	} else {
		error_message("unknown encoder specified");
	}

	return NULL;
}

MediaType get_video_mp4_fileinfo_base (CLiveConfig *pConfig,
				       bool *createIod,
				       bool *isma_compliant,
				       uint8_t *videoProfile,
				       uint8_t **videoConfig,
				       uint32_t *videoConfigLen,
				       uint8_t *mp4_video_type)
{
  const char *encodingName = pConfig->GetStringValue(CONFIG_VIDEO_ENCODING);
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
  }
  return UNDEFINEDFRAME;
}

media_desc_t *create_video_sdp_base(CLiveConfig *pConfig,
				    bool *createIod,
				    bool *isma_compliant,
				    uint8_t *videoProfile,
				    uint8_t **videoConfig,
				    uint32_t *videoConfigLen)
{
  // do the work here for mpeg4 - we know pretty much everything
  media_desc_t *sdpMediaVideo;
  format_list_t *sdpMediaVideoFormat;
  rtpmap_desc_t *sdpVideoRtpMap;
  char videoFmtpBuf[512];
  MediaType mtype;

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
  sdpMediaVideo->fmt = sdpMediaVideoFormat;

  if (mtype == MPEG4VIDEOFRAME) {
    sdpVideoRtpMap = MALLOC_STRUCTURE(rtpmap_desc_t);
    memset(sdpVideoRtpMap, 0, sizeof(*sdpVideoRtpMap));
    sdpMediaVideoFormat->media = sdpMediaVideo;
    sdpMediaVideoFormat->rtpmap = sdpVideoRtpMap;
    sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines, 
			   "a=mpeg4-esid:20");
#ifndef HAVE_XVID10
    // it's more complex than this, but tough.
    sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines,
			   "a=x-mpeg4-simple-profile-decoder");
#endif
    sdpMediaVideoFormat->fmt = strdup("96");
	
    sdpVideoRtpMap->encode_name = strdup("MP4V-ES");
    sdpVideoRtpMap->clock_rate = 90000;


    char* sConfig = MP4BinaryToBase16(pConfig->m_videoMpeg4Config, 
				    pConfig->m_videoMpeg4ConfigLength);

    sprintf(videoFmtpBuf, 
	    "profile-level-id=%u; config=%s;",
	    pConfig->m_videoMpeg4ProfileId,
	    sConfig); 
    free(sConfig);

    sdpMediaVideoFormat->fmt_param = strdup(videoFmtpBuf);
  } else if (mtype == H261VIDEOFRAME) {
    sdpMediaVideoFormat->fmt = strdup("31");
#if 0
    sdpVideoRtpMap->encode_name = strdup("h261");
    sdpVideoRtpMap->clock_rate = 90000;
#endif
  } else if (mtype == MPEG2VIDEOFRAME) {
    sdpMediaVideoFormat->fmt = strdup("32");
  } else if (mtype == H263VIDEOFRAME) {
    sdpVideoRtpMap = MALLOC_STRUCTURE(rtpmap_desc_t);
    memset(sdpVideoRtpMap, 0, sizeof(*sdpVideoRtpMap));
    sdpMediaVideoFormat->fmt = strdup("96");
    sdpMediaVideoFormat->media = sdpMediaVideo;
    sdpMediaVideoFormat->rtpmap = sdpVideoRtpMap;
    sdpVideoRtpMap->clock_rate = 90000;
    sdpVideoRtpMap->encode_name = strdup("H263-2000");
    char cliprect[80];
    sprintf(cliprect, "a=cliprect:0,0,%d,%d",
 	    pConfig->m_videoHeight, pConfig->m_videoWidth);
    sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines, cliprect);
  }

  return sdpMediaVideo;
}


void create_mp4_video_hint_track_base (CLiveConfig *pConfig,
				       MP4FileHandle mp4file,
				       MP4TrackId trackId)
{
  const char *encodingName = pConfig->GetStringValue(CONFIG_VIDEO_ENCODING);

  if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG4)) {
    MP4AV_Rfc3016Hinter(mp4file, 
			trackId,
			pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_MPEG2)) {
    Mpeg12Hinter(mp4file, 
		 trackId,
		 pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H263)) {
    MP4AV_Rfc2429Hinter(mp4file, 
			trackId, 
			pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
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
      rc -= sizeof(rtp_packet_header);
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
    rc -= sizeof(rtp_packet_header);
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
 
video_rtp_transmitter_f GetVideoRtpTransmitRoutineBase(CLiveConfig *pConfig,
						       MediaType *pType,
						       uint8_t *pPayload)
{
  const char *encodingName = pConfig->GetStringValue(CONFIG_VIDEO_ENCODING);
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
  }
  return NULL;
}
