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

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4creator.h>
#include <mp4v.h>

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
		/* read 4 bytes */
		if (fread(pBuf, 1, 4, inFile) != 4) {
			/* IO error, or not enough bytes */
			return 0;
		}
		*pBufSize = 4;

		/* check for a valid start code */
		if (pBuf[0] != 0 || pBuf[1] != 0 || pBuf[2] != 1) {
			return 0;
		}
		/* set current object start code */
		(*pObjCode) = (u_int)(pBuf[3]);

		/* move to next state */
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

class CBitBuffer{
public:
	CBitBuffer(u_int8_t* pBytes, u_int32_t numBytes) {
		m_pBytes = pBytes;
		m_numBytes = numBytes;
		m_byteIndex = 0;
		m_bitIndex = 0;
	}

	u_int32_t GetBits(u_int8_t numBits);

protected:
	u_int8_t*	m_pBytes;
	u_int32_t	m_numBytes;
	u_int32_t	m_byteIndex;
	u_int8_t	m_bitIndex;
}; 

u_int32_t CBitBuffer::GetBits(u_int8_t numBits)
{
	ASSERT(m_pBytes);
	ASSERT(numBits > 0);
	ASSERT(numBits <= 32);

	u_int32_t bits = 0;

	for (u_int8_t i = numBits; i > 0; i--) {
		if (m_bitIndex == 8) {
			m_byteIndex++;
			if (m_byteIndex >= m_numBytes) {
				throw;
			}
			m_bitIndex = 0;
		}
		bits = (bits << 1) 
			| ((m_pBytes[m_byteIndex] >> (7 - m_bitIndex)) & 1);
		m_bitIndex++;
	}

	// DEBUG printf("GetBits 0x%08x (%u bits)\n", bits, numBits);

	return bits;
}

static void ParseVosh(u_int8_t* pVoshBuf, u_int32_t voshSize,
	u_int8_t* pProfileLevel)
{
	CBitBuffer vosh(pVoshBuf, voshSize);

	try {
		vosh.GetBits(32);				// start code
		*pProfileLevel = vosh.GetBits(8);
	}
	catch (...) {
		fprintf(stderr, "%s: Warning, couldn't parse VOSH\n", ProgName);
	}
}

static void ParseVol(u_int8_t* pVolBuf, u_int32_t volSize,
	u_int8_t* pTimeBits, u_int16_t* pTimeTicks, u_int16_t* pFrameDuration, 
	u_int16_t* pFrameWidth, u_int16_t* pFrameHeight)
{
	CBitBuffer vol(pVolBuf, volSize);

	try {
		vol.GetBits(32);				// start code

		vol.GetBits(1);					// random accessible vol
		vol.GetBits(8);					// object type id
		u_int8_t verid = 0;
		if (vol.GetBits(1)) {			// is object layer id
			verid = vol.GetBits(4);			// object layer verid
			vol.GetBits(3);					// object layer priority
		}
		if (vol.GetBits(4) == 0xF) { 	// aspect ratio info
			vol.GetBits(8);					// par width
			vol.GetBits(8);					// par height
		}
		if (vol.GetBits(1)) {			// vol control parameters
			vol.GetBits(2);					// chroma format
			vol.GetBits(1);					// low delay
			if (vol.GetBits(1)) {			// vbv parameters
				vol.GetBits(15);				// first half bit rate
				vol.GetBits(1);					// marker bit
				vol.GetBits(15);				// latter half bit rate
				vol.GetBits(1);					// marker bit
				vol.GetBits(15);				// first half vbv buffer size
				vol.GetBits(1);					// marker bit
				vol.GetBits(3);					// latter half vbv buffer size
				vol.GetBits(11);				// first half vbv occupancy
				vol.GetBits(1);					// marker bit
				vol.GetBits(15);				// latter half vbv occupancy
				vol.GetBits(1);					// marker bit
			}
		}
		u_int8_t shape = vol.GetBits(2); // object layer shape
		if (shape == 3 /* GRAYSCALE */ && verid != 1) {
			vol.GetBits(4);					// object layer shape extension
		}
		vol.GetBits(1);					// marker bit
		*pTimeTicks = vol.GetBits(16);		// vop time increment resolution 

		u_int8_t i;
		u_int32_t powerOf2 = 1;
		for (i = 0; i < 16; i++) {
			if (*pTimeTicks < powerOf2) {
				break;
			}
			powerOf2 <<= 1;
		}
		*pTimeBits = i;

		vol.GetBits(1);					// marker bit
		if (vol.GetBits(1)) {			// fixed vop rate
			// fixed vop time increment
			*pFrameDuration = vol.GetBits(*pTimeBits); 
		} else {
			*pFrameDuration = 0;
		}
		if (shape == 0 /* RECTANGULAR */) {
			vol.GetBits(1);					// marker bit
			*pFrameWidth = vol.GetBits(13);	// object layer width
			vol.GetBits(1);					// marker bit
			*pFrameHeight = vol.GetBits(13);// object layer height
			vol.GetBits(1);					// marker bit
		} else {
			*pFrameWidth = 0;
			*pFrameHeight = 0;
		}
		// there's more, but we don't need it
	}
	catch (...) {
		fprintf(stderr, "%s: Warning, couldn't parse VOL\n", ProgName);
	}

	return;
}

