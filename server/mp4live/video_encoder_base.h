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
 * Copyright (C) Cisco Systems Inc. 2003.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */

#ifndef __VIDEO_ENCODER_BASE_H__
#define __VIDEO_ENCODER_H__

#include "media_codec.h"
#include "media_frame.h"
#include <sdp.h>
#include <mp4.h>
#include "video_encoder.h"

void VideoProfileCheckBase(CVideoProfile *vp);

CVideoEncoder* VideoEncoderCreateBase(CVideoProfile *vp, 
				      uint16_t mtu,
				      CVideoEncoder *next, 
				      bool realTime = true);

MediaType get_video_mp4_fileinfo_base(CVideoProfile *pConfig,
				      bool *createIod,
				      bool *isma_compliant,
				      uint8_t *videoProfile,
				      uint8_t **videoConfig,
				      uint32_t *videoConfigLen,
				      uint8_t *mp4_video_type);

media_desc_t *create_video_sdp_base(CVideoProfile *pConfig,
				    bool *createIod,
				    bool *isma_compliant,
				    bool *is3gp,
				    uint8_t *videoProfile,
				    uint8_t **videoConfig,
				    uint32_t *videoConfigLen,
				    uint8_t *payload_number);

void create_mp4_video_hint_track_base(CVideoProfile *pConfig,
				      MP4FileHandle mp4file,
				      MP4TrackId trackId,
				      uint16_t mtu);


rtp_transmitter_f GetVideoRtpTransmitRoutineBase(CVideoProfile *pConfig,
						 MediaType *pType,
						 uint8_t *pPayload);

void AddVideoProfileEncoderVariablesBase(CVideoProfile *pConfig);
#endif /* __VIDEO_ENCODER_BASE_H__ */

