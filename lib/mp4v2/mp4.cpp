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

#include "mp4common.h"

#define PRINT_ERROR(e) \
	VERBOSE_ERROR(((MP4File*)hFile)->GetVerbosity(), e->Print());

/* file operations */

extern "C" MP4FileHandle MP4Read(char* fileName, u_int32_t verbosity)
{
	try {
		return (MP4FileHandle)(new MP4File(fileName, "r", verbosity));
	}
	catch (MP4Error* e) {
		VERBOSE_ERROR(verbosity, e->Print());
		return (MP4FileHandle)NULL;
	}
}

extern "C" MP4FileHandle MP4Create(char* fileName, u_int32_t verbosity)
{
	try {
		return (MP4FileHandle)(new MP4File(fileName, "w", verbosity));
	}
	catch (MP4Error* e) {
		VERBOSE_ERROR(verbosity, e->Print());
		return (MP4FileHandle)NULL;
	}
}

extern "C" MP4FileHandle MP4Clone(char* existingFileName, 
	char* newFileName, u_int32_t verbosity)
{
	try {
		return (MP4FileHandle)NULL;
			// TBD (new MP4File(existingFileName, newFileName, "c", verbosity));
	}
	catch (MP4Error* e) {
		VERBOSE_ERROR(verbosity, e->Print());
		return (MP4FileHandle)NULL;
	}
}

extern "C" int MP4Close(MP4FileHandle hFile)
{
	try {
		((MP4File*)hFile)->Close();
		delete (MP4File*)hFile;
		return 0;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return -1;
	}
}

extern "C" int MP4Dump(MP4FileHandle hFile, FILE* pDumpFile)
{
	try {
		((MP4File*)hFile)->Dump(pDumpFile);
		return 0;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return -1;
	}
}


/* specific file properties */

extern "C" u_int32_t MP4GetVerbosity(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetVerbosity();
	}
	catch (MP4Error* e) {
		return 0;
	}
}

