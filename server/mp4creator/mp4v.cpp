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

// DEBUG #define MP4V_DEBUG 1

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4creator.h>

/*
 * Load the next syntatic object from the file
 * into the supplied buffer, which better be large enough!
 *
 * Objects are delineated by 4 byte start codes (0x00 0x00 0x01 0x??)
 * Objects are padded to byte boundaries, and the encoder assures that
 * the start code pattern is never generated for encoded data
 */
static int LoadNextObject(FILE* inFile, 
	u_int8_t* pBuf, u_int32_t* pBufSize, u_int8_t* pObjCode)
{
	static u_int8_t state = 0;
	static u_int8_t nextObjCode = 0;

	*pBufSize = 0;

	/* initial state, never read from file before */
	if (state == 0) {
		/*
		 * go looking for first sync word 
		 * we discard anything before this
		 */
		state = 1;

		while (state < 5) {
			/* read a byte */
			u_char b;

			if (fread(&b, 1, 1, inFile) == 0) {
				// EOF or IO error
				return 0;
			}

			switch (state) {
			case 1:
				if (b == 0) {
					state = 2;
				}
				break;
			case 2:
				if (b == 0) {
					state = 3;
				}
				break;
			case 3:
				if (b == 1) {
					state = 4;
				}
				break;
			case 4:
				pBuf[0] = 0;
				pBuf[1] = 0;
				pBuf[2] = 1;
				pBuf[3] = b;
				(*pObjCode) = (u_int)b;
				*pBufSize = 4;
				state = 5;
				break;
			}
		}

		/* we're primed now */
		state = 1;

	} else if (state == 5) {
		/* EOF was reached on last call */
		return 0;

	} else {
		/*
		 * We have next object code from prevous call 
		 * insert start code into buffer
		 */
		(*pObjCode) = nextObjCode;
		pBuf[(*pBufSize)++] = 0;
		pBuf[(*pBufSize)++] = 0;
		pBuf[(*pBufSize)++] = 1;
		pBuf[(*pBufSize)++] = (u_char)(*pObjCode);
	}

	/* read bytes, execute state machine */
	while (1) {
		/* read a byte */
		u_char b;

		if (fread(&b, 1, 1, inFile) == 0) {
			/* handle EOF */
			if (feof(inFile)) {
				switch (state) {
				case 3:
					pBuf[(*pBufSize)++] = 0;
					/* fall thru */
				case 2:
					pBuf[(*pBufSize)++] = 0;
					break;
				}
				state = 5;
				return 1;
			}
			/* else error */
			*pBufSize = 0;
			return 0;
		}

		switch (state) {
		case 1:
			if (b == 0) {
				state = 2;
			} else {
				pBuf[(*pBufSize)++] = b;
			}
			break;
		case 2:
			if (b == 0) {
				state = 3;
			} else {
				pBuf[(*pBufSize)++] = 0;
				pBuf[(*pBufSize)++] = b;
				state = 1;
			}
			break;
		case 3:
			if (b == 1) {
				state = 4;
			} else if (b == 0) {
				pBuf[(*pBufSize)++] = 0;
				// state remains 3
			} else {
				pBuf[(*pBufSize)++] = 0;
				pBuf[(*pBufSize)++] = 0;
				pBuf[(*pBufSize)++] = b;
				state = 1;
			}
			break;
		case 4:
			nextObjCode = b;
			state = 1;
			return 1;
		default:
			ASSERT(false);
		}
	}
}

