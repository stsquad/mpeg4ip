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

#include <sys/types.h>
#include <stdio.h>

#include "mpeg4ip.h"

typedef void* MP4FileHandle;
typedef u_int16_t MP4TrackId;
typedef u_int32_t MP4SampleId;

/* declaration of C++ classes */

#ifdef __cplusplus
#include "mp4util.h"
#include "mp4file.h"
#endif

/* declaration of C interface functions */

#ifdef __cplusplus
extern "C" {
#endif

/* file operations */

MP4FileHandle MP4Open(char* fileName, char* mode);

int MP4Read(MP4FileHandle hFile);

int MP4Write(MP4FileHandle hFile);

int MP4Close(MP4FileHandle hFile);

int MP4Dump(MP4FileHandle hFile, FILE* pDumpFile);

/* file properties */

u_int32_t MP4GetVerbosity(MP4FileHandle hFile);
void MP4SetVerbosity(MP4FileHandle hFile, u_int32_t verbosity);

u_int64_t MP4GetIntegerProperty(
	MP4FileHandle hFile, char* propName);
//LATER float MP4GetFloatProperty(
	// LATER MP4FileHandle hFile, char* propName);
const char* MP4GetStringProperty(
	MP4FileHandle hFile, char* propName);
void MP4GetBytesProperty(
	MP4FileHandle hFile, char* propName,
	u_int8_t** ppValue, u_int32_t* pValueSize);

void MP4SetIntegerProperty(
	MP4FileHandle hFile, char* propName, int64_t value);
// LATER void MP4SetFloatProperty(
	// LATER MP4FileHandle hFile, char* propName, float value);
void MP4SetStringProperty(
	MP4FileHandle hFile, char* propName, char* value);
void MP4SetBytesProperty(
	MP4FileHandle hFile, char* propName, u_int8_t* pValue);

/* track operations */

void MP4AddTrack(
	MP4FileHandle hFile, char* type);
void MP4DeleteTrack(
	MP4FileHandle hFile, MP4TrackId trackId);
MP4TrackId MP4FindTrack(
	MP4FileHandle hFile, u_int16_t index, char* type);
u_int32_t MP4GetNumTracks(
	MP4FileHandle hFile, char* type);

/* track properties */

u_int64_t MP4GetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName);
// LATER float MP4GetTrackFloatProperty(
	// LATER MP4FileHandle hFile, MP4TrackId trackId, 
	// LATER char* propName);
char* MP4GetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName);
u_int8_t* MP4GetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName);

void MP4SetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, int64_t value);
// LATER void MP4SetTrackFloatProperty(
	// LATER MP4FileHandle hFile, MP4TrackId trackId, 
	// LATER char* propName, float value);
void MP4SetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, char* value);
void MP4SetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, u_int8_t* pValue);

/* sample operations */

void MP4ReadSample(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t** ppBytes, u_int32_t* pNumBytes);
void MP4WriteSample(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t* pBytes, u_int32_t numBytes);

/* sample properties */

u_int64_t MP4GetSampleIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	char* propName);
// LATER float MP4GetSampleFloatProperty(
	// LATER MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	// LATER char* propName);
char* MP4GetSampleStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	char* propName);
u_int8_t* MP4GetSampleBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	char* propName);

void MP4SetSampleIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	char* propName, int64_t value);
// LATER void MP4SetSampleFloatProperty(
	// LATER MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	// LATER char* propName, float value);
void MP4SetSampleStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	char* propName, char* value);
void MP4SetSampleBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, MP4SampleId sampleId, 
	char* propName, u_int8_t* pValue);

#ifdef __cplusplus
}
#endif

#endif /* __MP4_INCLUDED__ */
