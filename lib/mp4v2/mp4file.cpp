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

MP4File::MP4File(u_int32_t verbosity)
{
	m_fileName = NULL;
	m_pFile = NULL;
	m_fileSize = 0;
	m_pRootAtom = NULL;
	m_odTrackId = MP4_INVALID_TRACK_ID;

	m_verbosity = verbosity;
	m_mode = 0;
	m_use64bits = false;

	m_pModificationProperty = NULL;
	m_pTimeScaleProperty = NULL;
	m_pDurationProperty = NULL;

	m_numReadBits = 0;
	m_bufReadBits = 0;
	m_numWriteBits = 0;
	m_bufWriteBits = 0;
}

MP4File::~MP4File()
{
	MP4Free(m_fileName);
	delete m_pRootAtom;
	for (u_int32_t i = 0; i < m_pTracks.Size(); i++) {
		delete m_pTracks[i];
	}
}

void MP4File::Read(const char* fileName)
{
	m_fileName = MP4Stralloc(fileName);
	m_mode = 'r';

	Open();

	ReadFromFile();

	CacheProperties();
}

void MP4File::Create(const char* fileName, bool use64bits)
{
	m_fileName = MP4Stralloc(fileName);
	m_mode = 'w';
	m_use64bits = use64bits;

	Open();

	// generate a skeletal atom tree
	m_pRootAtom = MP4Atom::CreateAtom(NULL);
	m_pRootAtom->SetFile(this);
	m_pRootAtom->Generate();

	CacheProperties();

	// create mdat
	AddAtom(m_pRootAtom, "mdat");

	// write mdat's header
	BeginWrite();
}

void MP4File::Clone(const char* existingFileName, const char* newFileName)
{
	m_fileName = MP4Stralloc(existingFileName);
	m_mode = 'r';

	Open();
	ReadFromFile();

	m_mdatFileName = m_fileName;
	m_pMdatFile = m_pFile;
	m_fileName = NULL;
	m_pFile = NULL;

	m_fileName = MP4Stralloc(newFileName);
	m_mode = 'w';

	Open();

	CacheProperties();

	// create an mdat if one doesn't already exist
	if (m_pRootAtom->FindAtom("mdat") == NULL) {
		AddAtom(m_pRootAtom, "mdat");
	}

	BeginWrite();
}

void MP4File::Open()
{
	ASSERT(m_pFile == NULL);
	m_pFile = fopen(m_fileName, m_mode == 'r' ? "r" : "w");
	if (m_pFile == NULL) {
		throw new MP4Error(errno, "failed", "MP4Open");
	}

	if (m_mode == 'r') {
		struct stat s;
		if (fstat(fileno(m_pFile), &s) < 0) {
			throw new MP4Error(errno, "stat failed", "MP4Open");
		}
		m_fileSize = s.st_size;
	} else {
		m_fileSize = 0;
	}
}

void MP4File::ReadFromFile()
{
	// ensure we start at beginning of file
	SetPosition(0);

	// create a new root atom
	ASSERT(m_pRootAtom == NULL);
	m_pRootAtom = MP4Atom::CreateAtom(NULL);

	u_int64_t fileSize = GetSize();

	m_pRootAtom->SetFile(this);
	m_pRootAtom->SetStart(0);
	m_pRootAtom->SetSize(fileSize);
	m_pRootAtom->SetEnd(fileSize);

	m_pRootAtom->Read();

	// create MP4Track's for any tracks in the file
	u_int32_t trackIndex = 0;
	while (true) {
		char trackName[32];
		snprintf(trackName, sizeof(trackName), "moov.trak[%u]", trackIndex);

		MP4Atom* pTrakAtom = m_pRootAtom->FindAtom(trackName);
		if (pTrakAtom == NULL) {
			break;
		}

		MP4Integer32Property* pTrackIdProperty;
		if (pTrakAtom->FindProperty("trak.tkhd.trackId",
		  (MP4Property**)&pTrackIdProperty)) {

			m_trakIds.Add(pTrackIdProperty->GetValue());

			MP4Track* pTrack = NULL;
			try {
				pTrack = new MP4Track(this, pTrakAtom);
				m_pTracks.Add(pTrack);
			}
			catch (MP4Error* e) {
				VERBOSE_ERROR(m_verbosity, e->Print());
				delete e;
			}

			// remember when we encounter the OD track
			if (pTrack && !strcmp(pTrack->GetType(), MP4_OD_TRACK_TYPE)) {
				if (m_odTrackId == MP4_INVALID_TRACK_ID) {
					m_odTrackId = pTrackIdProperty->GetValue();
				} else {
					VERBOSE_READ(GetVerbosity(),
						printf("Warning: multiple OD tracks present\n"));
				}
			}
		} else {
			m_trakIds.Add(0);
		}

		trackIndex++;
	}
}