extern "C" bool MP4SetVerbosity(MP4FileHandle hFile, u_int32_t verbosity)
{
	try {
		((MP4File*)hFile)->SetVerbosity(verbosity);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" MP4Duration MP4GetDuration(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetDuration();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" u_int32_t MP4GetTimeScale(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetTimeScale();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetTimeScale(MP4FileHandle hFile, u_int32_t value)
{
	try {
		((MP4File*)hFile)->SetTimeScale(value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" u_int8_t MP4GetODProfileLevel(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetODProfileLevel();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetODProfileLevel(MP4FileHandle hFile, u_int8_t value)
{
	try {
		((MP4File*)hFile)->SetODProfileLevel(value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" u_int8_t MP4GetSceneProfileLevel(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetSceneProfileLevel();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetSceneProfileLevel(MP4FileHandle hFile, u_int8_t value)
{
	try {
		((MP4File*)hFile)->SetSceneProfileLevel(value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" u_int8_t MP4GetVideoProfileLevel(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetVideoProfileLevel();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetVideoProfileLevel(MP4FileHandle hFile, u_int8_t value)
{
	try {
		((MP4File*)hFile)->SetVideoProfileLevel(value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" u_int8_t MP4GetAudioProfileLevel(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetAudioProfileLevel();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetAudioProfileLevel(MP4FileHandle hFile, u_int8_t value)
{
	try {
		((MP4File*)hFile)->SetAudioProfileLevel(value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" u_int8_t MP4GetGraphicsProfileLevel(MP4FileHandle hFile)
{
	try {
		return ((MP4File*)hFile)->GetGraphicsProfileLevel();
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetGraphicsProfileLevel(MP4FileHandle hFile, u_int8_t value)
{
	try {
		((MP4File*)hFile)->SetGraphicsProfileLevel(value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

/* generic file properties */

extern "C" u_int64_t MP4GetIntegerProperty(
	MP4FileHandle hFile, char* propName)
{
	try {
		return ((MP4File*)hFile)->GetIntegerProperty(propName);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return (u_int64_t)-1;
	}
}

extern "C" float MP4GetFloatProperty(
	MP4FileHandle hFile, char* propName)
{
	try {
		return ((MP4File*)hFile)->GetIntegerProperty(propName);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return NAN;
	}
}

extern "C" const char* MP4GetStringProperty(
	MP4FileHandle hFile, char* propName)
{
	try {
		return ((MP4File*)hFile)->GetStringProperty(propName);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return NULL;
	}
}

extern "C" void MP4GetBytesProperty(
	MP4FileHandle hFile, char* propName, 
	u_int8_t** ppValue, u_int32_t* pValueSize)
{
	try {
		((MP4File*)hFile)->GetBytesProperty(propName, ppValue, pValueSize);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		*ppValue = NULL;
		*pValueSize = 0;
	}
}

extern "C" bool MP4SetIntegerProperty(
	MP4FileHandle hFile, char* propName, int64_t value)
{
	try {
		((MP4File*)hFile)->SetIntegerProperty(propName, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" bool MP4SetFloatProperty(
	MP4FileHandle hFile, char* propName, float value)
{
	try {
		((MP4File*)hFile)->SetFloatProperty(propName, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" bool MP4SetStringProperty(
	MP4FileHandle hFile, char* propName, char* value)
{
	try {
		((MP4File*)hFile)->SetStringProperty(propName, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" bool MP4SetBytesProperty(
	MP4FileHandle hFile, char* propName, u_int8_t* pValue, u_int32_t valueSize)
{
	try {
		((MP4File*)hFile)->SetBytesProperty(propName, pValue, valueSize);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

/* track operations */

extern "C" MP4TrackId MP4AddTrack(
	MP4FileHandle hFile, char* type)
{
	try {
		return ((MP4File*)hFile)->AddTrack(type);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" MP4TrackId MP4AddAudioTrack(
	MP4FileHandle hFile, u_int32_t timeScale, u_int32_t sampleDuration)
{
	try {
		return ((MP4File*)hFile)->AddAudioTrack(timeScale, sampleDuration);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" MP4TrackId MP4AddVideoTrack(
	MP4FileHandle hFile, u_int32_t timeScale, u_int32_t sampleDuration,
	u_int16_t width, u_int16_t height)
{
	try {
		return ((MP4File*)hFile)->AddVideoTrack(
			timeScale, sampleDuration, width, height);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" MP4TrackId MP4AddHintTrack(
	MP4FileHandle hFile, MP4TrackId refTrackId)
{
	try {
		return ((MP4File*)hFile)->AddHintTrack(refTrackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4DeleteTrack(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		((MP4File*)hFile)->DeleteTrack(trackId);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" u_int32_t MP4GetNumberOfTracks(
	MP4FileHandle hFile, char* type)
{
	try {
		return ((MP4File*)hFile)->GetNumberOfTracks(type);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" MP4TrackId MP4FindTrackId(
	MP4FileHandle hFile, u_int16_t index, char* type)
{
	try {
		return ((MP4File*)hFile)->FindTrackId(index, type);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" u_int16_t MP4FindTrackIndex(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		return ((MP4File*)hFile)->FindTrackIndex(trackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return (u_int16_t)-1;
	}
}

/* specific track properties */

extern "C" const char* MP4GetTrackType(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		return ((MP4File*)hFile)->GetTrackType(trackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return NULL;
	}
}

extern "C" MP4Duration MP4GetTrackDuration(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		return ((MP4File*)hFile)->GetTrackDuration(trackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" u_int32_t MP4GetTrackTimeScale(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		return ((MP4File*)hFile)->GetTrackTimeScale(trackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" bool MP4SetTrackTimeScale(
	MP4FileHandle hFile, MP4TrackId trackId, u_int32_t value)
{
	try {
		((MP4File*)hFile)->SetTrackTimeScale(trackId, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" MP4Duration MP4GetTrackFixedSampleDuration(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		return ((MP4File*)hFile)->GetTrackFixedSampleDuration(trackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

extern "C" void MP4GetTrackESConfiguration(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t** ppConfig, u_int32_t* pConfigSize)
{
	try {
		((MP4File*)hFile)->GetTrackESConfiguration(
			trackId, ppConfig, pConfigSize);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		*ppConfig = NULL;
		*pConfigSize = 0;
	}
}

extern "C" bool MP4SetTrackESConfiguration(
	MP4FileHandle hFile, MP4TrackId trackId, 
	u_int8_t* pConfig, u_int32_t configSize)
{
	try {
		((MP4File*)hFile)->SetTrackESConfiguration(
			trackId, pConfig, configSize);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" MP4SampleId MP4GetNumberOfTrackSamples(
	MP4FileHandle hFile, MP4TrackId trackId)
{
	try {
		return ((MP4File*)hFile)->GetNumberOfTrackSamples(trackId);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}

/* generic track properties */

extern "C" u_int64_t MP4GetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName)
{
	try {
		return ((MP4File*)hFile)->GetTrackIntegerProperty(trackId, propName);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return (u_int64_t)-1;
	}
}

extern "C" float MP4GetTrackFloatProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName)
{
	try {
		return ((MP4File*)hFile)->GetTrackFloatProperty(trackId, propName);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return NAN;
	}
}

extern "C" const char* MP4GetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName)
{
	try {
		return ((MP4File*)hFile)->GetTrackStringProperty(trackId, propName);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return NULL;
	}
}

extern "C" void MP4GetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, char* propName, 
	u_int8_t** ppValue, u_int32_t* pValueSize)
{
	try {
		((MP4File*)hFile)->GetTrackBytesProperty(
			trackId, propName, ppValue, pValueSize);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		*ppValue = NULL;
		*pValueSize = 0;
	}
}

extern "C" bool MP4SetTrackIntegerProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, int64_t value)
{
	try {
		((MP4File*)hFile)->SetTrackIntegerProperty(trackId, propName, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" bool MP4SetTrackFloatProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, float value)
{
	try {
		((MP4File*)hFile)->SetTrackFloatProperty(trackId, propName, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" bool MP4SetTrackStringProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, char* value)
{
	try {
		((MP4File*)hFile)->SetTrackStringProperty(trackId, propName, value);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" bool MP4SetTrackBytesProperty(
	MP4FileHandle hFile, MP4TrackId trackId, 
	char* propName, u_int8_t* pValue, u_int32_t valueSize)
{
	try {
		((MP4File*)hFile)->SetTrackBytesProperty(
			trackId, propName, pValue, valueSize);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

/* sample operations */

extern "C" bool MP4ReadSample(
	/* input parameters */
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4SampleId sampleId,
	/* output parameters */
	u_int8_t** ppBytes, 
	u_int32_t* pNumBytes, 
	MP4Timestamp* pStartTime, 
	MP4Duration* pDuration,
	MP4Duration* pRenderingOffset, 
	bool* pIsSyncSample)
{
	try {
		((MP4File*)hFile)->ReadSample(trackId, sampleId, ppBytes, pNumBytes, 
			pStartTime, pDuration, pRenderingOffset, pIsSyncSample);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		*ppBytes = NULL;
		*pNumBytes = 0;
		return false;
	}
}

extern "C" bool MP4WriteSample(
	MP4FileHandle hFile,
	MP4TrackId trackId,
	u_int8_t* pBytes, 
	u_int32_t numBytes,
	MP4Duration duration DEFAULT(0),
	MP4Duration renderingOffset DEFAULT(0), 
	bool isSyncSample DEFAULT(true))
{
	try {
		((MP4File*)hFile)->WriteSample(trackId, pBytes, numBytes, 
			duration, renderingOffset, isSyncSample);
		return true;
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return false;
	}
}

extern "C" MP4SampleId MP4GetSampleIdFromTime(
	MP4FileHandle hFile,
	MP4TrackId trackId, 
	MP4Timestamp when, 
	bool wantSyncSample)
{
	try {
		return ((MP4File*)hFile)->GetSampleIdFromTime(
			trackId, when, wantSyncSample);
	}
	catch (MP4Error* e) {
		PRINT_ERROR(e);
		return 0;
	}
}
