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

MP4File::MP4File(char* fileName, char* mode, u_int32_t verbosity)
{
	m_fileName = MP4Stralloc(fileName);
	m_pFile = NULL;
	m_verbosity = verbosity;

	m_numReadBits = 0;
	m_bufReadBits = 0;
	m_numWriteBits = 0;
	m_bufWriteBits = 0;

	Open(fileName, mode);

	if (strchr(mode, 'r')) {
		// for read-only or read-write files,
		// read the file info into memory
		Read();
	} else {
		// for write-only files, 
		// generate a skeletal atom tree
		m_pRootAtom = MP4Atom::CreateAtom(NULL);
		m_pRootAtom->SetFile(this);
		m_pRootAtom->Generate();
	}
}

MP4File::~MP4File()
{
	fclose(m_pFile);
	MP4Free(m_fileName);
}

void MP4File::Open(char* fileName, char* mode)
{
	ASSERT(m_pFile == NULL);
	m_pFile = fopen(fileName, mode);
	if (m_pFile == NULL) {
		VERBOSE_ERROR(m_verbosity, 
			fprintf(stderr, "Open: failed: %s\n", strerror(errno)));
		throw MP4Error(errno, "MP4Open");
	}
}

void MP4File::Read()
{
	try {
		// destroy any old information
		delete m_pRootAtom;
		for (u_int32_t i = 0; i < m_pTracks.Size(); i++) {
			delete m_pTracks[i];
		}
		m_pTracks.Resize(0);

		// create a new root atom
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

			// TBD catch errors and ignore trak?
			m_pTracks.Add(new MP4Track(this, pTrakAtom));
	
			trackIndex++;
		}
	}
	catch (MP4Error* e) {
		VERBOSE_ERROR(m_verbosity, e->Print());
		throw e;
	}
}

void MP4File::Write()
{
	try {
		m_pRootAtom->Write();
	}
	catch (MP4Error* e) {
		VERBOSE_ERROR(m_verbosity, e->Print());
		throw e;
	}
}

void MP4File::Dump(FILE* pDumpFile)
{
	if (pDumpFile == NULL) {
		pDumpFile = stdout;
	}

	try {
		fprintf(pDumpFile, "Dumping %s meta-information...\n", m_fileName);
		m_pRootAtom->Dump(pDumpFile);
	}
	catch (MP4Error* e) {
		VERBOSE_ERROR(m_verbosity, e->Print());
		throw e;
	}
}

u_int64_t MP4File::GetPosition()
{
	fpos_t fpos;
	if (fgetpos(m_pFile, &fpos) < 0) {
		throw new MP4Error(errno, "MP4GetPosition");
	}
	return fpos.__pos;
}

void MP4File::SetPosition(u_int64_t pos)
{
	fpos_t fpos;
	fpos.__pos = pos;
	if (fsetpos(m_pFile, &fpos) < 0) {
		throw new MP4Error(errno, "MP4SetPosition");
	}
}

u_int64_t MP4File::GetSize()
{
	struct stat s;
	if (fstat(fileno(m_pFile), &s) < 0) {
		throw new MP4Error(errno, "MP4GetSize");
	}
	return s.st_size;
}


u_int32_t MP4File::ReadBytes(u_int8_t* pBytes, u_int32_t numBytes)
{
	ASSERT(m_pFile);
	ASSERT(pBytes);
	ASSERT(numBytes > 0);
	WARNING(m_numReadBits > 0);

	u_int32_t rc;
	rc = fread(pBytes, 1, numBytes, m_pFile);
	if (rc == 0) {
		throw new MP4Error(errno, "MP4ReadBytes");
	}
	if (rc != numBytes) {
		throw new MP4Error(
			"not enough bytes, reached end-of-file",
			"MP4ReadBytes");
	}
	return rc;
}