void MP4File::CacheProperties()
{
	FindIntegerProperty("moov.mvhd.modificationTime", 
		(MP4Property**)&m_pModificationProperty);

	FindIntegerProperty("moov.mvhd.timeScale", 
		(MP4Property**)&m_pTimeScaleProperty);

	FindIntegerProperty("moov.mvhd.duration", 
		(MP4Property**)&m_pDurationProperty);
}

void MP4File::BeginWrite()
{
	m_pRootAtom->BeginWrite();
}

MP4Duration MP4File::UpdateDuration(MP4Duration duration)
{
	MP4Duration currentDuration = GetDuration();
	if (duration > currentDuration) {
		SetDuration(duration);
		return duration;
	}
	return currentDuration;
}

void MP4File::FinishWrite()
{
	// for all tracks, flush chunking buffers
	for (u_int32_t i = 0; i < m_pTracks.Size(); i++) {
		ASSERT(m_pTracks[i]);
		m_pTracks[i]->FinishWrite();
	}

	// TBD for hint tracks, set values of hmhd

	// ask root atom to write
	m_pRootAtom->Write();
}

void MP4File::Dump(FILE* pDumpFile)
{
	if (pDumpFile == NULL) {
		pDumpFile = stdout;
	}

	fprintf(pDumpFile, "Dumping %s meta-information...\n", m_fileName);
	m_pRootAtom->Dump(pDumpFile);
}

void MP4File::Close()
{
	if (m_mode == 'w') {
		SetIntegerProperty("moov.mvhd.modificationTime", 
			MP4GetAbsTimestamp());

		FinishWrite();
	}

	fclose(m_pFile);
	m_pFile = NULL;
}

MP4Atom* MP4File::FindAtom(char* name)
{
	MP4Atom* pAtom;
	if (!name || !strcmp(name, "")) {
		pAtom = m_pRootAtom;
	} else {
		pAtom = m_pRootAtom->FindAtom(name);
	}
	ASSERT(pAtom);
	return pAtom;
}

MP4Atom* MP4File::AddAtom(char* parentName, char* childName)
{
	return AddAtom(FindAtom(parentName), childName);
}

MP4Atom* MP4File::AddAtom(MP4Atom* pParentAtom, char* childName)
{
	return InsertAtom(pParentAtom, childName, 
		pParentAtom->GetNumberOfChildAtoms());
}

MP4Atom* MP4File::InsertAtom(char* parentName, char* childName, 
	u_int32_t index)
{
	return InsertAtom(FindAtom(parentName), childName, index); 
}

MP4Atom* MP4File::InsertAtom(MP4Atom* pParentAtom, char* childName, 
	u_int32_t index)
{
	MP4Atom* pChildAtom = MP4Atom::CreateAtom(childName);

	pParentAtom->InsertChildAtom(pChildAtom, index);

	pChildAtom->Generate();

	return pChildAtom;
}

bool MP4File::FindProperty(char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (pIndex) {
		*pIndex = 0;	// set the default answer for index
	}

	return m_pRootAtom->FindProperty(name, ppProperty, pIndex);
}