static void ParseGov(u_int8_t* pGovBuf, u_int32_t govSize,
	u_int8_t* pHours, u_int8_t* pMinutes, u_int8_t* pSeconds)
{
	CBitBuffer gov(pGovBuf, govSize);

	try {
		gov.GetBits(32);	// start code
		*pHours = gov.GetBits(5);
		*pMinutes = gov.GetBits(6);
		gov.GetBits(1);		// marker bit
		*pSeconds = gov.GetBits(6);
	}
	catch (...) {
		fprintf(stderr, "%s: Warning, couldn't parse VOSH\n", ProgName);
	}
}

static void ParseVop(u_int8_t* pVopBuf, u_int32_t vopSize,
	u_char* pVopType, 
	u_int8_t timeBits, u_int16_t timeTicks, u_int32_t* pVopTimeIncrement)
{
	CBitBuffer vop(pVopBuf, vopSize);

	try {
		vop.GetBits(32);	// skip start code

		switch (vop.GetBits(2)) {
		case 0:
			/* Intra */
			*pVopType = 'I';
			break;
		case 1:
			/* Predictive */
			*pVopType = 'P';
			break;
		case 2:
			/* Bidirectional Predictive */
			*pVopType = 'B';
			break;
		case 3:
			/* Sprite */
			*pVopType = 'S';
			break;
		}

		if (!pVopTimeIncrement) {
			return;
		}

		u_int8_t numSecs = 0;
		while (vop.GetBits(1) != 0) {
			numSecs++;
		}
		vop.GetBits(1);		// skip marker
		u_int16_t numTicks = vop.GetBits(timeBits);
		*pVopTimeIncrement = (numSecs * timeTicks) + numTicks; 
	}
	catch (...) {
		fprintf(stderr, "%s: Warning, couldn't parse VOP\n", ProgName);
	}

	return;
}

