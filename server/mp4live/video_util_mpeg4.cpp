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

#include "mp4live.h"
#include <mp4av.h>
#include "profile_video.h"
#include "video_encoder.h"

void GenerateMpeg4VideoConfig(CVideoProfile *pConfig)
{
  CVideoEncoder *pVidEncoder = VideoEncoderCreate(pConfig, 
						  1460,
						  NULL, 
						  false);

  CHECK_AND_FREE(pConfig->m_videoMpeg4Config);
  if (pVidEncoder) {
    if (pVidEncoder->CanGetEsConfig()) {
      if (pVidEncoder->Init() == false) {
	error_message("Couldn't init encoder for VOL setting");
	delete pVidEncoder;
	return;
      }
      if (pVidEncoder->GetEsConfig(&pConfig->m_videoMpeg4Config,
				   &pConfig->m_videoMpeg4ConfigLength)) {
	delete pVidEncoder;
	return;
      }
    }
    delete pVidEncoder;
  }
  u_int8_t* pMpeg4Config = (u_int8_t*)Malloc(256);
	u_int32_t mpeg4ConfigLength = 0;

	MP4AV_Mpeg4CreateVosh(
		&pMpeg4Config,
		&mpeg4ConfigLength,
		// profile_level_id, default is 3, Simple Profile @ Level 3
		pConfig->m_videoMpeg4ProfileId);

	MP4AV_Mpeg4CreateVo(
		&pMpeg4Config,
		&mpeg4ConfigLength,
		1);

	uint vot = pConfig->GetBoolValue(CFG_VIDEO_USE_B_FRAMES) ?
	  17 : 1;
	MP4AV_Mpeg4CreateVol(
		&pMpeg4Config,
		&mpeg4ConfigLength,
		vot, 
		pConfig->GetFloatValue(CFG_VIDEO_FRAME_RATE),
		pConfig->GetIntegerValue(CFG_VIDEO_TIMEBITS) == 0, 
		// short time - true if we haven't set the # of bits
		true,	// variableRate
		pConfig->m_videoWidth,
		pConfig->m_videoHeight,
		0,	// quantType, H.263
		&pConfig->m_videoTimeIncrBits);

	free(pConfig->m_videoMpeg4Config);
	pConfig->m_videoMpeg4Config = pMpeg4Config;
	pConfig->m_videoMpeg4ConfigLength = mpeg4ConfigLength;
}

void RemoveUserdataFromVol (uint8_t **ppEsConfig, uint32_t *pEsConfigLen)
{
  uint8_t *orig = *ppEsConfig;
  uint32_t orig_len = *pEsConfigLen;
  uint8_t *replace = (uint8_t *)malloc(*pEsConfigLen);
  int32_t next_offset;
  uint32_t written = 0;

  next_offset = MP4AV_Mpeg4FindHeader(orig, orig_len);
  if (next_offset < 0) return;
  orig += next_offset;
  orig_len -= next_offset;

  while ((next_offset = 
	  1 + MP4AV_Mpeg4FindHeader(orig + 1, orig_len - 1)) > 0) {
    if (orig[3] != MP4AV_MPEG4_USER_DATA_START) {
      memcpy(replace + written,
	     orig,
	     next_offset);
      written += next_offset;
    }
    orig_len -= next_offset;
    orig += next_offset;
  }
  if (orig[3] != MP4AV_MPEG4_USER_DATA_START) {
    memcpy(replace + written,
	   orig,
	   orig_len);
    written += orig_len;
  }
  free(*ppEsConfig);
  *ppEsConfig = replace;
  *pEsConfigLen = written;
}