void MP4File::FindIntegerProperty(char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property", "MP4File::FindIntegerProperty");
	}

	switch ((*ppProperty)->GetType()) {
	case Integer8Property:
	case Integer16Property:
	case Integer24Property:
	case Integer32Property:
	case Integer64Property:
		break;
	default:
		throw new MP4Error("type mismatch", "MP4File::FindIntegerProperty");
	}
}

u_int64_t MP4File::GetIntegerProperty(char* name)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindIntegerProperty(name, &pProperty, &index);

	return ((MP4IntegerProperty*)pProperty)->GetValue(index);
}

void MP4File::SetIntegerProperty(char* name, u_int64_t value)
{
	MP4Property* pProperty = NULL;
	u_int32_t index = 0;

	FindIntegerProperty(name, &pProperty, &index);

	((MP4IntegerProperty*)pProperty)->SetValue(value, index);
}

void MP4File::FindFloatProperty(char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property", "MP4File::FindFloatProperty");
	}
	if ((*ppProperty)->GetType() != Float32Property) {
		throw new MP4Error("type mismatch", "MP4File::FindFloatProperty");
	}
}

float MP4File::GetFloatProperty(char* name)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindFloatProperty(name, &pProperty, &index);

	return ((MP4Float32Property*)pProperty)->GetValue(index);
}

void MP4File::SetFloatProperty(char* name, float value)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindFloatProperty(name, &pProperty, &index);

	((MP4Float32Property*)pProperty)->SetValue(value, index);
}

void MP4File::FindStringProperty(char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property", "MP4File::FindStringProperty");
	}
	if ((*ppProperty)->GetType() != StringProperty) {
		throw new MP4Error("type mismatch", "MP4File::FindStringProperty");
	}
}

const char* MP4File::GetStringProperty(char* name)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindStringProperty(name, &pProperty, &index);

	return ((MP4StringProperty*)pProperty)->GetValue(index);
}

void MP4File::SetStringProperty(char* name, char* value)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindStringProperty(name, &pProperty, &index);

	((MP4StringProperty*)pProperty)->SetValue(value, index);
}

void MP4File::FindBytesProperty(char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property", "MP4File::FindBytesProperty");
	}
	if ((*ppProperty)->GetType() != BytesProperty) {
		throw new MP4Error("type mismatch", "MP4File::FindBytesProperty");
	}
}

void MP4File::GetBytesProperty(char* name, 
	u_int8_t** ppValue, u_int32_t* pValueSize)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindBytesProperty(name, &pProperty, &index);

	((MP4BytesProperty*)pProperty)->GetValue(ppValue, pValueSize, index);
}

void MP4File::SetBytesProperty(char* name, 
	u_int8_t* pValue, u_int32_t valueSize)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindBytesProperty(name, &pProperty, &index);

	((MP4BytesProperty*)pProperty)->SetValue(pValue, valueSize, index);
}


// track functions

MP4TrackId MP4File::AddTrack(char* type, u_int32_t timeScale)
{
	// create and add new trak atom
	MP4Atom* pTrakAtom = AddAtom("moov", "trak");

	// allocate a new track id
	MP4TrackId trackId = AllocTrackId();

	m_trakIds.Add(trackId);

	// set track id
	MP4Integer32Property* pInteger32Property = NULL;
	pTrakAtom->FindProperty(
		"trak.tkhd.trackId", (MP4Property**)&pInteger32Property);
	ASSERT(pInteger32Property);
	pInteger32Property->SetValue(trackId);

	// set track type
	MP4StringProperty* pStringProperty = NULL;
	pTrakAtom->FindProperty(
		"trak.mdia.hdlr.handlerType", (MP4Property**)&pStringProperty);
	ASSERT(pStringProperty);
	pStringProperty->SetValue(MP4Track::NormalizeTrackType(type));

	// set track time scale
	pInteger32Property = NULL;
	pTrakAtom->FindProperty(
		"trak.mdia.mdhd.timeScale", (MP4Property**)&pInteger32Property);
	ASSERT(pInteger32Property);
	pInteger32Property->SetValue(timeScale ? timeScale : 1);

	// now have enough to create MP4Track object
	MP4Track* pTrack = new MP4Track(this, pTrakAtom);
	m_pTracks.Add(pTrack);

	// mark track as enabled
	SetTrackIntegerProperty(trackId, "tkhd.flags", 1);

	// mark track as contained in this file
	SetTrackIntegerProperty(trackId, "mdia.minf.dinf.dref.flags", 1);

	return trackId;
}

