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
#include <mp3.h>

static bool LoadNextMp3Header(FILE* inFile, u_int32_t* pHdr, bool allowLayer4)
{
	u_int8_t state = 0;
	u_int32_t dropped = 0;
	u_int8_t bytes[4];

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

static u_int16_t Mp3BitRates[5][14] = {
	/* MPEG-1, Layer III */
	{ 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
	/* MPEG-1, Layer II */
	{ 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
	/* MPEG-1, Layer I */
	{ 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
	/* MPEG-2 or 2.5, Layer II or III */
	{ 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
	/* MPEG-2 or 2.5, Layer I */
	{ 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 }
};

static u_int16_t Mp3SampleRates[4][3] = {
	{ 11025, 12000, 8000 },		/* MPEG-2.5 */
	{ 0, 0, 0 },
	{ 22050, 24000, 16000 },	/* MPEG-2 */
	{ 44100, 48000, 32000 }		/* MPEG-1 */
};

u_int8_t Mp3GetHdrVersion(u_int32_t hdr)
{
	/* extract the necessary field from the MP3 header */
	return ((hdr >> 19) & 0x3); 
}

u_int16_t Mp3GetHdrSamplingRate(u_int32_t hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = Mp3GetHdrVersion(hdr);
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;

	return Mp3SampleRates[version][sampleRateIndex];
}

u_int16_t Mp3GetHdrSamplingWindow(u_int32_t hdr)
{
	u_int8_t version = Mp3GetHdrVersion(hdr);
	u_int8_t layer = (hdr >> 17) & 0x3; 
	u_int16_t samplingWindow;

	if (layer == 1) {
		if (version == 3) {
			samplingWindow = 1152;
		} else {
			samplingWindow = 576;
		}
	} else if (layer == 2) {
		samplingWindow = 1152;
	} else {
		samplingWindow = 384;
	}

	return samplingWindow;
}

/*
 * compute MP3 frame size
 */
u_int16_t Mp3GetFrameSize(u_int32_t hdr)
{
	/* extract the necessary fields from the MP3 header */
	u_int8_t version = Mp3GetHdrVersion(hdr);
	u_int8_t layer = (hdr >> 17) & 0x3; 
	u_int8_t bitRateIndex1;
	u_int8_t bitRateIndex2 = (hdr >> 12) & 0xF;
	u_int8_t sampleRateIndex = (hdr >> 10) & 0x3;
	bool isProtected = !((hdr >> 16) & 0x1);
	bool isPadded = (hdr >> 9) & 0x1;
	u_int16_t frameSize = 0;

	if (version == 3) {
		/* MPEG-1 */
		bitRateIndex1 = layer - 1;
	} else {
		/* MPEG-2 or MPEG-2.5 */
		if (layer == 3) {
			/* layer 1 */
			bitRateIndex1 = 4;
		} else {
			bitRateIndex1 = 3;
		}
	}

	/* compute frame size */
	frameSize = (144 * 1000 * Mp3BitRates[bitRateIndex1][bitRateIndex2-1]) 
		/ (Mp3SampleRates[version][sampleRateIndex] << !(version & 1));

	if (isProtected) {
		frameSize += 2;		/* 2 byte checksum is present */
	}
	if (isPadded) {
		frameSize++;		/* 1 byte pad is present */
	}

	return frameSize;
}

u_int16_t Mp3GetAduOffset(u_int8_t* pFrame, u_int32_t frameSize)
{
	if (frameSize < 6) {
		return 0;
	}

	bool isProtected = !(pFrame[1] & 0x1);
	u_int8_t layer = (pFrame[1] >> 1) & 0x3; 

	if (layer != 1 || (isProtected && frameSize < 8)) {
		return 0;
	}

	u_int8_t crcSize = isProtected ? 2 : 0;

	return (pFrame[4 + crcSize] << 1) | (pFrame[5 + crcSize] >> 7);
}

u_int16_t Mp3GetSideInfoSize(u_int32_t hdr)
{
	u_int8_t layer = (hdr >> 17) & 0x3; 

	// check that this is layer 3
	if (layer != 1) {
		return 0;
	}

	u_int8_t mode = (hdr >> 6) & 0x3;
	u_int8_t channels;

	if (mode == 3) {
		channels = 1;
	} else {
		channels = 2;
	}

	u_int16_t bits = 0;

	// main data begin
	bits += 9;

	// private bits
	if (channels == 1) {
		bits += 5;
	} else {
		bits += 3;
	}

	// scfsi
	bits += channels * 4;

	// gr loop, channel loop
	bits += 2 * channels 
		* (12 + 9 + 8 + 4 + 1 + 22 + 1 + 1 + 1);

	return (bits + 7) / 8;
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
	frameSize = Mp3GetFrameSize(header);

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

	(*pSamplingRate) = Mp3GetHdrSamplingRate(header);
	(*pSamplingWindow) = Mp3GetHdrSamplingWindow(header);
	(*pVersion) = Mp3GetHdrVersion(header);

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

	(*pLayer) = (header >> 17) & 0x3; 

	/* rewind the file to where we were */
	fsetpos(inFile, &curPos);

	return true;
}

u_int8_t Mp3ToMp4AudioType(u_int8_t mpegVersion)
{
	u_int8_t audioType = MP4_INVALID_AUDIO_TYPE;

	switch (mpegVersion) {
	case 3:
		audioType = MP4_MPEG1_AUDIO_TYPE;
		break;
	case 2:
	case 0:
		audioType = MP4_MPEG2_AUDIO_TYPE;
		break;
	case 1:
		break;
	default:
		ASSERT(false);
	}
	return audioType;
}

MP4TrackId Mp3Creator(MP4FileHandle mp4File, FILE* inFile)
{
	u_int8_t mpegLayer;

	if (!GetMpegLayer(inFile, &mpegLayer)) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be MPEG audio\n", ProgName);
		exit(EXIT_MP3_CREATOR);
	}
	if (mpegLayer == 0) {
		fprintf(stderr,	
			"%s: data in file appears to be AAC audio, use .aac extension\n",
			 ProgName);
		exit(EXIT_MP3_CREATOR);
	}

	u_int16_t samplesPerSecond;
	u_int16_t samplesPerFrame;
	u_int8_t mpegVersion;

	if (!GetMp3SamplingParams(inFile, 
	  &samplesPerSecond, &samplesPerFrame, &mpegVersion)) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be valid audio\n",
			 ProgName);
		exit(EXIT_MP3_CREATOR);
	}

	u_int8_t audioType = Mp3ToMp4AudioType(mpegVersion);

	if (audioType == MP4_INVALID_AUDIO_TYPE) {
		fprintf(stderr,	
			"%s: data in file doesn't appear to be valid audio\n",
			 ProgName);
		exit(EXIT_MP3_CREATOR);
	}

	MP4TrackId trackId = 
		MP4AddAudioTrack(mp4File, 
			samplesPerSecond, samplesPerFrame, audioType);

	if (trackId == MP4_INVALID_TRACK_ID) {
		fprintf(stderr,	
			"%s: can't create audio track\n", ProgName);
		exit(EXIT_MP3_CREATOR);
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
		MP4SetAudioProfileLevel(mp4File, 0xFE);
	}

	u_int8_t sampleBuffer[8 * 1024];
	u_int32_t sampleSize = sizeof(sampleBuffer);
	MP4SampleId sampleId = 1;

	while (LoadNextMp3Frame(inFile, sampleBuffer, &sampleSize)) {
		if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize)) {
			fprintf(stderr,	
				"%s: can't write audio frame %u\n", ProgName, sampleId);
			exit(EXIT_MP3_CREATOR);
		}
		sampleId++;
		sampleSize = sizeof(sampleBuffer);
	}

	return trackId;
}
