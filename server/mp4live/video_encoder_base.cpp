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

#ifdef ADD_FFMPEG_ENCODER
#include "video_ffmpeg.h"
#endif

#ifdef ADD_XVID_ENCODER
#include "video_xvid.h"
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
#ifdef ADD_XVID_ENCODER
		return new CXvidVideoEncoder();
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
    *videoProfile = pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID);
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
  sdpVideoRtpMap = MALLOC_STRUCTURE(rtpmap_desc_t);
  memset(sdpVideoRtpMap, 0, sizeof(*sdpVideoRtpMap));
  sdpMediaVideoFormat->media = sdpMediaVideo;
  sdpMediaVideoFormat->rtpmap = sdpVideoRtpMap;

  if (mtype == MPEG4VIDEOFRAME) {
    sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines, 
			   strdup("a=mpeg4-esid:20"));
    sdpMediaVideoFormat->fmt = strdup("96");
	
    sdpVideoRtpMap->encode_name = strdup("MP4V-ES");
    sdpVideoRtpMap->clock_rate = 90000;


    char* sConfig = MP4BinaryToBase16(pConfig->m_videoMpeg4Config, 
				    pConfig->m_videoMpeg4ConfigLength);

    sprintf(videoFmtpBuf, 
	    "profile-level-id=%u; config=%s;",
	    pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_LEVEL_ID),
	    sConfig); 
    free(sConfig);

    sdpMediaVideoFormat->fmt_param = strdup(videoFmtpBuf);
  } else if (mtype == H261VIDEOFRAME) {
    sdpMediaVideoFormat->fmt = strdup("31");
    sdpVideoRtpMap->encode_name = strdup("h261");
    sdpVideoRtpMap->clock_rate = 90000;
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

  u_int8_t* pData = (u_int8_t*)pFrame->GetData();
  u_int32_t bytesToSend = pFrame->GetDataLength();
  struct iovec iov;

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
      rdptr->send_iov(&iov, 1, rtpTimestamp, lastPacket);
      rdptr = rdptr->get_next();
    }

    pData += payloadLength;
    bytesToSend -= payloadLength;
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
  } else if (!strcasecmp(encodingName, VIDEO_ENCODING_H261)) {
    *pPayload = 31;
    *pType = H261VIDEOFRAME;
    return H261SendVideo;
  }
  return NULL;
}
