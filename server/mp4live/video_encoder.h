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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include "media_codec.h"
#include "media_frame.h"
#include <sdp.h>
#include <mp4.h>

class CVideoEncoder : public CMediaCodec {
public:
	CVideoEncoder() { };

	virtual bool EncodeImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV,
		u_int32_t yStride, u_int32_t uvStride,
		bool wantKeyFrame = false) = NULL;

	virtual bool GetEncodedImage(
		u_int8_t** ppBuffer, u_int32_t* pBufferLength) = NULL;

	virtual bool GetReconstructedImage(
		u_int8_t* pY, u_int8_t* pU, u_int8_t* pV) = NULL;
	virtual media_free_f GetMediaFreeFunction(void) { return NULL; };
};

CVideoEncoder* VideoEncoderCreate(const char* encoderName);

MediaType get_video_mp4_fileinfo(CLiveConfig *pConfig,
				 bool *createIod,
				 bool *isma_compliant,
				 uint8_t *videoProfile,
				 uint8_t **videoConfig,
				 uint32_t *videoConfigLen,
				 uint8_t *mp4_video_type);

media_desc_t *create_video_sdp(CLiveConfig *pConfig,
			       bool *createIod,
			       bool *isma_compliant,
			       uint8_t *audioProfile,
			       uint8_t **audioConfig,
			       uint32_t *audioConfigLen);

void create_mp4_video_hint_track(CLiveConfig *pConfig,
				  MP4FileHandle mp4file,
				  MP4TrackId trackId);

typedef struct video_encoder_table_t {
  char *encoding_name;
  char *encoding;
  char *encoder;
  uint16_t numSizesNTSC;
  uint16_t numSizesPAL;
  uint16_t numSizesSecam;
  uint16_t *widthValuesNTSC;
  uint16_t *widthValuesPAL;
  uint16_t *widthValuesSecam;
  uint16_t *heightValuesNTSC;
  uint16_t *heightValuesPAL;
  uint16_t *heightValuesSecam;
  char **sizeNamesNTSC;
  char **sizeNamesPAL;
  char **sizeNamesSecam;
} video_encoder_table_t;

extern const video_encoder_table_t video_encoder_table[];
extern const uint32_t video_encoder_table_size;

#endif /* __VIDEO_ENCODER_H__ */

