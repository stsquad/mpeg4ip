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

#ifndef __MP4_FILE_INCLUDED__
#define __MP4_FILE_INCLUDED__

// MP4 API time types
typedef u_int64_t	MP4Timestamp;
typedef int64_t		MP4Duration;

// forward declarations
class MP4Atom;
class MP4RootAtom;
class MP4Property;
class MP4Float32Property;
class MP4StringProperty;
class MP4BytesProperty;

class MP4File {
public:
	MP4File(char* fileName, char* mode, u_int32_t verbosity = 0);
	~MP4File();

	/* file operations */
	int Read();
	int Write();
	int Dump(FILE* pDumpFile = NULL);

	/* library property per file */
	u_int32_t GetVerbosity() {
		return m_verbosity;
	}
	void SetVerbosity(u_int32_t verbosity) {
		m_verbosity = verbosity;
	}

	/* file properties */

	u_int64_t GetIntegerProperty(char* name);
	float GetFloatProperty(char* name);
	const char* GetStringProperty(char* name);
	void GetBytesProperty(char* name,
		u_int8_t** ppValue, u_int32_t* pValueSize);

	void SetIntegerProperty(char* name, u_int64_t value);
	void SetFloatProperty(char* name, float value);
	void SetStringProperty(char* name, char* value);
	void SetBytesProperty(char* name, u_int8_t* pValue, u_int32_t valueSize);

	/* track operations */

	void AddTrack(char* type);
	void DeleteTrack(MP4TrackId trackId);
	MP4TrackId FindTrack(u_int16_t index, char* type = NULL);
	u_int32_t GetNumTracks(char* type = NULL);
	void SetTrackPosition(MP4SampleId sampleId);
	void SetTrackTime(u_int64_t time);

	/* track properties */

	u_int64_t GetTrackIntegerProperty(
		MP4TrackId trackId, char* name);
	float GetTrackFloatProperty(
		MP4TrackId trackId, char* name);
	char* GetTrackStringProperty(
		MP4TrackId trackId, char* name);
	void GetTrackBytesProperty(
		MP4TrackId trackId, char* name,
		u_int8_t** ppValue, u_int32_t* pValueSize);

	void SetTrackIntegerProperty(
		MP4TrackId trackId, char* name, int64_t value);
	void SetTrackFloatProperty(
		MP4TrackId trackId, char* name, float value);
	void SetTrackStringProperty(
		MP4TrackId trackId, char* name, char* value);
	void SetTrackBytesProperty(
		MP4TrackId trackId, char* name, u_int8_t* pValue, u_int32_t valueSize);

	/* sample operations */
	MP4SampleId GetSampleIdFromTime(MP4Timestamp when);
	MP4SampleId GetSyncSampleIdFromTime(MP4Timestamp when);

	void ReadSample(
		// input parameters
		MP4TrackId trackId, 
		MP4SampleId sampleId,
		// output parameters
		u_int8_t** ppBytes, 
		u_int32_t* pNumBytes, 
		MP4Timestamp* pStartTime = NULL, 
		MP4Duration* pDuration = NULL,
		MP4Duration* pRenderingOffset = NULL, 
		bool* pIsSyncSample = NULL);

	void WriteSample(
		MP4TrackId trackId,
		u_int8_t* pBytes, 
		u_int32_t numBytes,
		MP4Duration duration = 0,
		MP4Duration renderingOffset = 0, 
		bool isSyncSample = true);

	// convenience functions
	MP4Duration GetDuration();
	u_int32_t GetTimeScale();

	u_int8_t GetVideoProfileLevel();
	void SetVideoProfileLevel(u_int8_t value);

	u_int8_t GetAudioProfileLevel();
	void SetAudioProfileLevel(u_int8_t value);

	void GetESConfiguration(MP4TrackId trackId, 
		u_int8_t** ppConfig, u_int32_t* pConfigSize);
	void SetESConfiguration(MP4TrackId trackId, 
		u_int8_t* pConfig, u_int32_t configSize);

	/* "protected" interface to be used only by friends in library */

	u_int64_t GetPosition();
	void SetPosition(u_int64_t pos);
	u_int64_t GetSize();

	u_int32_t ReadBytes(u_int8_t* pBytes, u_int32_t numBytes);
	u_int64_t ReadUInt(u_int8_t size);
	u_int8_t ReadUInt8();
	u_int16_t ReadUInt16();
	u_int32_t ReadUInt24();
	u_int32_t ReadUInt32();
	u_int64_t ReadUInt64();
	float ReadFixed16();
	float ReadFixed32();
	float ReadFloat();
	char* ReadString();
	char* ReadCountedString(
		u_int8_t charSize = 1, bool allowExpandedCount = false);
	u_int64_t ReadBits(u_int8_t numBits);
	u_int32_t ReadMpegLength();

	u_int32_t PeekBytes(u_int8_t* pBytes, u_int32_t numBytes);

	void WriteBytes(u_int8_t* pBytes, u_int32_t numBytes);
	void WriteUInt(u_int64_t value, u_int8_t size);
	void WriteUInt8(u_int8_t value);
	void WriteUInt16(u_int16_t value);
	void WriteUInt24(u_int32_t value);
	void WriteUInt32(u_int32_t value);
	void WriteUInt64(u_int64_t value);
	void WriteFixed16(float value);
	void WriteFixed32(float value);
	void WriteFloat(float value);
	void WriteString(char* string);
	void WriteCountedString(char* string, 
		u_int8_t charSize = 1, bool allowExpandedCount = false);
	void WriteBits(u_int64_t bits, u_int8_t numBits);
	void FlushWriteBits();
	void WriteMpegLength(u_int32_t value, bool compact = false);

protected:
	void Open(char* fileName, char* mode);

	void FindIntegerProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex);
	void FindFloatProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex);
	void FindStringProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex);
	void FindBytesProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex);

	bool FindProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

	char* MakeTrackName(MP4TrackId trackId, char* name);

protected:
	FILE*		m_pFile;
	MP4RootAtom* m_pRootAtom;
	u_int32_t	m_verbosity;

	u_int8_t	m_numReadBits;
	u_int8_t	m_bufReadBits;
	u_int8_t	m_numWriteBits;
	u_int8_t	m_bufWriteBits;
};

#ifdef NOTDEF
	SetTimeScale

	GetNumberOfTracks(char* type);

	AddVideoTrack(w, h, fps, timeScale)
	AddAudioTrack

	WriteSample
	WriteNextSample
#endif

#endif /* __MP4_FILE_INCLUDED__ */