void MP4File::AddTrackToIod(MP4TrackId trackId)
{
	MP4DescriptorProperty* pDescriptorProperty = NULL;
	m_pRootAtom->FindProperty("moov.iods.esIds", 
		(MP4Property**)&pDescriptorProperty);
	ASSERT(pDescriptorProperty);

	MP4Descriptor* pDescriptor = 
		pDescriptorProperty->AddDescriptor(MP4ESIDIncDescrTag);
	ASSERT(pDescriptor);

	MP4Integer32Property* pProperty = NULL;
	pDescriptor->FindProperty("id", 
		(MP4Property**)&pProperty);
	ASSERT(pProperty);

	pProperty->SetValue(trackId);
}

void MP4File::AddTrackToOd(MP4TrackId trackId)
{
	if (!m_odTrackId) {
		return;
	}

	AddTrackReference(MakeTrackName(m_odTrackId, "tref.mpod"), trackId);
}

void MP4File::AddTrackReference(char* trefName, MP4TrackId refTrackId)
{
	char propName[1024];

	snprintf(propName, sizeof(propName), "%s.%s", trefName, "entryCount");
	MP4Integer32Property* pCountProperty = NULL;
	m_pRootAtom->FindProperty(propName,
		(MP4Property**)&pCountProperty);
	ASSERT(pCountProperty);

	snprintf(propName, sizeof(propName), "%s.%s", trefName, "entries.trackId");
	MP4Integer32Property* pTrackIdProperty = NULL;
	m_pRootAtom->FindProperty(propName,
		(MP4Property**)&pTrackIdProperty);
	ASSERT(pTrackIdProperty);

	pTrackIdProperty->AddValue(refTrackId);
	pCountProperty->IncrementValue();
}

MP4TrackId MP4File::AddSystemsTrack(char* type)
{
	const char* normType = MP4Track::NormalizeTrackType(type); 

	MP4TrackId trackId = AddTrack(type);

	InsertAtom(MakeTrackName(trackId, "mdia.minf"), "nmhd", 0);

	AddAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "mp4s");

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4s.esds.ESID", trackId);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.objectTypeId", 
		MP4SystemsObjectType);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4s.esds.decConfigDescr.streamType", 
		ConvertTrackTypeToStreamType(normType));

	return trackId;
}

MP4TrackId MP4File::AddObjectDescriptionTrack()
{
	// until a demonstrated need emerges
	// we limit ourselves to one object description track
	if (m_odTrackId != MP4_INVALID_TRACK_ID) {
		throw new MP4Error("object description track already exists",
			"AddObjectDescriptionTrack");
	}

	m_odTrackId = AddSystemsTrack(MP4_OD_TRACK_TYPE);

	AddTrackToIod(m_odTrackId);

	MP4Atom* pTrefAtom = 
		AddAtom(MakeTrackName(m_odTrackId, NULL), "tref");

	AddAtom(pTrefAtom, "mpod");
	
	return m_odTrackId;
}

MP4TrackId MP4File::AddSceneDescriptionTrack()
{
	MP4TrackId trackId = AddSystemsTrack(MP4_SCENE_TRACK_TYPE);

	AddTrackToIod(trackId);
	AddTrackToOd(trackId);

	return trackId;
}

MP4TrackId MP4File::AddAudioTrack(u_int32_t timeScale, 
	u_int32_t sampleDuration, u_int8_t audioType)
{
	MP4TrackId trackId = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);

	AddTrackToIod(trackId);
	AddTrackToOd(trackId);

	SetTrackFloatProperty(trackId, "tkhd.volume", 1.0);

	InsertAtom(MakeTrackName(trackId, "mdia.minf"), "smhd", 0);

	AddAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "mp4a");

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4a.timeScale", timeScale);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4a.esds.ESID", trackId);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.objectTypeId", 
		audioType);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.streamType", 
		MP4AudioStreamType);

	m_pTracks[FindTrackIndex(trackId)]->
		SetFixedSampleDuration(sampleDuration);

	return trackId;
}

