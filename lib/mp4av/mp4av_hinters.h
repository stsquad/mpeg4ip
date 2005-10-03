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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Mark Baugher		mbaugher@cisco.com
 */

#ifndef __MP4AV_HINTERS_INCLUDED__
#define __MP4AV_HINTERS_INCLUDED__ 

#define MP4AV_DFLT_PAYLOAD_SIZE		1460

// Audio Hinters

#ifdef __cplusplus
extern "C" {
#endif

bool MP4AV_Rfc2250Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_Rfc3119Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_RfcIsmaHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_RfcIsmaConcatenator(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration,
	u_int16_t maxPayloadSize);

bool MP4AV_RfcIsmaFragmenter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t sampleSize, 
	MP4Duration sampleDuration,
	u_int16_t maxPayloadSize);

// Video Hinters
MP4TrackId MP4AV_Rfc3016_HintTrackCreate(MP4FileHandle mp4File,
                                         MP4TrackId mediaTrackId);

void MP4AV_Rfc3016_HintAddSample (
				  MP4FileHandle mp4File,
				  MP4TrackId hintTrackId,
				  MP4SampleId sampleId,
				  uint8_t *pSampleBuffer,
				  uint32_t sampleSize,
				  MP4Duration duration,
				  MP4Duration renderingOffset,
				  bool isSyncSample,
				  uint16_t maxPayloadSize);

bool MP4AV_Rfc3016Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_Rfc3016LatmHinter(MP4FileHandle mp4File, 
			     MP4TrackId mediaTrackId, 
			     u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool G711Hinter(MP4FileHandle mp4file, 
		MP4TrackId trackid,
		uint16_t maxPayloadSize);

bool L16Hinter(MP4FileHandle mp4File,
	        MP4TrackId mediaTrackID,
	       uint16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool Mpeg12Hinter(MP4FileHandle mp4File,
		  MP4TrackId mediaTrackID,
		  uint16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_Rfc3267Hinter(MP4FileHandle mp4File,
		  MP4TrackId mediaTrackID,
		  uint16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

// This struct is used to pass ISMACRYP protocol parameters
// to the ISMACRYP hinters, MP4AV_RfcCryptoAudioHinter and
// MP4AV_RfcCryptoVideoHinter.
typedef struct ismacryp_session_params {
u_int8_t   key_count;
u_int8_t   key_ind_len;
u_int8_t   key_ind_perau;
u_int8_t   key_life;
u_int8_t   key_len;
u_int8_t   salt_len;
u_int8_t   selective_enc;
u_int8_t   delta_iv_len;
u_int8_t   iv_len;
u_int32_t  scheme;
u_int8_t   *key;
u_int8_t   *salt;
} mp4av_ismacrypParams;

bool MP4AV_RfcCryptoAudioHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
        mp4av_ismacrypParams *icPp,
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE),
	char* PayloadMIMEType DEFAULT_PARM(""));

bool MP4AV_RfcCryptoVideoHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
        mp4av_ismacrypParams *icPp,
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE),
	char* PayloadMIMEType DEFAULT_PARM(""));

  MP4TrackId MP4AV_H264_HintTrackCreate(MP4FileHandle mp4File,
					MP4TrackId mediaTrackId);
  void MP4AV_H264_HintAddSample(MP4FileHandle mp4File,
				MP4TrackId hintTrackId,
				MP4SampleId sampleId,
				uint8_t *pSampleBuffer,
				uint32_t sampleSize,
				uint32_t sizeLength,
				MP4Duration duration,
				MP4Duration renderingOffset,
				bool isSyncSample,
				uint16_t maxPayloadSize);

  bool MP4AV_H264Hinter(MP4FileHandle mp4File, 
			MP4TrackId mediaTrackId, 
			u_int16_t maxPayloadSize);

  int16_t MP4AV_AmrFrameSize(uint8_t mode, bool isAmrWb);
  bool MP4AV_Rfc2429Hinter(MP4FileHandle file,
			   MP4TrackId mediaTrackId,
			   uint16_t maxPayloadSize);

  bool HrefHinter(MP4FileHandle mp4file, 
		  MP4TrackId trackid,
		  uint16_t maxPayloadSize);
#ifdef __cplusplus
}
#endif

#endif /* __MP4AV_HINTERS_INCLUDED__ */ 