u_int32_t MP4File::PeekBytes(u_int8_t* pBytes, u_int32_t numBytes)
{
	u_int64_t pos = GetPosition();
	ReadBytes(pBytes, numBytes);
	SetPosition(pos);
	return numBytes;
}

void MP4File::WriteBytes(u_int8_t* pBytes, u_int32_t numBytes)
{
	ASSERT(m_pFile);
	WARNING(pBytes == NULL);
	WARNING(numBytes == 0);
	WARNING(m_numWriteBits > 0 && m_numWriteBits < 8);

	if (pBytes == NULL || numBytes == 0) {
		return;
	}

	u_int32_t rc;
	rc = fwrite(pBytes, 1, numBytes, m_pFile);
	if (rc != numBytes) {
		throw new MP4Error(errno, "MP4WriteBytes");
	}
}

u_int64_t MP4File::ReadUInt(u_int8_t size)
{
	switch (size) {
	case 1:
		return ReadUInt8();
	case 2:
		return ReadUInt16();
	case 3:
		return ReadUInt24();
	case 4:
		return ReadUInt32();
	case 8:
		return ReadUInt64();
	default:
		ASSERT(false);
		return 0;
	}
}

void MP4File::WriteUInt(u_int64_t value, u_int8_t size)
{
	switch (size) {
	case 1:
		WriteUInt8(value);
	case 2:
		WriteUInt16(value);
	case 3:
		WriteUInt24(value);
	case 4:
		WriteUInt32(value);
	case 8:
		WriteUInt64(value);
	default:
		ASSERT(false);
	}
}

u_int8_t MP4File::ReadUInt8()
{
	u_int8_t data;
	ReadBytes(&data, 1);
	return data;
}

void MP4File::WriteUInt8(u_int8_t value)
{
	WriteBytes(&value, 1);
}

u_int16_t MP4File::ReadUInt16()
{
	u_int8_t data[2];
	ReadBytes(&data[0], 2);
	return ((data[0] << 8) | data[1]);
}

void MP4File::WriteUInt16(u_int16_t value)
{
	u_int8_t data[2];
	data[0] = (value >> 8) & 0xFF;
	data[1] = value & 0xFF;
	WriteBytes(data, 2);
}

u_int32_t MP4File::ReadUInt24()
{
	u_int8_t data[3];
	ReadBytes(&data[0], 3);
	return ((data[0] << 16) | (data[1] << 8) | data[2]);
}

void MP4File::WriteUInt24(u_int32_t value)
{
	u_int8_t data[3];
	data[0] = (value >> 16) & 0xFF;
	data[1] = (value >> 8) & 0xFF;
	data[2] = value & 0xFF;
	WriteBytes(data, 3);
}

u_int32_t MP4File::ReadUInt32()
{
	u_int8_t data[4];
	ReadBytes(&data[0], 4);
	return ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
}

void MP4File::WriteUInt32(u_int32_t value)
{
	u_int8_t data[4];
	data[0] = (value >> 24) & 0xFF;
	data[1] = (value >> 16) & 0xFF;
	data[2] = (value >> 8) & 0xFF;
	data[3] = value & 0xFF;
	WriteBytes(data, 4);
}

u_int64_t MP4File::ReadUInt64()
{
	u_int8_t data[8];
	u_int64_t result = 0;
	u_int64_t temp;

	ReadBytes(&data[0], 8);
	
	for (int i = 0; i < 8; i++) {
		temp = data[i];
		result |= temp << ((7 - i) * 8);
	}
	return result;
}

void MP4File::WriteUInt64(u_int64_t value)
{
	u_int8_t data[8];

	for (int i = 7; i >= 0; i--) {
		data[i] = value & 0xFF;
		value >>= 8;
	}
	WriteBytes(data, 8);
}

float MP4File::ReadFixed16()
{
	u_int8_t iPart = ReadUInt8();
	u_int8_t fPart = ReadUInt8();

	return iPart + (((float)fPart) / 0x100);
}