MP4TrackId MP4File::AddVideoTrack(
	u_int32_t timeScale, u_int32_t sampleDuration, 
	u_int16_t width, u_int16_t height, u_int8_t videoType)
{
	MP4TrackId trackId = AddTrack(MP4_VIDEO_TRACK_TYPE, timeScale);

	AddTrackToIod(trackId);
	AddTrackToOd(trackId);

	SetTrackFloatProperty(trackId, "tkhd.width", width);
	SetTrackFloatProperty(trackId, "tkhd.height", height);

	InsertAtom(MakeTrackName(trackId, "mdia.minf"), "vmhd", 0);

	AddAtom(MakeTrackName(trackId, "mdia.minf.stbl.stsd"), "mp4v");
	// TBD need to increment mdia.minf.stbl.stsd.entryCount
	// for this and other track types

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4v.width", width);
	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4v.height", height);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4v.esds.ESID", trackId);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.objectTypeId", 
		videoType);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.streamType", 
		MP4VisualStreamType);

	SetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsz.sampleSize", sampleDuration);

	m_pTracks[FindTrackIndex(trackId)]->
		SetFixedSampleDuration(sampleDuration);

	return trackId;
}

MP4TrackId MP4File::AddHintTrack(MP4TrackId refTrackId)
{
	// validate reference track id
	FindTrackIndex(refTrackId);

	MP4TrackId trackId = 
		AddTrack(MP4_HINT_TRACK_TYPE, GetTrackTimeScale(refTrackId));

	// mark track as disabled
	SetTrackIntegerProperty(trackId, "tkhd.flags", 0);

	InsertAtom(MakeTrackName(trackId, "mdia.minf"), "hmhd", 0);

	MP4Atom* pTrefAtom = 
		AddAtom(MakeTrackName(trackId, NULL), "tref");

	AddAtom(pTrefAtom, "hint");

	AddTrackReference(MakeTrackName(trackId, "tref.hint"), refTrackId);

	return trackId;
}

void MP4File::DeleteTrack(MP4TrackId trackId)
{
	u_int32_t trakIndex = FindTrakAtomIndex(trackId);
	u_int16_t trackIndex = FindTrackIndex(trackId);
	MP4Track* pTrack = m_pTracks[trackIndex];

	MP4Atom* pTrakAtom = pTrack->GetTrakAtom();
	ASSERT(pTrakAtom);

	MP4Atom* pMoovAtom = FindAtom("moov");

	// TBD DeleteTrack - remove track from iod and od?

	if (trackId == m_odTrackId) {
		m_odTrackId = 0;
	}

	pMoovAtom->DeleteChildAtom(pTrakAtom);

	m_trakIds.Delete(trakIndex);

	m_pTracks.Delete(trackIndex);

	delete pTrack;
	delete pTrakAtom;
}

u_int32_t MP4File::GetNumberOfTracks(char* type)
{
	if (type == NULL) {
		return m_pTracks.Size();
	} else {
		u_int16_t typeSeen = 0;
		const char* normType = MP4Track::NormalizeTrackType(type);

		for (u_int16_t i = 0; i < m_pTracks.Size(); i++) {
			if (!strcmp(normType, m_pTracks[i]->GetType())) {
				typeSeen++;
			}
		}
		return typeSeen;
	}
}

MP4TrackId MP4File::AllocTrackId()
{
	MP4TrackId trackId = GetIntegerProperty("moov.mvhd.nextTrackId");

	if (trackId <= 0xFFFF) {
		SetIntegerProperty("moov.mvhd.nextTrackId", trackId + 1);
	} else {
		// extremely rare case where we need to search for a track id
		for (u_int16_t i = 1; i <= 0xFFFF; i++) {
			try {
				FindTrackIndex(i);
			}
			catch (MP4Error* e) {
				trackId = i;
				delete e;
			}
		}
		// even more extreme case where mp4 file has 2^16 tracks in it
		if (trackId > 0xFFFF) {
			throw new MP4Error("too many exising tracks", "AddTrack");
		}
	}

	return trackId;
}