// Map from ISO IEC 14496-2:2000 Appendix G 
// to ISO IEC 14496-1:2001 8.6.4.2 Table 6
u_int8_t Mp4vVideoToSystemsProfileLevel(u_int8_t videoProfileLevel)
{
	switch (videoProfileLevel) {
	// Simple Profile
	case 0x01: // L1
		return 0x03;
	case 0x02: // L2
		return 0x02;
	case 0x03: // L3
		return 0x01;
	// Simple Scalable Profile
	case 0x11: // L1
		return 0x05;
	case 0x12: // L2
		return 0x04;
	// Core Profile
	case 0x21: // L1
		return 0x07;
	case 0x22: // L2
		return 0x06;
	// Main Profile
	case 0x32: // L2
		return 0x0A;
	case 0x33: // L3
		return 0x09;
	case 0x34: // L4
		return 0x08;
	// N-bit Profile
	case 0x42: // L2
		return 0x0B;
	// Scalable Texture
	case 0x51: // L1
		return 0x12;
	case 0x52: // L2
		return 0x11;
	case 0x53: // L3
		return 0x10;
	// Simple Face Animation Profile
	case 0x61: // L1
		return 0x14;
	case 0x62: // L2
		return 0x13;
	// Simple FBA Profile
	case 0x63: // L1
	case 0x64: // L2
		return 0xFE;
	// Basic Animated Texture Profile
	case 0x71: // L1
		return 0x0F;
	case 0x72: // L2
		return 0x0E;
	// Hybrid Profile
	case 0x81: // L1
		return 0x0D;
	case 0x82: // L2
		return 0x0C;
	// Advanced Real Time Simple Profile
	case 0x91: // L1
	case 0x92: // L2
	case 0x93: // L3
	case 0x94: // L4
	// Core Scalable Profile
	case 0xA1: // L1
	case 0xA2: // L2
	case 0xA3: // L3
	// Advanced Coding Efficiency Profle
	case 0xB1: // L1
	case 0xB2: // L2
	case 0xB3: // L3
	case 0xB4: // L4
	// Advanced Core Profile
	case 0xC1: // L1
	case 0xC2: // L2
	// Advanced Scalable Texture Profile
	case 0xD1: // L1
	case 0xD2: // L2
	case 0xD3: // L3
	// from draft amendments
	// Simple Studio
	case 0xE1: // L1
	case 0xE2: // L2
	case 0xE3: // L3
	case 0xE4: // L4
	// Core Studio Profile
	case 0xE5: // L1
	case 0xE6: // L2
	case 0xE7: // L3
	case 0xE8: // L4
	// Advanced Simple Profile
	case 0xF1: // L0
	case 0xF2: // L1
	case 0xF3: // L2
	case 0xF4: // L3
	case 0xF5: // L4
	// Fine Granularity Scalable Profile
	case 0xF6: // L0
	case 0xF7: // L1
	case 0xF8: // L2
	case 0xF9: // L3
	case 0xFA: // L4
	default:
		return 0xFE;
	}
}

