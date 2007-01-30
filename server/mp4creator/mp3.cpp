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
 *		Dave Mackie		   dmackie@cisco.com
 *              Alix Marchandise-Franquet  alix@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4creator.h>

static bool LoadNextMp3Header(FILE* inFile, u_int32_t* pHdr, bool allowLayer4)
{
	u_int8_t state = 0;
	u_int32_t dropped = 0;
	u_int8_t bytes[4];
	memset(bytes, 0, sizeof(bytes));
	while (true) {
		/* read a byte */
		u_int8_t b;

		if (fread(&b, 1, 1, inFile) == 0) {
			return false;
		}

		if (state == 3) {
			bytes[state] = b;
			(*pHdr) = (bytes[0] << 24) | (bytes[1] << 16) 
				| (bytes[2] << 8) | bytes[3];
			if (dropped > 0) {
				fprintf(stderr, "Warning dropped %u input bytes\n", dropped);
			}
			return true;
		}
		if (state == 2) {
			if ((b & 0xF0) == 0 || (b & 0xF0) == 0xF0 || (b & 0x0C) == 0x0C) {
				if (bytes[1] == 0xFF) {
					state = 1; 
				} else {
					state = 0; 
				}
			} else {
				bytes[state] = b;
				state = 3;
			}
		}
		if (state == 1) {
			if ((b & 0xE0) == 0xE0 && (b & 0x18) != 0x08 && 
			  ((b & 0x06) != 0 || allowLayer4)) {
				bytes[state] = b;
				state = 2;
			} else {
				state = 0;
			}
		}
		if (state == 0) {
			if (b == 0xFF) {
				bytes[state] = b;
				state = 1;
			} else {
				if (dropped == 0 && 
				  ((b & 0xE0) == 0xE0 && 
				  (b & 0x18) != 0x08 && 
			  	  ((b & 0x06) != 0 || allowLayer4))) {
					/*
					 * HACK have seen files where previous frame 
					 * was marked as padded, but the byte was never added
					 * which results in the next frame's leading 0XFF being
					 * eaten. We attempt to repair that situation here.
					 */
					bytes[0] = 0xFF;
					bytes[1] = b;
					state = 2;
				} else {
					/* else drop it */ 
					dropped++;
				}
			}
		}
	}
}

/*
 * Load the next frame from the file
 * into the supplied buffer, which better be large enough!
 *
 * Note: Frames are padded to byte boundaries
 */
static bool LoadNextMp3Frame(FILE* inFile, u_int8_t* pBuf, u_int32_t* pBufSize)
{
	u_int32_t header;
	u_int16_t frameSize;

	/* get the next MP3 frame header */
	if (!LoadNextMp3Header(inFile, &header, false)) {
		return false;
	}
	
	/* get frame size from header */
	frameSize = MP4AV_Mp3GetFrameSize(header);

	// guard against buffer overflow
	if (frameSize > (*pBufSize)) {
		return false;
	}

	/* put the header in the buffer */
	pBuf[0] = (header >> 24) & 0xFF;
	pBuf[1] = (header >> 16) & 0xFF;
	pBuf[2] = (header >> 8) & 0xFF;
	pBuf[3] = header & 0xFF;

	/* read the frame data into the buffer */
	if (fread(&pBuf[4], 1, frameSize - 4, inFile) != (size_t)(frameSize - 4)) {
		return false;
	}
	(*pBufSize) = frameSize;

	return true;
}

static bool GetMp3SamplingParams(FILE* inFile, 
	u_int16_t* pSamplingRate, u_int16_t* pSamplingWindow, u_int8_t* pVersion)
{
	/* read file until we find an audio frame */
	fpos_t curPos;
	u_int32_t header;

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* get the next MP3 frame header */
	if (!LoadNextMp3Header(inFile, &header, false)) {
		return false;
	}

	(*pSamplingRate) = MP4AV_Mp3GetHdrSamplingRate(header);
	(*pSamplingWindow) = MP4AV_Mp3GetHdrSamplingWindow(header);
	(*pVersion) = MP4AV_Mp3GetHdrVersion(header);

	/* rewind the file to where we were */
	fsetpos(inFile, &curPos);

	return true;
}

