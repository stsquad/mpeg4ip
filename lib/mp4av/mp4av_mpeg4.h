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

#ifndef __MP4AV_MPEG4_INCLUDED__
#define __MP4AV_MPEG4_INCLUDED__

#define MP4AV_MPEG4_SYNC		0x000001
#define MP4AV_MPEG4_VOL_START	0x20
#define MP4AV_MPEG4_VOSH_START	0xB0
#define MP4AV_MPEG4_VOSH_END    0xB1
#define MP4AV_MPEG4_USER_DATA_START 0xB2
#define MP4AV_MPEG4_GOV_START	0xB3
#define MP4AV_MPEG4_VO_START	0xB5
#define MP4AV_MPEG4_VOP_START	0xB6


#ifdef __cplusplus
extern "C" {
#endif

int32_t MP4AV_Mpeg4FindHeader (const uint8_t *pStart, 
			       uint32_t buflen,
			       bool do_header_type DEFAULT_PARM(false), 
			       uint8_t header_type DEFAULT_PARM(0));
  uint8_t *MP4AV_Mpeg4FindVosh(uint8_t *pBuf, uint32_t bufLen);
bool MP4AV_Mpeg4ParseVosh(
	u_int8_t* pVoshBuf, 
	u_int32_t voshSize,
	u_int8_t* pProfileLevel);

bool MP4AV_Mpeg4CreateVosh(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t profileLevel);

bool MP4AV_Mpeg4CreateVo(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t objectId);

  uint8_t *MP4AV_Mpeg4FindVol(uint8_t *pBuf, uint32_t buflen);

bool MP4AV_Mpeg4ParseVol(
	u_int8_t* pVolBuf, 
	u_int32_t volSize,
	u_int8_t* pTimeBits, 
	u_int16_t* pTimeTicks, 
	u_int16_t* pFrameDuration, 
	u_int16_t* pFrameWidth, 
	u_int16_t* pFrameHeight,
	u_int8_t * aspectRatioDefine DEFAULT_PARM(NULL),
	u_int8_t * aspectRatioWidth DEFAULT_PARM(NULL),
	u_int8_t * aspectRatioHeight DEFAULT_PARM(NULL));
bool MP4AV_Mpeg4CreateVol(
	u_int8_t** ppBytes,
	u_int32_t* pNumBytes,
	u_int8_t profile,
	double frameRate,
	bool shortTime,
	bool variableRate,
	u_int16_t width,
	u_int16_t height,
	u_int8_t quantType,
	u_int8_t* pTimeBits DEFAULT_PARM(NULL));

bool MP4AV_Mpeg4ParseGov(
	u_int8_t* pGovBuf, 
	u_int32_t govSize,
	u_int8_t* pHours, 
	u_int8_t* pMinutes, 
	u_int8_t* pSeconds);

  uint8_t *MP4AV_Mpeg4FindVop(uint8_t *pBuf, uint32_t buflen);
bool MP4AV_Mpeg4ParseVop(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize,
	int* pVopType, 
	u_int8_t timeBits, 
	u_int16_t timeTicks, 
	u_int32_t* pVopTimeIncrement);

int MP4AV_Mpeg4GetVopType(
	u_int8_t* pVopBuf, 
	u_int32_t vopSize);

  const char *MP4AV_Mpeg4VisualProfileName (uint8_t profile);
#define VOP_TYPE_I 1
#define VOP_TYPE_P 2
#define VOP_TYPE_B 3
#define VOP_TYPE_S 4

typedef struct mp4av_pts_to_dts_t {
  int last_type;
  uint64_t last_dts;
  int have_last_pts;
  uint64_t last_pts;
} mp4av_pts_to_dts_t;
int MP4AV_calculate_dts_from_pts(mp4av_pts_to_dts_t *ptr,
				 uint64_t pts,
				 int type,
				 uint64_t *dts);
  void MP4AV_clear_dts_from_pts (mp4av_pts_to_dts_t *ptr);
#ifdef __cplusplus
}
#endif
#endif /* __MP4AV_MPEG4_INCLUDED__ */
