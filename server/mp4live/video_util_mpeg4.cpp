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

void GenerateMpeg4VideoConfig(CLiveConfig* pConfig)
{
	u_int8_t* pMpeg4Config = (u_int8_t*)Malloc(256);
	u_int32_t mpeg4ConfigLength = 0;

	MP4AV_Mpeg4CreateVosh(
		&pMpeg4Config,
		&mpeg4ConfigLength,
		// profile_level_id, default is 3, Simple Profile @ Level 3
		pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_LEVEL_ID));

	MP4AV_Mpeg4CreateVo(
		&pMpeg4Config,
		&mpeg4ConfigLength,
		1);

	MP4AV_Mpeg4CreateVol(
		&pMpeg4Config,
		&mpeg4ConfigLength,
		pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID),
		pConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE),
		true,	// shortTime
		true,	// variableRate
		pConfig->m_videoWidth,
		pConfig->m_videoHeight,
		0,	// quantType, H.263
		&pConfig->m_videoTimeIncrBits);

	free(pConfig->m_videoMpeg4Config);
	pConfig->m_videoMpeg4Config = pMpeg4Config;
	pConfig->m_videoMpeg4ConfigLength = mpeg4ConfigLength;
}

