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