/*
 * Get MPEG layer from MP3 header
 * if it's really MP3, it should be layer 3, value 01
 * if it's really ADTS, it should be layer 4, value 00
 */
static bool GetMpegLayer(FILE* inFile, u_int8_t* pLayer)
{
	/* read file until we find an audio frame */
	fpos_t curPos;
	u_int32_t header;

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* get the next MP3 frame header */
	if (!LoadNextMp3Header(inFile, &header, true)) {
		return false;
	}

	(*pLayer) = MP4AV_Mp3GetHdrLayer(header);

	/* rewind the file to where we were */
	fsetpos(inFile, &curPos);

	return true;
}

MP4TrackId Mp3Creator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt)
{
	u_int8_t mpegLayer;

	if (!GetMpegLayer(inFile, &mpegLayer)) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be MPEG audio\n", ProgName);
		return MP4_INVALID_TRACK_ID;
	}
	if (mpegLayer == 0) {
		fprintf(stderr,	
			"%s: data in file appears to be AAC audio, use .aac extension\n",
			 ProgName);
		return MP4_INVALID_TRACK_ID;
	}

	u_int16_t samplesPerSecond;
	u_int16_t samplesPerFrame;
	u_int8_t mpegVersion;

	if (!GetMp3SamplingParams(inFile, 
	  &samplesPerSecond, &samplesPerFrame, &mpegVersion)) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be valid audio\n",
			 ProgName);
		return MP4_INVALID_TRACK_ID;
	}

	u_int8_t audioType = MP4AV_Mp3ToMp4AudioType(mpegVersion);

	if (audioType == MP4_INVALID_AUDIO_TYPE
	  || samplesPerSecond == 0) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be valid audio\n",
			 ProgName);
		return MP4_INVALID_TRACK_ID;
	}

	
	ismacryp_session_id_t ismaCrypSId;
        mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
        memset(icPp, 0, sizeof(mp4v2_ismacrypParams));



	// initialize session if encrypting
	if (doEncrypt) {
	  if (ismacrypInitSession(&ismaCrypSId, KeyTypeAudio) != 0) {
	    fprintf(stderr, "%s: could not initialize the ISMAcryp session\n",
		    ProgName);
	    return MP4_INVALID_TRACK_ID;
	  }

          if (ismacrypGetScheme(ismaCrypSId, &(icPp->scheme_type)) != ismacryp_rc_ok) {
             fprintf(stderr, "%s: could not get ismacryp scheme type. sid %d\n", 
                     ProgName, ismaCrypSId);
             ismacrypEndSession(ismaCrypSId);
             return MP4_INVALID_TRACK_ID;
          }
          if (ismacrypGetSchemeVersion(ismaCrypSId, &(icPp->scheme_version)) != ismacryp_rc_ok) {
             fprintf(stderr, "%s: could not get ismacryp scheme ver. sid %d\n",
                     ProgName, ismaCrypSId);
             ismacrypEndSession(ismaCrypSId);
             return MP4_INVALID_TRACK_ID;
          }
          if (ismacrypGetKMSUri(ismaCrypSId, &(icPp->kms_uri)) != ismacryp_rc_ok) {
             fprintf(stderr, "%s: could not get ismacryp kms uri. sid %d\n",
                     ProgName, ismaCrypSId);
             CHECK_AND_FREE(icPp->kms_uri);
             ismacrypEndSession(ismaCrypSId);
             return MP4_INVALID_TRACK_ID;
          }
          if ( ismacrypGetSelectiveEncryption(ismaCrypSId, &(icPp->selective_enc)) != ismacryp_rc_ok ) {
             fprintf(stderr, "%s: could not get ismacryp selec enc. sid %d\n",
                     ProgName, ismaCrypSId);
             ismacrypEndSession(ismaCrypSId);
             return MP4_INVALID_TRACK_ID;
          }
          if (ismacrypGetKeyIndicatorLength(ismaCrypSId, &(icPp->key_ind_len)) != ismacryp_rc_ok) {
             fprintf(stderr, "%s: could not get ismacryp key ind len. sid %d\n",
                     ProgName, ismaCrypSId);
             ismacrypEndSession(ismaCrypSId);
             return MP4_INVALID_TRACK_ID;
          }
          if (ismacrypGetIVLength(ismaCrypSId, &(icPp->iv_len)) != ismacryp_rc_ok) {
             fprintf(stderr, "%s: could not get ismacryp iv len. sid %d\n",
                     ProgName, ismaCrypSId);
             ismacrypEndSession(ismaCrypSId);
             return MP4_INVALID_TRACK_ID;
          }
	}

	MP4TrackId trackId;
	MP4Duration duration;
	if (TimeScaleSpecified && Mp4TimeScale == 90000) {
	  duration = (90000 * samplesPerFrame) / samplesPerSecond;
	  if (doEncrypt) {
	    trackId = 
	      MP4AddEncAudioTrack(mp4File, 
				  90000,
				  duration,
                                  icPp,
				  audioType);
	  } else {
	    trackId = 
	      MP4AddAudioTrack(mp4File, 
			       90000,
			       duration, 
			       audioType);
	  }
	} else {
	  if (doEncrypt) {
	    trackId = 
	      MP4AddEncAudioTrack(mp4File, 
			       samplesPerSecond, samplesPerFrame, 
                               icPp,
                               audioType);
	  } else {
	    trackId = 
	      MP4AddAudioTrack(mp4File, 
			       samplesPerSecond, samplesPerFrame, audioType);
	  }
	}

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create audio track\n", ProgName);
		return MP4_INVALID_TRACK_ID;
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
		MP4SetAudioProfileLevel(mp4File, 0xFE);
	}

	u_int8_t sampleBuffer[8 * 1024];
	u_int32_t sampleSize = sizeof(sampleBuffer);
	MP4SampleId sampleId = 1;

	while (LoadNextMp3Frame(inFile, sampleBuffer, &sampleSize)) {
	  if (doEncrypt) {
	    u_int8_t* encSampleData = NULL;
	    u_int32_t encSampleLen = 0;
	    if (ismacrypEncryptSampleAddHeader(ismaCrypSId, sampleSize, sampleBuffer, 
					       &encSampleLen, &encSampleData) != 0) {
	      fprintf(stderr,	
		      "%s: can't encrypt audio frame and add header%u\n", 
		      ProgName, sampleId);
	    }
	    if (!MP4WriteSample(mp4File, trackId, encSampleData, encSampleLen)) {
	      fprintf(stderr,	
		      "%s: can't write audio frame %u\n", ProgName, sampleId);
	      MP4DeleteTrack(mp4File, trackId);
	      return MP4_INVALID_TRACK_ID;
	    }
	    if (encSampleData != NULL) {
	      free(encSampleData);
	    }
	  }
	  else {
	    if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize)) {
	      fprintf(stderr,	
		      "%s: can't write audio frame %u\n", ProgName, sampleId);
	      MP4DeleteTrack(mp4File, trackId);
	      return MP4_INVALID_TRACK_ID;
	    }
	  }

	  sampleId++;
	  sampleSize = sizeof(sampleBuffer);
	}

	 // terminate session if encrypting
	if (doEncrypt) {
	  if (ismacrypEndSession(ismaCrypSId) != 0) {
	    fprintf(stderr, 
		    "%s: could not end the ISMAcryp session\n",
		    ProgName);
	  }
	}

	return trackId;
}
