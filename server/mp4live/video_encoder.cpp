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

CVideoEncoder* VideoEncoderCreate(const char* encoderName)
{
	if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
#ifdef ADD_FFMPEG_ENCODER
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
	} else {
		error_message("unknown encoder specified");
	}

	return NULL;
}

MediaType get_video_mp4_fileinfo (CLiveConfig *pConfig,
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
  }
  return UNDEFINEDFRAME;
}
media_desc_t *create_video_sdp(CLiveConfig *pConfig,
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

  get_video_mp4_fileinfo(pConfig,
			 createIod,
			 isma_compliant,
			 videoProfile,
			 videoConfig,
			 videoConfigLen,
			 NULL);

  sdpMediaVideo = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaVideo, 0, sizeof(*sdpMediaVideo));

  sdp_add_string_to_list(&sdpMediaVideo->unparsed_a_lines, 
			 strdup("a=mpeg4-esid:20"));
  sdpMediaVideoFormat = MALLOC_STRUCTURE(format_list_t);
  sdpMediaVideo->fmt = sdpMediaVideoFormat;
  memset(sdpMediaVideoFormat, 0, sizeof(*sdpMediaVideoFormat));
  sdpMediaVideoFormat->media = sdpMediaVideo;
  sdpMediaVideoFormat->fmt = strdup("96");
	
  sdpVideoRtpMap = MALLOC_STRUCTURE(rtpmap_desc_t);
  memset(sdpVideoRtpMap, 0, sizeof(*sdpVideoRtpMap));
  sdpVideoRtpMap->encode_name = strdup("MP4V-ES");
  sdpVideoRtpMap->clock_rate = 90000;

  sdpMediaVideoFormat->rtpmap = sdpVideoRtpMap;

  char* sConfig = MP4BinaryToBase16(pConfig->m_videoMpeg4Config, 
				    pConfig->m_videoMpeg4ConfigLength);

  sprintf(videoFmtpBuf, 
	  "profile-level-id=%u; config=%s;",
	  pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_LEVEL_ID),
	  sConfig); 
  free(sConfig);

  sdpMediaVideoFormat->fmt_param = strdup(videoFmtpBuf);

  return sdpMediaVideo;
}


void create_mp4_video_hint_track (CLiveConfig *pConfig,
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
