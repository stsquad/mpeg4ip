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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __MP4_INCLUDED__
#define __MP4_INCLUDED__

/* include system and project specific headers */
#include "mpeg4ip.h"

#include <math.h>	/* to define float HUGE_VAL and/or NAN */
#ifndef NAN
#define NAN HUGE_VAL
#endif

#ifdef __cplusplus
/* exploit C++ ability of default values for function parameters */
#define DEFAULT(x)	=x
#else
#define DEFAULT(x)
#endif

/* MP4 API types */

typedef void*		MP4FileHandle;
typedef u_int32_t	MP4TrackId;
typedef u_int32_t	MP4SampleId;
typedef u_int64_t	MP4Timestamp;
typedef int64_t		MP4Duration;

/* MP4 verbosity levels, used as input to MP4Open() and MP4SetVerbosity() */
#define MP4_DETAILS_ALL				0xFFFF
#define MP4_DETAILS_ERROR			0x0001
#define MP4_DETAILS_WARNING			0x0002
#define MP4_DETAILS_READ			0x0004
#define MP4_DETAILS_WRITE			0x0008
#define MP4_DETAILS_FIND			0x0010
#define MP4_DETAILS_TABLE			0x0020
#define MP4_DETAILS_SAMPLE			0x0040
#define MP4_DETAILS_READ_ALL		\
	(MP4_DETAILS_READ | MP4_DETAILS_TABLE | MP4_DETAILS_SAMPLE)
#define MP4_DETAILS_WRITE_ALL		\
	(MP4_DETAILS_WRITE | MP4_DETAILS_TABLE | MP4_DETAILS_SAMPLE)


/* MP4 API declarations */

#ifdef __cplusplus
extern "C" {
#endif

/* file operations */

MP4FileHandle MP4Read(char* fileName, u_int32_t verbosity DEFAULT(0));

MP4FileHandle MP4Create(char* fileName, u_int32_t verbosity DEFAULT(0));

MP4FileHandle MP4Clone(char* existingFileName, char* newFileName, 
	u_int32_t verbosity DEFAULT(0));

int MP4Close(MP4FileHandle hFile);

int MP4Dump(MP4FileHandle hFile, FILE* pDumpFile DEFAULT(NULL));

/* file properties */

/* specific file properties */

u_int32_t MP4GetVerbosity(MP4FileHandle hFile);

bool MP4SetVerbosity(MP4FileHandle hFile, u_int32_t verbosity);

MP4Duration MP4GetDuration(MP4FileHandle hFile);

u_int32_t MP4GetTimeScale(MP4FileHandle hFile);

bool MP4SetTimeScale(MP4FileHandle hFile, u_int32_t value);

u_int8_t MP4GetODProfileLevel(MP4FileHandle hFile);

bool MP4SetODProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetSceneProfileLevel(MP4FileHandle hFile);

bool MP4SetSceneProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetVideoProfileLevel(MP4FileHandle hFile);

bool MP4SetVideoProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetAudioProfileLevel(MP4FileHandle hFile);

bool MP4SetAudioProfileLevel(MP4FileHandle hFile, u_int8_t value);

u_int8_t MP4GetGraphicsProfileLevel(MP4FileHandle hFile);

bool MP4SetGraphicsProfileLevel(MP4FileHandle hFile, u_int8_t value);

/* generic file properties */

u_int64_t MP4GetIntegerProperty(
	MP4FileHandle hFile, char* propName);

float MP4GetFloatProperty(
	MP4FileHandle hFile, char* propName);

const char* MP4GetStringProperty(
	MP4FileHandle hFile, char* propName);

void MP4GetBytesProperty(
	MP4FileHandle hFile, char* propName,
	u_int8_t** ppValue, u_int32_t* pValueSize);

bool MP4SetIntegerProperty(
	MP4FileHandle hFile, char* propName, int64_t value);

bool MP4SetFloatProperty(
	MP4FileHandle hFile, char* propName, float value);

bool MP4SetStringProperty(
	MP4FileHandle hFile, char* propName, char* value);

bool MP4SetBytesProperty(
	MP4FileHandle hFile, char* propName, u_int8_t* pValue, u_int32_t valueSize);

/* track operations */

MP4TrackId MP4AddTrack(
	MP4FileHandle hFile, char* type);

MP4TrackId MP4AddAudioTrack(
	MP4FileHandle hFile, u_int32_t timeScale, u_int32_t sampleDuration);

MP4TrackId MP4AddVideoTrack(
	MP4FileHandle hFile, u_int32_t timeScale, u_int32_t sampleDuration,
	u_int16_t width, u_int16_t height);

MP4TrackId MP4AddHintTrack(
	MP4FileHandle hFile, MP4TrackId refTrackId);

bool MP4DeleteTrack(
	MP4FileHandle hFile, MP4TrackId trackId);

u_int32_t MP4GetNumberOfTracks(
	MP4FileHandle hFile, char* type DEFAULT(NULL));

MP4TrackId MP4FindTrackId(
	MP4FileHandle hFile, u_int16_t index, char* type DEFAULT(NULL));

u_int16_t MP4FindTrackIndex(
	MP4FileHandle hFile, MP4TrackId trackId);

/* track properties */

/* specific track properties */

MP4Duration MP4GetTrackDuration(
	MP4FileHandle hFile, MP4TrackId trackId);

u_int32_t MP4GetTrackTimeScale(
	MP4FileHandle hFile, MP4TrackId trackId);

bool MP4SetTrackTimeScale(
	MP4FileHandle hFile, MP4TrackId trackId, u_int32_t value);

/* returns zero if track samples do not have a fixed duration */
MP4Duration MP4GetTrackFixedSampleDuration(
	MP4FileHandle hFile, MP4TrackId trackId);

void MP4GetTrackESConfiguration(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t** ppConfig, u_int32_t* pConfigSize);

bool MP4SetTrackESConfiguration(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t* pConfig, u_int32_t configSize);

MP4SampleId MP4GetNumberOfTrackSamples(
	MP4FileHandle hFile, MP4TrackId trackId);

/* generic track properties */

u_int64_t MP4GetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName);

float MP4GetTrackFloatProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName);

const char* MP4GetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName);

void MP4GetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, char* propName,
	u_int8_t** ppValue, u_int32_t* pValueSize);

bool MP4SetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, int64_t value);

bool MP4SetTrackFloatProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, float value);

bool MP4SetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, char* value);

bool MP4SetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, u_int8_t* pValue, u_int32_t valueSize);

/* sample operations */

bool MP4ReadSample(
	/* input parameters */
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId,
	/* output parameters */
	u_int8_t** ppBytes, 
	u_int32_t* pNumBytes, 
	MP4Timestamp* pStartTime DEFAULT(NULL), 
	MP4Duration* pDuration DEFAULT(NULL),
	MP4Duration* pRenderingOffset DEFAULT(NULL), 
	bool* pIsSyncSample DEFAULT(NULL));

bool MP4WriteSample(
	MP4FileHandle hFile,
	MP4TrackId trackId,
	u_int8_t* pBytes, 
	u_int32_t numBytes,
	MP4Duration duration DEFAULT(0),
	MP4Duration renderingOffset DEFAULT(0), 
	bool isSyncSample DEFAULT(true));

MP4SampleId MP4GetSampleIdFromTime(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4Timestamp when, 
	bool wantSyncSample DEFAULT(false));

#ifdef __cplusplus
}
#endif

#endif /* __MP4_INCLUDED__ */
