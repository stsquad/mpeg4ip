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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */
#ifndef __MP4AV_MPEG3_H__
#define __MP4AV_MPEG3_H__ 1

#define MPEG3_START_CODE_PREFIX          0x000001
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_SLICE_MIN_START            0x00000101
#define MPEG3_SLICE_MAX_START            0x000001af
#define MPEG3_USER_DATA_START_CODE       0x000001b2
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_SEQUENCE_ERR_START_CODE    0x000001b4
#define MPEG3_EXT_START_CODE             0x000001b5
#define MPEG3_SEQUENCE_END_START_CODE    0x000001b7
#define MPEG3_GOP_START_CODE             0x000001b8

typedef struct mpeg3_pts_to_dts_t {
  double frame_rate;
  uint16_t last_i_temp_ref;
  uint64_t last_i_pts;
  uint64_t last_i_dts;
  uint64_t last_dts;
} mpeg3_pts_to_dts_t;
  
#ifdef __cplusplus
extern "C" {
#endif

  int MP4AV_Mpeg3ParseSeqHdr(const uint8_t *pbuffer, uint32_t buflen, 
			     int *have_mpeg2,
			      uint32_t *height, uint32_t *width, 
			      double *frame_rate, double *bitrate,
			     double *aspect_ratio,
			     uint8_t *profile_code);

  int MP4AV_Mpeg3PictHdrType(const uint8_t *pbuffer);

  uint16_t MP4AV_Mpeg3PictHdrTempRef(const uint8_t *pbuffer);

  int MP4AV_Mpeg3FindPictHdr(const uint8_t *pbuffer, 
			     uint32_t buflen, 
			     int *ftype);
  int MP4AV_Mpeg3FindNextStart(const uint8_t *pbuffer, 
			       uint32_t buflen,
			       uint32_t *optr, 
			       uint32_t *scode);
  int MP4AV_Mpeg3FindNextSliceStart(const uint8_t *pbuffer,
				     uint32_t startoffset, 
				     uint32_t buflen,
				     uint32_t *slice_offset);
  int mpeg3_find_dts_from_pts(mpeg3_pts_to_dts_t *ptr,
			      uint64_t pts_in_msec,
			      int frame_type,
			      uint16_t temp_ref,
			      uint64_t *return_value);
  uint8_t mpeg2_profile_to_mp4_track_type (uint8_t profile);
  const char *mpeg2_type(uint8_t profile);
#ifdef __cplusplus
}
#endif

#endif
