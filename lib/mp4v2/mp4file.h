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

// forward declarations
class MP4Atom;
class MP4Property;
class MP4Float32Property;
class MP4StringProperty;
class MP4BytesProperty;
class MP4Descriptor;

class MP4File {
public:
	MP4File(u_int32_t verbosity = 0);
	~MP4File();

	/* file operations */
	void Read(const char* fileName);
	void Create(const char* fileName, bool use64bits);
	void Clone(const char* existingFileName, const char* newFileName);
	void Dump(FILE* pDumpFile = NULL, bool dumpImplicits = false);
	void Close();

	/* library property per file */

	u_int32_t GetVerbosity() {
		return m_verbosity;
	}
	void SetVerbosity(u_int32_t verbosity) {
		m_verbosity = verbosity;
	}

	bool Use64Bits() {
		return m_use64bits;
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

	// file level convenience functions

	MP4Duration GetDuration();
	void SetDuration(MP4Duration value);

	u_int32_t GetTimeScale();
	void SetTimeScale(u_int32_t value);

	u_int8_t GetODProfileLevel();
	void SetODProfileLevel(u_int8_t value);

	u_int8_t GetSceneProfileLevel();
	void SetSceneProfileLevel(u_int8_t value);

	u_int8_t GetVideoProfileLevel();
	void SetVideoProfileLevel(u_int8_t value);

	u_int8_t GetAudioProfileLevel();
	void SetAudioProfileLevel(u_int8_t value);

	u_int8_t GetGraphicsProfileLevel();
	void SetGraphicsProfileLevel(u_int8_t value);


	/* track operations */

	MP4TrackId AddTrack(char* type, u_int32_t timeScale = 1);
	void DeleteTrack(MP4TrackId trackId);

	u_int32_t GetNumberOfTracks(char* type = NULL);

	MP4TrackId AllocTrackId();
	MP4TrackId FindTrackId(u_int16_t trackIndex, char* type = NULL);
	u_int16_t FindTrackIndex(MP4TrackId trackId);
	u_int16_t FindTrakAtomIndex(MP4TrackId trackId);

	/* track properties */

	u_int64_t GetTrackIntegerProperty(
		MP4TrackId trackId, char* name);
	float GetTrackFloatProperty(
		MP4TrackId trackId, char* name);
	const char* GetTrackStringProperty(
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

	u_int32_t GetSampleSize(MP4TrackId trackId, MP4SampleId sampleId);

	u_int32_t GetMaxSampleSize(MP4TrackId trackId);

	MP4SampleId GetSampleIdFromTime(MP4TrackId trackId, 
		MP4Timestamp when, bool wantSyncSample = false);

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

	/* track level convenience functions */

	MP4TrackId AddSystemsTrack(char* type);

	MP4TrackId AddODTrack();

	MP4TrackId AddSceneTrack();

	MP4TrackId AddAudioTrack(u_int32_t timeScale, u_int32_t sampleDuration,
		u_int8_t audioType);

	MP4TrackId AddVideoTrack(u_int32_t timeScale, u_int32_t sampleDuration,
		u_int16_t width, u_int16_t height, u_int8_t videoType);

	MP4TrackId AddHintTrack(MP4TrackId refTrackId);

	MP4SampleId GetNumberOfTrackSamples(MP4TrackId trackId);

	const char* GetTrackType(MP4TrackId trackId);

	MP4Duration GetTrackDuration(MP4TrackId trackId);

	u_int32_t GetTrackTimeScale(MP4TrackId trackId);
	void SetTrackTimeScale(MP4TrackId trackId, u_int32_t value);

	u_int8_t GetTrackAudioType(MP4TrackId trackId);
	u_int8_t GetTrackVideoType(MP4TrackId trackId);

	MP4Duration GetTrackFixedSampleDuration(MP4TrackId trackId);

	void GetTrackESConfiguration(MP4TrackId trackId, 
		u_int8_t** ppConfig, u_int32_t* pConfigSize);
	void SetTrackESConfiguration(MP4TrackId trackId, 
		u_int8_t* pConfig, u_int32_t configSize);

	// time convenience functions

	u_int64_t ConvertFromMovieDuration(
		MP4Duration duration,
		u_int32_t timeScale);

	u_int64_t ConvertFromTrackTimestamp(
		MP4TrackId trackId, 
		MP4Timestamp timeStamp,
		u_int32_t timeScale);

	MP4Timestamp ConvertToTrackTimestamp(
		MP4TrackId trackId, 
		u_int64_t timeStamp,
		u_int32_t timeScale);

	u_int64_t ConvertFromTrackDuration(
		MP4TrackId trackId, 
		MP4Duration duration,
		u_int32_t timeScale);

	MP4Duration ConvertToTrackDuration(
		MP4TrackId trackId, 
		u_int64_t duration,
		u_int32_t timeScale);

	// specialized operations

	void MakeIsmaCompliant();

	/* "protected" interface to be used only by friends in library */

	u_int64_t GetPosition();
	void SetPosition(u_int64_t pos);

	u_int64_t GetSize();

	u_int32_t ReadBytes(u_int8_t* pBytes, u_int32_t numBytes, 
		FILE* pFile = NULL);
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
	void FlushReadBits();
	u_int32_t ReadMpegLength();

	u_int32_t PeekBytes(u_int8_t* pBytes, u_int32_t numBytes);

	void EnableWriteBuffer();
	void GetWriteBuffer(u_int8_t** ppBytes, u_int64_t* pNumBytes);
	void DisableWriteBuffer();

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
	void PadWriteBits(u_int8_t pad = 0);
	void FlushWriteBits();
	void WriteMpegLength(u_int32_t value, bool compact = false);

	MP4Duration UpdateDuration(MP4Duration duration);

protected:
	void Open();
	void ReadFromFile();
	void BeginWrite();
	void FinishWrite();
	void CacheProperties();

	MP4Atom* FindAtom(char* name);

	MP4Atom* AddAtom(char* parentName, char* childName);
	MP4Atom* AddAtom(MP4Atom* pParentAtom, char* childName);

	MP4Atom* InsertAtom(char* parentName, char* childName, u_int32_t index);
	MP4Atom* InsertAtom(MP4Atom* pParentAtom, char* childName, u_int32_t index);

	void FindIntegerProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);
	void FindFloatProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);
	void FindStringProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);
	void FindBytesProperty(char* name, 
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	bool FindProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

	void AddTrackToIod(MP4TrackId trackId);

	void AddTrackToOd(MP4TrackId trackId);

	void GetTrackReferenceProperties(char* trefName,
		MP4Property** ppCountProperty, MP4Property** ppTrackIdProperty);

	void AddTrackReference(char* trefName, MP4TrackId refTrackId);

	u_int32_t FindTrackReference(char* trefName, MP4TrackId refTrackId);

	char* MakeTrackName(MP4TrackId trackId, char* name);

	u_int8_t ConvertTrackTypeToStreamType(const char* trackType);

	void WriteIsmaODUpdateCommand(MP4TrackId odTrackId,
		MP4TrackId audioTrackId, MP4TrackId videoTrackId);

	void WriteODCommand(MP4TrackId odTrackId,
		MP4Descriptor* pDescriptor);

	void WriteIsmaSceneCommand(MP4TrackId sceneTrackId,
		MP4TrackId audioTrackId, MP4TrackId videoTrackId);

protected:
	char*			m_fileName;
	FILE*			m_pFile;
	u_int64_t		m_fileSize;
	MP4Atom*		m_pRootAtom;
	MP4Integer32Array m_trakIds;
	MP4TrackArray	m_pTracks;
	MP4TrackId		m_odTrackId;
	u_int32_t		m_verbosity;
	char			m_mode;
	bool			m_use64bits;
	bool			m_useIsma;

	// cached properties
	MP4IntegerProperty*		m_pModificationProperty;
	MP4Integer32Property*	m_pTimeScaleProperty;
	MP4IntegerProperty*		m_pDurationProperty;

	// used when cloning
	char*		m_mdatFileName;
	FILE*		m_pMdatFile;

	// write buffering
	u_int8_t*	m_writeBuffer;
	u_int64_t	m_writeBufferSize;
	u_int64_t	m_writeBufferMaxSize;

	// bit read/write buffering
	u_int8_t	m_numReadBits;
	u_int8_t	m_bufReadBits;
	u_int8_t	m_numWriteBits;
	u_int8_t	m_bufWriteBits;
};

#endif /* __MP4_FILE_INCLUDED__ */
