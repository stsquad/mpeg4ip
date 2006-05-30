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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *              Bill May  wmay@cisco.com
 */

#include "mp4live.h"
#include "video_encoder.h"
#include "video_encoder_base.h"
#include "video_util_filter.h"

void VideoProfileCheck (CVideoProfile *vp)
{
  return VideoProfileCheckBase(vp);
}

CVideoEncoder* VideoEncoderCreate(CVideoProfile *vp, 
				  uint16_t mtu,
				  CVideoEncoder *next, 
				  bool realTime)
{
  return VideoEncoderCreateBase(vp, mtu, next, realTime);
}
void AddVideoProfileEncoderVariables(CVideoProfile *pConfig)
{
  return AddVideoProfileEncoderVariablesBase(pConfig);
}

MediaType get_video_mp4_fileinfo (CVideoProfile *pConfig,
				  bool *createIod,
				  bool *isma_compliant,
				  uint8_t *videoProfile,
				  uint8_t **videoConfig,
				  uint32_t *videoConfigLen,
				  uint8_t *mp4_video_type)
{
  return get_video_mp4_fileinfo_base(pConfig, 
				     createIod, 
				     isma_compliant, 
				     videoProfile, 
				     videoConfig, 
				     videoConfigLen, 
				     mp4_video_type);
}

media_desc_t *create_video_sdp (CVideoProfile *pConfig,
				bool *createIod,
				bool *isma_compliant,
				bool *is3gp,
				uint8_t *videoProfile,
				uint8_t **videoConfig,
				uint32_t *videoConfigLen,
				uint8_t *payload_number)
{
 return create_video_sdp_base(pConfig,
			      createIod,
			      isma_compliant,
			      is3gp,
			      videoProfile,
			      videoConfig,
			      videoConfigLen,
			      payload_number);
}


void create_mp4_video_hint_track (CVideoProfile *pConfig,
				  MP4FileHandle mp4file,
				  MP4TrackId trackId,
				  uint16_t mtu)
{
  return create_mp4_video_hint_track_base(pConfig, mp4file, trackId, mtu);
}

rtp_transmitter_f GetVideoRtpTransmitRoutine (CVideoProfile *pConfig,
					      MediaType *pType,
					      uint8_t *pPayload)
{
  return GetVideoRtpTransmitRoutineBase(pConfig, pType, pPayload);
}