MP4TrackId Mp4vCreator(MP4FileHandle mp4File, FILE* inFile)
{
	bool rc; 

	// we need a two sample window in memory
	// so we use two static buffers 
	// and just keep flipping pointers to avoid memory copies
	u_int8_t sampleBuffer1[256 * 1024];
	u_int8_t sampleBuffer2[256 * 1024];
	u_int8_t* pPreviousSample = sampleBuffer1;
	u_int8_t* pCurrentSample = sampleBuffer2;
	u_int32_t maxSampleSize = sizeof(sampleBuffer1);

	// the current syntactical object
	// typically 1:1 with a sample 
	// but not always, i.e. non-VOP's 
	u_int8_t* pObj = pPreviousSample;
	u_int32_t objSize;
	u_int8_t objType;

	// the previous sample
	u_int32_t previousSampleSize = 0;
	MP4Timestamp previousSampleTime = 0;
	bool previousIsSyncSample = true;

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
		if (pObj + objSize >= pPreviousSample + maxSampleSize) {
			fprintf(stderr,	
				"%s: buffer overflow, invalid video stream?\n", ProgName);
			exit(EXIT_MP4V_CREATOR);
		}

		if (objType == VOSH_START) {
			ParseVosh(pObj, objSize, 
				&videoProfileLevel);

		} else if (objType == VOL_START) {
			ParseVol(pObj, objSize, 
				&timeBits, &timeTicks, &frameDuration, 
				&frameWidth, &frameHeight);

#ifdef MP4V_DEBUG
			printf("ParseVol: timeBits %u timeTicks %u frameDuration %u\n",
				timeBits, timeTicks, frameDuration);
#endif

		} else if (objType == VOP_START) {
			esConfigSize = pObj - pPreviousSample;
			previousSampleSize = esConfigSize + objSize;
			previousIsSyncSample = true;
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
	u_int32_t mp4TimeScale = 90000;
	u_int32_t mp4FrameDuration;
	bool isFixedFrameRate;

	if (VideoFrameRate) {
		mp4FrameDuration = (u_int32_t)(((float)mp4TimeScale) / VideoFrameRate);
		isFixedFrameRate = true;
	} else {
		if (frameDuration) {
			mp4FrameDuration = 
				(mp4TimeScale * frameDuration) / timeTicks;
			isFixedFrameRate = true;
		} else {
			mp4FrameDuration = 0;
			isFixedFrameRate = false;
		}
	}

	// create the new video track
	MP4TrackId trackId = 
		MP4AddVideoTrack(mp4File, 
			mp4TimeScale, mp4FrameDuration, 
			frameWidth, frameHeight, 
			MP4_MPEG4_VIDEO_TYPE);

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create video track\n", ProgName);
		exit(EXIT_MP4V_CREATOR);
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE) == 1) {
		MP4SetVideoProfileLevel(mp4File, 
			Mp4vVideoToSystemsProfileLevel(videoProfileLevel));
	}

	if (esConfigSize) {
		MP4SetTrackESConfiguration(mp4File, trackId, 
			pPreviousSample, esConfigSize);

		// move past ES config, so it doesn't go into first sample
		pPreviousSample += esConfigSize;
		previousSampleSize -= esConfigSize;
	}

	// now process the rest of the video stream
	pObj = pCurrentSample;

	while (LoadNextObject(inFile, pObj, &objSize, &objType)) {
		// guard against buffer overflow
		if (pObj + objSize >= pCurrentSample + maxSampleSize) {
			fprintf(stderr,	
				"%s: buffer overflow, invalid video stream?\n", ProgName);
			exit(EXIT_MP4V_CREATOR);
		}

		if (objType != VOP_START) {
			if (objType == GOV_START) {
				u_int8_t govHours, govMinutes, govSeconds;

				ParseGov(pObj, objSize, &govHours, &govMinutes, &govSeconds);

#ifdef MP4V_DEBUG
				printf("GOV %u:%u:%u\n", govHours, govMinutes, govSeconds);
#endif
				// refVopTime = gov time?
			}

			// keep it in the buffer until a VOP comes along
			pObj += objSize;

		} else { // we have a VOP
			u_int32_t sampleSize = (pObj + objSize) - pCurrentSample;

			u_char vopType = '\0';
			u_int32_t vopTimeIncrement = 0;

			ParseVop(pObj, objSize, &vopType, 
				timeBits, timeTicks, &vopTimeIncrement);

			if (!isFixedFrameRate) {
				currentSampleTime = refVopTime 
					+ ((mp4TimeScale * vopTimeIncrement) / timeTicks);
				mp4FrameDuration = currentSampleTime - previousSampleTime;

#ifdef MP4V_DEBUG
				printf("sample %u %c vopTimeIncrement %u\n"
					sampleId + 1, vopType, vopTimeIncrement);
#endif
			} else {
				currentSampleTime += mp4FrameDuration;
			}

			// now we know enough to write the previous sample
			rc = MP4WriteSample(mp4File, trackId, 
				pPreviousSample, previousSampleSize,
				mp4FrameDuration, 0, previousIsSyncSample);

			if (!rc) {
				fprintf(stderr,	
					"%s: can't write video frame %u\n",
					 ProgName, sampleId);
				exit(EXIT_MP4V_CREATOR);
			}

			sampleId++;

			// deal with rendering time offsets 
			// that can occur when B frames are being used 
			// which is the case for all profiles except Simple Profile
			if (vopType != 'B') {
				if (videoProfileLevel > 3) {
#ifdef MP4V_DEBUG
					printf("sample %u renderingOffset %u\n",
						refVopId, currentSampleTime - refVopTime);
#endif

					MP4SetSampleRenderingOffset(
						mp4File, trackId, refVopId,
						currentSampleTime - refVopTime);
				}

				refVopId = sampleId;
				refVopTime = currentSampleTime;
			}

			// move along
			u_int8_t* temp = pPreviousSample;
			pPreviousSample = pCurrentSample;
			previousSampleSize = sampleSize;
			previousSampleTime = currentSampleTime;
			previousIsSyncSample = (vopType == 'I');
			pCurrentSample = temp;
			pObj = pCurrentSample;
		}
	}

	// write final sample
	rc = MP4WriteSample(mp4File, trackId, 
		pPreviousSample, previousSampleSize,
		0, 0, previousIsSyncSample);

	return trackId;
}

u_char Mp4vGetVopType(u_int8_t* pVopBuf, u_int32_t vopSize)
{
	u_char vopType;
	ParseVop(pVopBuf, vopSize, &vopType, 0, 0, NULL);
	return vopType;
}