MP4TrackId MP4File::FindTrackId(u_int16_t trackIndex, char* type)
{
	if (type == NULL) {
		return m_pTracks[trackIndex]->GetId();
	} else {
		u_int16_t typeSeen = 0;
		const char* normType = MP4Track::NormalizeTrackType(type);

		for (u_int16_t i = 0; i < m_pTracks.Size(); i++) {
			if (!strcmp(normType, m_pTracks[i]->GetType())) {
				if (trackIndex == typeSeen) {
					return m_pTracks[i]->GetId();
				}
				typeSeen++;
			}
		}
		throw new MP4Error("Track index doesn't exist", "FindTrackId"); 
	}
}

u_int16_t MP4File::FindTrackIndex(MP4TrackId trackId)
{
	for (u_int16_t i = 0; i < m_pTracks.Size(); i++) {
		if (m_pTracks[i]->GetId() == trackId) {
			return i;
		}
	}
	
	throw new MP4Error("Track id doesn't exist", "FindTrackIndex"); 
}

u_int16_t MP4File::FindTrakAtomIndex(MP4TrackId trackId)
{
	if (trackId) {
		for (u_int32_t i = 0; i < m_trakIds.Size(); i++) {
			if (m_trakIds[i] == trackId) {
				return i;
			}
		}
	}

	throw new MP4Error("Track id doesn't exist", "FindTrakAtomIndex"); 
}

u_int32_t MP4File::GetSampleSize(MP4TrackId trackId, MP4SampleId sampleId)
{
	return m_pTracks[FindTrackIndex(trackId)]->GetSampleSize(sampleId);
}

u_int32_t MP4File::GetMaxSampleSize(MP4TrackId trackId)
{
	return m_pTracks[FindTrackIndex(trackId)]->GetMaxSampleSize();
}

MP4SampleId MP4File::GetSampleIdFromTime(MP4TrackId trackId, 
	MP4Timestamp when, bool wantSyncSample)
{
	return m_pTracks[FindTrackIndex(trackId)]->
		GetSampleIdFromTime(when, wantSyncSample);
}

void MP4File::ReadSample(MP4TrackId trackId, MP4SampleId sampleId,
		u_int8_t** ppBytes, u_int32_t* pNumBytes, 
		MP4Timestamp* pStartTime, MP4Duration* pDuration,
		MP4Duration* pRenderingOffset, bool* pIsSyncSample)
{
	m_pTracks[FindTrackIndex(trackId)]->
		ReadSample(sampleId, ppBytes, pNumBytes, 
			pStartTime, pDuration, pRenderingOffset, pIsSyncSample);
}

void MP4File::WriteSample(MP4TrackId trackId,
		u_int8_t* pBytes, u_int32_t numBytes,
		MP4Duration duration, MP4Duration renderingOffset, bool isSyncSample)
{
	m_pTracks[FindTrackIndex(trackId)]->
		WriteSample(pBytes, numBytes, duration, renderingOffset, isSyncSample);

	m_pModificationProperty->SetValue(MP4GetAbsTimestamp());
}

char* MP4File::MakeTrackName(MP4TrackId trackId, char* name)
{
	u_int16_t trakIndex = FindTrakAtomIndex(trackId);

	static char trakName[1024];
	if (name == NULL || name[0] == '\0') {
		snprintf(trakName, sizeof(trakName), 
			"moov.trak[%u]", trakIndex);
	} else {
		snprintf(trakName, sizeof(trakName), 
			"moov.trak[%u].%s", trakIndex, name);
	}
	return trakName;
}

u_int64_t MP4File::GetTrackIntegerProperty(MP4TrackId trackId, char* name)
{
	return GetIntegerProperty(MakeTrackName(trackId, name));
}