void MP4File::WriteFixed16(float value)
{
	if (value >= 0x100) {
		throw new MP4Error(ERANGE, "MP4WriteFixed16");
	}

	u_int8_t iPart = (u_int8_t)value;
	u_int8_t fPart = (u_int8_t)((value - iPart) * 0x100);

	WriteUInt8(iPart);
	WriteUInt8(fPart);
}

float MP4File::ReadFixed32()
{
	u_int16_t iPart = ReadUInt16();
	u_int16_t fPart = ReadUInt16();

	return iPart + (((float)fPart) / 0x10000);
}

void MP4File::WriteFixed32(float value)
{
	if (value >= 0x10000) {
		throw new MP4Error(ERANGE, "MP4WriteFixed32");
	}

	u_int16_t iPart = (u_int16_t)value;
	u_int16_t fPart = (u_int16_t)((value - iPart) * 0x10000);

	WriteUInt16(iPart);
	WriteUInt16(fPart);
}

float MP4File::ReadFloat()
{
	union {
		float f;
		u_int32_t i;
	} u;

	u.i = ReadUInt32();
	return u.f;
}

void MP4File::WriteFloat(float value)
{
	union {
		float f;
		u_int32_t i;
	} u;

	u.f = value;
	WriteUInt32(u.i);
}

char* MP4File::ReadString()
{
	u_int32_t length = 0;
	u_int32_t alloced = 64;
	char* data = (char*)MP4Malloc(alloced);

	do {
		if (length == alloced) {
			data = (char*)MP4Realloc(data, alloced * 2);
		}
		ReadBytes((u_int8_t*)&data[length], 1);
		length++;
	} while (data[length - 1] != 0);

	data = (char*)MP4Realloc(data, length);
	return data;
}

void MP4File::WriteString(char* string)
{
	if (string == NULL) {
		u_int8_t zero = 0;
		WriteBytes(&zero, 1);
	} else {
		WriteBytes((u_int8_t*)string, strlen(string) + 1);
	}
}

char* MP4File::ReadCountedString(u_int8_t charSize, bool allowExpandedCount)
{
	u_int32_t charLength;
	if (allowExpandedCount) {
		u_int8_t b;
		charLength = 0;
		do {
			b = ReadUInt8();
			charLength += b;
		} while (b == 255);
	} else {
		charLength = ReadUInt8();
	}

	u_int32_t byteLength = charLength * charSize;
	char* data = (char*)MP4Malloc(byteLength + 1);
	if (byteLength > 0) {
		ReadBytes((u_int8_t*)data, byteLength);
	}
	data[byteLength] = '\0';
	return data;
}

void MP4File::WriteCountedString(char* string, 
	u_int8_t charSize, bool allowExpandedCount)
{
	u_int32_t byteLength = strlen(string);
	u_int32_t charLength = byteLength / charSize;

	if (allowExpandedCount) {
		do {
			u_int8_t b = MIN(charLength, 255);
			WriteUInt8(b);
			charLength -= b;
		} while (charLength);
	} else {
		if (charLength > 255) {
			throw new MP4Error(ERANGE, "MP4WriteCountedString");
		}
		WriteUInt8(charLength);
	}

	if (byteLength > 0) {
		WriteBytes((u_int8_t*)string, byteLength);
	}
}

u_int64_t MP4File::ReadBits(u_int8_t numBits)
{
	ASSERT(numBits > 0);
	ASSERT(numBits <= 64);

	u_int64_t bits = 0;

	for (u_int8_t i = numBits; i > 0; i--) {
		if (m_numReadBits == 0) {
			ReadBytes(&m_bufReadBits, 1);
			m_numReadBits = 8;
		}
		bits = (bits << 1) | ((m_bufReadBits >> (--m_numReadBits)) & 1);
	}

	return bits;
}