MP4TrackId Mp4vCreator(MP4FileHandle mp4File, FILE* inFile)
{
	bool rc; 

	u_int8_t sampleBuffer[256 * 1024];
	u_int8_t* pCurrentSample = sampleBuffer;
	u_int32_t maxSampleSize = sizeof(sampleBuffer);

	// the current syntactical object
	// typically 1:1 with a sample 
	// but not always, i.e. non-VOP's 
	u_int8_t* pObj = pCurrentSample;
	u_int32_t objSize;
	u_int8_t objType;

	// the current sample
	MP4SampleId sampleId = 1;
	MP4Timestamp currentSampleTime = 0;

	// the last reference VOP 
	MP4SampleId refVopId = 1;
	MP4Timestamp refVopTime = 0;

	// track configuration info
	u_int8_t videoProfileLevel = 0x03;
	u_int8_t timeBits = 15;
	u_int16_t timeTicks = 30000;
	u_int16_t frameDuration = 3000;
	u_int16_t frameWidth = 320;
	u_int16_t frameHeight = 240;
	u_int32_t esConfigSize = 0;

	// start reading objects until we get the first VOP
	while (LoadNextObject(inFile, pObj, &objSize, &objType)) {
		// guard against buffer overflow
		if (pObj + objSize >= pCurrentSample + maxSampleSize) {
			fprintf(stderr,	
				"%s: buffer overflow, invalid video stream?\n", ProgName);
			exit(EXIT_MP4V_CREATOR);
		}

		if (Verbosity & MP4_DETAILS_SAMPLE) {
			printf("MP4V type %x size %u\n",
				objType, objSize);
		}

		if (objType == MP4AV_MPEG4_VOSH_START) {
			MP4AV_Mpeg4ParseVosh(pObj, objSize, 
				&videoProfileLevel);

		} else if (objType == MP4AV_MPEG4_VOL_START) {
			MP4AV_Mpeg4ParseVol(pObj, objSize, 
				&timeBits, &timeTicks, &frameDuration, 
				&frameWidth, &frameHeight);

#ifdef MP4V_DEBUG
			printf("ParseVol: timeBits %u timeTicks %u frameDuration %u\n",
				timeBits, timeTicks, frameDuration);
#endif

		} else if (objType == MP4AV_MPEG4_VOP_START) {
			esConfigSize = pObj - pCurrentSample;
			// ready to set up mp4 track
			break;
		} 
		pObj += objSize;
	}

	// convert frame duration to canonical time scale
	// note zero value for frame duration signals variable rate video
	if (timeTicks == 0) {
		timeTicks = 1;
	}
	u_int32_t mp4FrameDuration;

	if (VideoFrameRate) {
		mp4FrameDuration = (u_int32_t)(((float)Mp4TimeScale) / VideoFrameRate);
	} else if (frameDuration) {
		mp4FrameDuration = (Mp4TimeScale * frameDuration) / timeTicks;
	} else {
		// the spec for embedded timing and the implementations that I've
		// seen all vary dramatically, so until things become clearer
		// we don't support these types of encoders
		fprintf(stderr,	
			"%s: variable rate video stream signalled,"
			" please specify average frame rate with -r option\n",
			 ProgName);
		exit(EXIT_MP4V_CREATOR);
	}

	// create the new video track
	MP4TrackId trackId = 
		MP4AddVideoTrack(
			mp4File, 
			Mp4TimeScale,
			mp4FrameDuration, 
			frameWidth, 
			frameHeight, 
			MP4_MPEG4_VIDEO_TYPE);

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create video track\n", ProgName);
		exit(EXIT_MP4V_CREATOR);
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE) == 1) {
		MP4SetVideoProfileLevel(mp4File, 
			MP4AV_Mpeg4VideoToSystemsProfileLevel(videoProfileLevel));
	}

	if (esConfigSize) {
		MP4SetTrackESConfiguration(mp4File, trackId, 
			pCurrentSample, esConfigSize);

		// move past ES config, so it doesn't go into first sample
		pCurrentSample += esConfigSize;
	}

	// now process the rest of the video stream
	while (true) {
		if (objType != MP4AV_MPEG4_VOP_START) {
			// keep it in the buffer until a VOP comes along
			pObj += objSize;

		} else { // we have a VOP
			u_int32_t sampleSize = (pObj + objSize) - pCurrentSample;

			u_char vopType = MP4AV_Mpeg4GetVopType(pObj, objSize);

			rc = MP4WriteSample(mp4File, trackId, 
				pCurrentSample, sampleSize,
				mp4FrameDuration, 0, vopType == 'I');

			if (!rc) {
				fprintf(stderr,	
					"%s: can't write video frame %u\n",
					 ProgName, sampleId);
				exit(EXIT_MP4V_CREATOR);
			}

			// deal with rendering time offsets 
			// that can occur when B frames are being used 
			// which is the case for all profiles except Simple Profile
			if (vopType != 'B') {
				if (videoProfileLevel > 3) {
#ifdef MP4V_DEBUG
					printf("sample %u %c renderingOffset %d\n",
						refVopId, vopType, currentSampleTime - refVopTime);
#endif

					MP4SetSampleRenderingOffset(
						mp4File, trackId, refVopId,
						currentSampleTime - refVopTime);
				}

				refVopId = sampleId;
				refVopTime = currentSampleTime;
			}

			sampleId++;
			currentSampleTime += mp4FrameDuration;

			// reset pointers
			pObj = pCurrentSample = sampleBuffer;
		}

		// load next object from bitstream
		if (!LoadNextObject(inFile, pObj, &objSize, &objType)) {
			break;
		}
		// guard against buffer overflow
		if (pObj + objSize >= pCurrentSample + maxSampleSize) {
			fprintf(stderr,	
				"%s: buffer overflow, invalid video stream?\n", ProgName);
			exit(EXIT_MP4V_CREATOR);
		}

		if (Verbosity & MP4_DETAILS_SAMPLE) {
			printf("MP4V type %x size %u\n",
				objType, objSize);
		}
	}

	return trackId;
}

