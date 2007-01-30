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
 *		Dave Mackie			dmackie@cisco.com
 *		Alix Marchandise-Franquet	alix@cisco.com
 */


/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4creator.h>

#define ADTS_HEADER_MAX_SIZE 10 /* bytes */

static u_int8_t firstHeader[ADTS_HEADER_MAX_SIZE];
static u_int16_t OLD_MP4AV_AdtsGetFrameSize(u_int8_t* pHdr)
{
	/* extract the necessary fields from the header */
	u_int8_t isMpeg4 = !(pHdr[1] & 0x08);
	u_int16_t frameLength;

	if (isMpeg4) {
		frameLength = (((u_int16_t)pHdr[4]) << 5) | (pHdr[5] >> 3); 
	} else { /* MPEG-2 */
		frameLength = (((u_int16_t)(pHdr[3] & 0x3)) << 11) 
			| (((u_int16_t)pHdr[4]) << 3) | (pHdr[5] >> 5); 
	}

	return frameLength;
}
static u_int16_t OLD_MP4AV_AdtsGetHeaderBitSize(u_int8_t* pHdr)
{
	u_int8_t isMpeg4 = !(pHdr[1] & 0x08);
	u_int8_t hasCrc = !(pHdr[1] & 0x01);
	u_int16_t hdrSize;

	if (isMpeg4) {
		hdrSize = 58;
	} else {
		hdrSize = 56;
	}
	if (hasCrc) {
		hdrSize += 16;
	}
	return hdrSize;
}

static u_int16_t OLD_MP4AV_AdtsGetHeaderByteSize(u_int8_t* pHdr)
{
	return (OLD_MP4AV_AdtsGetHeaderBitSize(pHdr) + 7) / 8;
}

/* 
 * hdr must point to at least ADTS_HEADER_MAX_SIZE bytes of memory 
 */