void MP4File::WriteBits(u_int64_t bits, u_int8_t numBits)
{
	ASSERT(numBits <= 64);

	for (u_int8_t i = numBits; i > 0; i--) {
		m_bufWriteBits |= ((bits >> (i - 1)) & 1) << (8 - m_numWriteBits++);
		if (m_numWriteBits == 8) {
			FlushWriteBits();
		}
	}
}

void MP4File::FlushWriteBits()
{
	if (m_numWriteBits > 0) {
		WriteBytes(&m_bufWriteBits, 1);
		m_numWriteBits = 0;
		m_bufWriteBits = 0;
	}
}

u_int32_t MP4File::ReadMpegLength()
{
	u_int32_t length = 0;
	u_int8_t numBytes = 0;
	u_int8_t b;

	do {
		b = ReadUInt8();
		length = (length << 7) | (b & 0x7F);
		numBytes++;
	} while ((b & 0x80) && numBytes < 4);

	return length;
}

void MP4File::WriteMpegLength(u_int32_t value, bool compact)
{
	if (value > 0x0FFFFFFF) {
		throw new MP4Error(ERANGE, "MP4WriteMpegLength");
	}

	int8_t numBytes;

	if (compact) {
		if (value <= 0x7F) {
			numBytes = 1;
		} else if (value <= 0x3FFF) {
			numBytes = 2;
		} else if (value <= 0x1FFFFF) {
			numBytes = 3;
		} else {
			numBytes = 4;
		}
	} else {
		numBytes = 4;
	}

	int8_t i = numBytes;
	do {
		i--;
		u_int8_t b = (value >> (i * 7)) & 0x7F;
		if (i > 0) {
			b |= 0x80;
		}
		WriteUInt8(b);
	} while (i > 0);
}

bool MP4File::FindProperty(char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	*pIndex = 0;	// set the default answer for index

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

	switch (pProperty->GetType()) {
	case Integer8Property:
		return ((MP4Integer8Property*)pProperty)->GetValue(index);
	case Integer16Property:
		return ((MP4Integer16Property*)pProperty)->GetValue(index);
	case Integer24Property:
		return ((MP4Integer24Property*)pProperty)->GetValue(index);
	case Integer32Property:
		return ((MP4Integer32Property*)pProperty)->GetValue(index);
	case Integer64Property:
		return ((MP4Integer64Property*)pProperty)->GetValue(index);
	}
	ASSERT(FALSE);
}

void MP4File::SetIntegerProperty(char* name, u_int64_t value)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindIntegerProperty(name, &pProperty, &index);

	switch (pProperty->GetType()) {
	case Integer8Property:
		((MP4Integer8Property*)pProperty)->SetValue(value, index);
		break;
	case Integer16Property:
		((MP4Integer16Property*)pProperty)->SetValue(value, index);
		break;
	case Integer24Property:
		((MP4Integer24Property*)pProperty)->SetValue(value, index);
		break;
	case Integer32Property:
		((MP4Integer32Property*)pProperty)->SetValue(value, index);
		break;
	case Integer64Property:
		((MP4Integer64Property*)pProperty)->SetValue(value, index);
		break;
	default:
		ASSERT(FALSE);
	}
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

MP4TrackId MP4File::AddTrack(char* type)
{
	MP4Atom* pTrakAtom = MP4Atom::CreateAtom("trak");
	ASSERT(pTrakAtom);

	MP4Atom* pMoovAtom = m_pRootAtom->FindAtom("moov");
	ASSERT(pMoovAtom);

	pTrakAtom->SetFile(this);
	pTrakAtom->SetParentAtom(pMoovAtom);
	pTrakAtom->Generate();

	pMoovAtom->AddChildAtom(pTrakAtom);

	MP4Track* pTrack = new MP4Track(this, pTrakAtom);
	m_pTracks.Add(pTrack);

	// TEMP really want to set type for trak atom
	pTrack->SetType(type);

	// allocate a new track id
	MP4TrackId trackId = GetIntegerProperty("moov.mvhd.nextTrackId");
	// TBD scan if nextTrackId is max'ed out
	SetIntegerProperty("moov.mvhd.nextTrackId", trackId + 1);

	return trackId;
}

