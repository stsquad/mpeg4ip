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

CVideoEncoder* VideoEncoderCreate(CLiveConfig *pConfig)
{
  return VideoEncoderCreateBase(pConfig);
}

MediaType get_video_mp4_fileinfo (CLiveConfig *pConfig,
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

media_desc_t *create_video_sdp (CLiveConfig *pConfig,
				bool *createIod,
				bool *isma_compliant,
				uint8_t *videoProfile,
				uint8_t **videoConfig,
				uint32_t *videoConfigLen)
{
 return create_video_sdp_base(pConfig,
			      createIod,
			      isma_compliant,
			      videoProfile,
			      videoConfig,
			      videoConfigLen);
}


void create_mp4_video_hint_track (CLiveConfig *pConfig,
				  MP4FileHandle mp4file,
				  MP4TrackId trackId)
{
  return create_mp4_video_hint_track_base(pConfig, mp4file, trackId);
}

video_rtp_transmitter_f GetVideoRtpTransmitRoutine (CLiveConfig *pConfig,
						    MediaType *pType,
						    uint8_t *pPayload)
{
  return GetVideoRtpTransmitRoutineBase(pConfig, pType, pPayload);
}