static bool LoadNextAdtsHeader(FILE* inFile, u_int8_t* hdr)
{
	u_int state = 0;
	u_int dropped = 0;
	u_int hdrByteSize = ADTS_HEADER_MAX_SIZE;

	while (1) {
		/* read a byte */
		u_int8_t b;

		if (fread(&b, 1, 1, inFile) == 0) {
			return false;
		}

		/* header is complete, return it */
		if (state == hdrByteSize - 1) {
			hdr[state] = b;
			if (dropped > 0) {
#ifndef _WIN32
			  fprintf(stderr, "Warning: dropped %u input bytes at offset "U64"\n", dropped, 
				  ftello(inFile) - dropped - state - 1);
#else
			  fprintf(stderr, "Warning: dropped %u input bytes at offset %u\n", dropped,
				  ftell(inFile) - dropped - state - 1);
#endif
			}
			return true;
		}

		/* collect requisite number of bytes, no constraints on data */
		if (state >= 2) {
			hdr[state++] = b;
		} else {
			/* have first byte, check if we have 1111X00X */
			if (state == 1) {
				if ((b & 0xF6) == 0xF0) {
					hdr[state] = b;
					state = 2;
					/* compute desired header size */
					if (aacUseOldFile) {
					  hdrByteSize = OLD_MP4AV_AdtsGetHeaderByteSize(hdr);
					} else {
					  hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdr);
					}
				} else {
					state = 0;
					dropped ++;
				}
			}
			/* initial state, looking for 11111111 */
			if (state == 0) {
				if (b == 0xFF) {
					hdr[state] = b;
					state = 1;
				} else {
					 /* else drop it */ 
					dropped++;
					//					printf("%02x ", b);
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
static bool LoadNextAacFrame(FILE* inFile, u_int8_t* pBuf, u_int32_t* pBufSize, bool stripAdts)
{
	u_int16_t frameSize;
	u_int16_t hdrBitSize, hdrByteSize;
	u_int8_t hdrBuf[ADTS_HEADER_MAX_SIZE];

	/* get the next AAC frame header, more or less */
	if (!LoadNextAdtsHeader(inFile, hdrBuf)) {
		return false;
	}
	
	/* get frame size from header */
	if (aacUseOldFile) {
	  frameSize = OLD_MP4AV_AdtsGetFrameSize(hdrBuf);
	  /* get header size in bits and bytes from header */
	  hdrBitSize = OLD_MP4AV_AdtsGetHeaderBitSize(hdrBuf);
	  hdrByteSize = OLD_MP4AV_AdtsGetHeaderByteSize(hdrBuf);
	} else {
	  frameSize = MP4AV_AdtsGetFrameSize(hdrBuf);
	  /* get header size in bits and bytes from header */
	  hdrBitSize = MP4AV_AdtsGetHeaderBitSize(hdrBuf);
	  hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdrBuf);
	}

	
	/* adjust the frame size to what remains to be read */
	frameSize -= hdrByteSize;

	if (stripAdts) {
		if ((hdrBitSize % 8) == 0) {
			/* header is byte aligned, i.e. MPEG-2 ADTS */
			/* read the frame data into the buffer */
			if (fread(pBuf, 1, frameSize, inFile) != frameSize) {
				return false;
			}
			(*pBufSize) = frameSize;
		} else {
			/* header is not byte aligned, i.e. MPEG-4 ADTS */
			int i;
			u_int8_t newByte;
			int upShift = hdrBitSize % 8;
			int downShift = 8 - upShift;

			pBuf[0] = hdrBuf[hdrBitSize / 8] << upShift;

			for (i = 0; i < frameSize; i++) {
				if (fread(&newByte, 1, 1, inFile) != 1) {
					return false;
				}
				pBuf[i] |= (newByte >> downShift);
				pBuf[i+1] = (newByte << upShift);
			}
			(*pBufSize) = frameSize + 1;
		}
	} else { /* don't strip ADTS headers */
		memcpy(pBuf, hdrBuf, hdrByteSize);
		if (fread(&pBuf[hdrByteSize], 1, frameSize, inFile) != frameSize) {
			return false;
		}
	}

	return true;
}

static bool GetFirstHeader(FILE* inFile)
{
	/* read file until we find an audio frame */
	fpos_t curPos;

	/* already read first header */
	if (firstHeader[0] == 0xff) {
		return true;
	}

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* go back to start of file */
	rewind(inFile);

	if (!LoadNextAdtsHeader(inFile, firstHeader)) {
		return false;
	}

	/* reposition the file to where we were */
	fsetpos(inFile, &curPos);

	return true;
}

MP4TrackId AacCreator(MP4FileHandle mp4File, FILE* inFile, bool doEncrypt)
{
  // collect all the necessary meta information
  u_int32_t samplesPerSecond;
  u_int8_t mpegVersion;
  u_int8_t profile;
  u_int8_t channelConfig;
  ismacryp_session_id_t ismaCrypSId;
  mp4v2_ismacrypParams *icPp =  (mp4v2_ismacrypParams *) malloc(sizeof(mp4v2_ismacrypParams));
  memset(icPp, 0, sizeof(mp4v2_ismacrypParams));


  // initialize session if encrypting
  if (doEncrypt) {
    if (ismacrypInitSession(&ismaCrypSId,KeyTypeAudio) != 0) {
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

  if (!GetFirstHeader(inFile)) {
    fprintf(stderr,	
	    "%s: data in file doesn't appear to be valid audio\n",
	    ProgName);
    return MP4_INVALID_TRACK_ID;
  }

  samplesPerSecond = MP4AV_AdtsGetSamplingRate(firstHeader);
  mpegVersion = MP4AV_AdtsGetVersion(firstHeader);
  profile = MP4AV_AdtsGetProfile(firstHeader);
  if (aacProfileLevel == 2) {
    if (profile > MP4_MPEG4_AAC_SSR_AUDIO_TYPE) {
      fprintf(stderr, "Can't convert profile to mpeg2\nDo not contact project creators for help\n");
      return MP4_INVALID_TRACK_ID;
    }
    mpegVersion = 1;
  } else if (aacProfileLevel == 4) {
    mpegVersion = 0;
  }
  channelConfig = MP4AV_AdtsGetChannels(firstHeader);

  u_int8_t audioType = MP4_INVALID_AUDIO_TYPE;
  switch (mpegVersion) {
  case 0:
    audioType = MP4_MPEG4_AUDIO_TYPE;
    break;
  case 1:
    switch (profile) {
    case 0:
      audioType = MP4_MPEG2_AAC_MAIN_AUDIO_TYPE;
      break;
    case 1:
      audioType = MP4_MPEG2_AAC_LC_AUDIO_TYPE;
      break;
    case 2:
      audioType = MP4_MPEG2_AAC_SSR_AUDIO_TYPE;
      break;
    case 3:
      fprintf(stderr,	
	      "%s: data in file doesn't appear to be valid audio\n",
	      ProgName);
      return MP4_INVALID_TRACK_ID;
    default:
      ASSERT(false);
    }
    break;
  default:
    ASSERT(false);
  }

  // add the new audio track
  MP4TrackId trackId;
  if (doEncrypt) {
    trackId = MP4AddEncAudioTrack(mp4File, samplesPerSecond, 1024, 
                                     icPp,
                                     audioType);
  } else {	
    trackId = MP4AddAudioTrack(mp4File,
                    	       samplesPerSecond, 1024, audioType);
  }

  if (trackId == MP4_INVALID_TRACK_ID) {
    fprintf(stderr,	
	    "%s: can't create audio track\n", ProgName);
    return MP4_INVALID_TRACK_ID;
  }

  if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
    MP4SetAudioProfileLevel(mp4File, 0x0F);
  }
	
  u_int8_t* pConfig = NULL;
  u_int32_t configLength = 0;

  MP4AV_AacGetConfiguration(
			    &pConfig,
			    &configLength,
			    profile,
			    samplesPerSecond,
			    channelConfig);

  if (!MP4SetTrackESConfiguration(mp4File, trackId, 
				  pConfig, configLength)) {
    fprintf(stderr,	
	    "%s: can't write audio configuration\n", ProgName);
    MP4DeleteTrack(mp4File, trackId);
    return MP4_INVALID_TRACK_ID;
  }

  // parse the ADTS frames, and write the MP4 samples
  u_int8_t sampleBuffer[8 * 1024];
  u_int32_t sampleSize = sizeof(sampleBuffer);
  MP4SampleId sampleId = 1;

  while (LoadNextAacFrame(inFile, sampleBuffer, &sampleSize, true)) {
    if (doEncrypt) {
      u_int8_t* encSampleData = NULL;
      u_int32_t encSampleLen = 0;
      if (ismacrypEncryptSampleAddHeader(ismaCrypSId, sampleSize, sampleBuffer,
					 &encSampleLen, &encSampleData) != 0) {
	fprintf(stderr,	
		"%s: can't encrypt audio frame  and add header %u\n", ProgName, sampleId);
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