void MP4File::SetTrackIntegerProperty(MP4TrackId trackId, char* name, 
	int64_t value)
{
	SetIntegerProperty(MakeTrackName(trackId, name), value);
}

float MP4File::GetTrackFloatProperty(MP4TrackId trackId, char* name)
{
	return GetFloatProperty(MakeTrackName(trackId, name));
}

void MP4File::SetTrackFloatProperty(MP4TrackId trackId, char* name, 
	float value)
{
	SetFloatProperty(MakeTrackName(trackId, name), value);
}

const char* MP4File::GetTrackStringProperty(MP4TrackId trackId, char* name)
{
	return GetStringProperty(MakeTrackName(trackId, name));
}

void MP4File::SetTrackStringProperty(MP4TrackId trackId, char* name,
	char* value)
{
	SetStringProperty(MakeTrackName(trackId, name), value);
}

void MP4File::GetTrackBytesProperty(MP4TrackId trackId, char* name, 
	u_int8_t** ppValue, u_int32_t* pValueSize)
{
	GetBytesProperty(MakeTrackName(trackId, name), ppValue, pValueSize);
}

void MP4File::SetTrackBytesProperty(MP4TrackId trackId, char* name, 
	u_int8_t* pValue, u_int32_t valueSize)
{
	SetBytesProperty(MakeTrackName(trackId, name), pValue, valueSize);
}


// file level convenience functions

MP4Duration MP4File::GetDuration()
{
	return m_pDurationProperty->GetValue();
}

void MP4File::SetDuration(MP4Duration value)
{
	m_pDurationProperty->SetValue(value);
}

u_int32_t MP4File::GetTimeScale()
{
	return m_pTimeScaleProperty->GetValue();
}

void MP4File::SetTimeScale(u_int32_t value)
{
	if (value == 0) {
		throw new MP4Error("invalid value", "SetTimeScale");
	}
	m_pTimeScaleProperty->SetValue(value);
}

u_int8_t MP4File::GetODProfileLevel()
{
	return GetIntegerProperty("moov.iods.ODProfileLevelId");
}

void MP4File::SetODProfileLevel(u_int8_t value)
{
	SetIntegerProperty("moov.iods.ODProfileLevelId", value);
}
 
u_int8_t MP4File::GetSceneProfileLevel()
{
	return GetIntegerProperty("moov.iods.sceneProfileLevelId");
}

void MP4File::SetSceneProfileLevel(u_int8_t value)
{
	SetIntegerProperty("moov.iods.sceneProfileLevelId", value);
}
 
u_int8_t MP4File::GetVideoProfileLevel()
{
	return GetIntegerProperty("moov.iods.visualProfileLevelId");
}

void MP4File::SetVideoProfileLevel(u_int8_t value)
{
	SetIntegerProperty("moov.iods.visualProfileLevelId", value);
}
 
u_int8_t MP4File::GetAudioProfileLevel()
{
	return GetIntegerProperty("moov.iods.audioProfileLevelId");
}

void MP4File::SetAudioProfileLevel(u_int8_t value)
{
	SetIntegerProperty("moov.iods.audioProfileLevelId", value);
}
 
u_int8_t MP4File::GetGraphicsProfileLevel()
{
	return GetIntegerProperty("moov.iods.graphicsProfileLevelId");
}

void MP4File::SetGraphicsProfileLevel(u_int8_t value)
{
	SetIntegerProperty("moov.iods.graphicsProfileLevelId", value);
}
 

// track level convenience functions

MP4SampleId MP4File::GetNumberOfTrackSamples(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsz.sampleCount");
}

const char* MP4File::GetTrackType(MP4TrackId trackId)
{
	return m_pTracks[FindTrackIndex(trackId)]->GetType();
}

u_int32_t MP4File::GetTrackTimeScale(MP4TrackId trackId)
{
	return m_pTracks[FindTrackIndex(trackId)]->GetTimeScale();
}