MP4TrackId MP4File::AddAudioTrack(u_int32_t timeScale, 
	u_int32_t sampleDuration)
{
	return AddTrack("audio");
}

MP4TrackId MP4File::AddVideoTrack(u_int32_t timeScale, 
	u_int32_t sampleDuration, u_int16_t width, u_int16_t height)
{
	return AddTrack("video");
}

MP4TrackId MP4File::AddHintTrack(MP4TrackId refTrackId)
{
	return AddTrack("hint");
}

void MP4File::DeleteTrack(MP4TrackId trackId)
{
	u_int16_t trackIndex = FindTrackIndex(trackId);
	MP4Track* pTrack = m_pTracks[trackIndex];

	MP4Atom* pTrakAtom = pTrack->GetTrakAtom();
	ASSERT(pTrakAtom);

	MP4Atom* pMoovAtom = m_pRootAtom->FindAtom("moov");
	ASSERT(pMoovAtom);
	pMoovAtom->DeleteChildAtom(pTrakAtom);

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

MP4TrackId MP4File::FindTrackId(u_int16_t index, char* type)
{
	if (type == NULL) {
		return m_pTracks[index]->GetId();
	} else {
		u_int16_t typeSeen = 0;
		const char* normType = MP4Track::NormalizeTrackType(type);

		for (u_int16_t i = 0; i < m_pTracks.Size(); i++) {
			if (!strcmp(normType, m_pTracks[i]->GetType())) {
				if (index == typeSeen) {
					return m_pTracks[i]->GetId();
				}
				typeSeen++;
			}
		}
		throw MP4Error("Track index doesn't exist", "FindTrackId"); 
	}
}

u_int16_t MP4File::FindTrackIndex(MP4TrackId trackId)
{
	for (u_int16_t i = 0; i < m_pTracks.Size(); i++) {
		if (m_pTracks[i]->GetId() == trackId) {
			return i;
		}
	}
	
	throw MP4Error("Track id doesn't exist", "FindTrackIndex"); 
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
	return m_pTracks[FindTrackIndex(trackId)]->
		ReadSample(sampleId, ppBytes, pNumBytes, 
			pStartTime, pDuration, pRenderingOffset, pIsSyncSample);
}

void MP4File::WriteSample(MP4TrackId trackId,
		u_int8_t* pBytes, u_int32_t numBytes,
		MP4Duration duration, MP4Duration renderingOffset, bool isSyncSample)
{
	return m_pTracks[FindTrackIndex(trackId)]->
		WriteSample(pBytes, numBytes, duration, renderingOffset, isSyncSample);
}

char* MP4File::MakeTrackName(MP4TrackId trackId, char* name)
{
	u_int16_t trackIndex = FindTrackIndex(trackId);
	static char trackName[1024];
	snprintf(trackName, sizeof(trackName), 
		"moov.trak[%u].%s", trackIndex, name);
	return trackName;
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
	return GetIntegerProperty("moov.mvhd.duration");
}

u_int32_t MP4File::GetTimeScale()
{
	return GetIntegerProperty("moov.mvhd.timeScale");
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

MP4SampleId MP4File::GetNumberofTrackSamples(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, "mdia.minf.stbl.stsz.sampleCount");
}

u_int32_t MP4File::GetTrackTimeScale(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, "mdia.mdhd.timeScale");
}

void MP4File::SetTrackTimeScale(MP4TrackId trackId, u_int32_t value)
{
	SetTrackIntegerProperty(trackId, "mdia.mdhd.timeScale", value);
}

MP4Duration MP4File::GetTrackDuration(MP4TrackId trackId)
{
	return GetTrackIntegerProperty(trackId, "mdia.mdhd.duration");
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