void MP4File::SetTrackTimeScale(MP4TrackId trackId, u_int32_t value)
{
	if (value == 0) {
		throw new MP4Error("invalid value", "SetTrackTimeScale");
	}
	SetTrackIntegerProperty(trackId, "mdia.mdhd.timeScale", value);
}

MP4Duration MP4File::GetTrackDuration(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, "mdia.mdhd.duration");
}

u_int8_t MP4File::GetTrackAudioType(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.objectTypeId");
}

u_int8_t MP4File::GetTrackVideoType(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, 
		"mdia.minf.stbl.stsd.mp4v.esds.decConfigDescr.objectTypeId");
}

MP4Duration MP4File::GetTrackFixedSampleDuration(MP4TrackId trackId)
{
	return m_pTracks[FindTrackIndex(trackId)]->GetFixedSampleDuration();
}

void MP4File::GetTrackESConfiguration(MP4TrackId trackId, 
	u_int8_t** ppConfig, u_int32_t* pConfigSize)
{
	GetTrackBytesProperty(trackId, 
		"mdia.minf.stbl.stsd.*[0].esds.decConfigDescr.decSpecificInfo.info",
		ppConfig, pConfigSize);
}

void MP4File::SetTrackESConfiguration(MP4TrackId trackId, 
	u_int8_t* pConfig, u_int32_t configSize)
{
	SetTrackBytesProperty(trackId, 
		"mdia.minf.stbl.stsd.*[0].esds.decConfigDescr.decSpecificInfo.info",
		pConfig, configSize);
}

u_int64_t MP4File::ConvertFromMovieDuration(
	MP4Duration duration,
	u_int32_t timeScale)
{
	return MP4ConvertTime((u_int64_t)duration, 
		GetTimeScale(), timeScale);
}

u_int64_t MP4File::ConvertFromTrackTimestamp(
	MP4TrackId trackId, 
	MP4Timestamp timeStamp,
	u_int32_t timeScale)
{
	return MP4ConvertTime((u_int64_t)timeStamp, 
		GetTrackTimeScale(trackId), timeScale);
}

MP4Timestamp MP4File::ConvertToTrackTimestamp(
	MP4TrackId trackId, 
	u_int64_t timeStamp,
	u_int32_t timeScale)
{
	return (MP4Timestamp)MP4ConvertTime(timeStamp, 
		timeScale, GetTrackTimeScale(trackId));
}

u_int64_t MP4File::ConvertFromTrackDuration(
	MP4TrackId trackId, 
	MP4Duration duration,
	u_int32_t timeScale)
{
	return MP4ConvertTime((u_int64_t)duration, 
		GetTrackTimeScale(trackId), timeScale);
}

MP4Duration MP4File::ConvertToTrackDuration(
	MP4TrackId trackId, 
	u_int64_t duration,
	u_int32_t timeScale)
{
	return (MP4Duration)MP4ConvertTime(duration, 
		timeScale, GetTrackTimeScale(trackId));
}

u_int8_t MP4File::ConvertTrackTypeToStreamType(const char* trackType)
{
	u_int8_t streamType;

	if (!strcmp(trackType, MP4_OD_TRACK_TYPE)) {
		streamType = MP4ObjectDescriptionStreamType;
	} else if (!strcmp(trackType, MP4_SCENE_TRACK_TYPE)) {
		streamType = MP4SceneDescriptionStreamType;
	} else if (!strcmp(trackType, MP4_CLOCK_TRACK_TYPE)) {
		streamType = MP4ClockReferenceStreamType;
	} else if (!strcmp(trackType, MP4_MPEG7_TRACK_TYPE)) {
		streamType = MP4Mpeg7StreamType;
	} else if (!strcmp(trackType, MP4_OCI_TRACK_TYPE)) {
		streamType = MP4OCIStreamType;
	} else if (!strcmp(trackType, MP4_IPMP_TRACK_TYPE)) {
		streamType = MP4IPMPStreamType;
	} else if (!strcmp(trackType, MP4_MPEGJ_TRACK_TYPE)) {
		streamType = MP4MPEGJStreamType;
	} else {
		streamType = MP4UserPrivateStreamType;
	}

	return streamType;
}
