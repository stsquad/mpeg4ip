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


// generate a skeletal self

void MP4Atom::Generate()
{

	// for all properties
	for (u_int32_t i = 0; i < m_pProperties.Size(); i++) {
		// ask it to self generate
		m_pProperties[i]->Generate();
	}

	// for all mandatory, single child atom types
	for (u_int32_t i = 0; i < m_pChildAtomInfos.Size(); i++) {
		if (m_pChildAtomInfos[i]->m_mandatory
		  && m_pChildAtomInfos[i]->m_onlyOne) {

			// create the mandatory, single child atom
			MP4Atom* pChildAtom = 
				CreateAtom(m_pChildAtomInfos[i]->m_name);
			ASSERT(pChildAtom);
			pChildAtom->SetFile(m_pFile);
			pChildAtom->SetParentAtom(this);
			AddChildAtom(pChildAtom);

			// and ask it to self generate
			pChildAtom->Generate();
		}
	}
}

MP4Atom* MP4Atom::ReadAtom(MP4File* pFile, MP4Atom* pParentAtom)
{
	u_int8_t hdrSize = 8;
	u_int8_t extendedType[16];

	u_int64_t pos = pFile->GetPosition();

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: pos = 0x%llx\n", pos));

	u_int64_t dataSize = pFile->ReadUInt32();

	if (dataSize == 1) {
		dataSize = pFile->ReadUInt64(); 
		hdrSize += 8;
	}

	char type[5];
	pFile->ReadBytes((u_int8_t*)&type[0], 4);
	type[4] = '\0';

	// extended type
	if (ATOMID(type) == ATOMID("uuid")) {
		pFile->ReadBytes(extendedType, sizeof(extendedType));
		hdrSize += sizeof(extendedType);
	}

	if (dataSize == 0) {
		// extends to EOF
		dataSize = pFile->GetSize() - pos;
	}

	dataSize -= hdrSize;

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: type = %s data-size = %llu (0x%llx)\n", 
			type, dataSize, dataSize));

	if (pos + hdrSize + dataSize > pParentAtom->GetEnd()) {
		VERBOSE_READ(pFile->GetVerbosity(), 
			printf("ReadAtom: invalid atom size, extends outside parent atom\n"));
		throw new MP4Error("invalid atom size", "ReadAtom");
	}


	MP4Atom* pAtom = CreateAtom(type);
	ASSERT(pAtom);

	pAtom->SetFile(pFile);
	pAtom->SetStart(pos);
	pAtom->SetEnd(pos + hdrSize + dataSize);
	pAtom->SetSize(dataSize);
	if (ATOMID(type) == ATOMID("uuid")) {
		pAtom->SetExtendedType(extendedType);
	}
	if (pAtom->IsUnknownType()) {
		if (!IsReasonableType(pAtom->GetType())) {
			VERBOSE_READ(pFile->GetVerbosity(),
				printf("Warning: atom type %s is suspect\n", pAtom->GetType()));
		} else {
			VERBOSE_READ(pFile->GetVerbosity(),
				printf("Info: atom type %s is unknown\n", pAtom->GetType()));
		}

		if (dataSize > 0) {
			pAtom->AddProperty(
				new MP4BytesProperty("data", dataSize));
		}
	}

	pAtom->SetParentAtom(pParentAtom);

	pAtom->Read();

	return pAtom;
}

bool MP4Atom::IsReasonableType(const char* type)
{
	for (u_int8_t i = 0; i < 4; i++) {
		if (isalnum(type[i])) {
			continue;
		}
		if (i == 3 && type[i] == ' ') {
			continue;
		}
		return false;
	}
	return true;
}

// generic read
void MP4Atom::Read()
{
	ASSERT(m_pFile);

	if (ATOMID(m_type) != 0 && m_size > 1000000) {
		VERBOSE_READ(GetVerbosity(), 
			printf("Warning: %s atom size %llu is suspect\n",
				m_type, m_size));
	}

	ReadProperties();

	// read child atoms, if we expect there to be some
	if (m_pChildAtomInfos.Size() > 0) {
		ReadChildAtoms();
	}

	Skip();	// to end of atom
}

void MP4Atom::Skip() {
	if (m_pFile->GetPosition() != m_end) {
		VERBOSE_READ(m_pFile->GetVerbosity(),
			printf("Skip: %d bytes\n", m_end - m_pFile->GetPosition()));
	}
	m_pFile->SetPosition(m_end);
}

MP4Atom* MP4Atom::FindAtom(char* name)
{
	if (!IsMe(name)) {
		return NULL;
	}

	if (!IsRootAtom()) {
		VERBOSE_FIND(m_pFile->GetVerbosity(),
			printf("FindAtom: matched %s\n", name));

		name = MP4NameAfterFirst(name);

		// I'm the sought after atom 
		if (name == NULL) {
			return this;
		}
	}

	// else it's one of my children
	return FindChildAtom(name);
}

bool MP4Atom::FindProperty(char *name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!IsMe(name)) {
		return false;
	}

	if (!IsRootAtom()) {
		VERBOSE_FIND(m_pFile->GetVerbosity(),
			printf("FindProperty: matched %s\n", name));

		name = MP4NameAfterFirst(name);

		// no property name given
		if (name == NULL) {
			return false;
		}
	}

	return FindContainedProperty(name, ppProperty, pIndex);
}

bool MP4Atom::IsMe(char* name)
{
	if (name == NULL) {
		return false;
	}

	// root atom always matches
	if (!strcmp(m_type, "")) {
		return true;
	}

	// check if our atom name is specified as the first component
	if (!MP4NameFirstMatches(m_type, name)) {
		return false;
	}

	return true;
}

MP4Atom* MP4Atom::FindChildAtom(char* name)
{
	u_int32_t atomIndex = 0;

	// get the index if we have one, e.g. moov.trak[2].mdia...
	MP4NameFirstIndex(name, &atomIndex);

	// need to get to the index'th child atom of the right type
	for (u_int32_t i = 0; i < m_pChildAtoms.Size(); i++) {
		if (MP4NameFirstMatches(m_pChildAtoms[i]->GetType(), name)) {
			if (atomIndex == 0) {
				// this is the one, ask it to match
				return m_pChildAtoms[i]->FindAtom(name);
			}
			atomIndex--;
		}
	}

	return NULL;
}

bool MP4Atom::FindContainedProperty(char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	u_int32_t numProperties = m_pProperties.Size();

	// check all of our properties
	for (u_int32_t i = 0; i < numProperties; i++) {
		if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) {
			return true;
		}
	}

	// not one of our properties, 
	// presumably one of our children's properties
	// check child atoms...

	// check if we have an index, e.g. trak[2].mdia...
	u_int32_t atomIndex = 0;
	bool haveAtomIndex = MP4NameFirstIndex(name, &atomIndex);

	// need to get to the index'th child atom of the right type
	for (u_int32_t i = 0; i < m_pChildAtoms.Size(); i++) {
		if (MP4NameFirstMatches(m_pChildAtoms[i]->GetType(), name)) {
			if (atomIndex == 0) {
				// this is the one, ask it to match
				return m_pChildAtoms[i]->FindProperty(name, ppProperty, pIndex);
			}
			atomIndex--;
		}
	}

	return false;
}

void MP4Atom::ReadProperties(u_int32_t startIndex, u_int32_t count)
{
	u_int32_t numProperties = MIN(count, m_pProperties.Size() - startIndex);

	// read any properties of the atom
	for (u_int32_t i = startIndex; i < startIndex + numProperties; i++) {

		m_pProperties[i]->Read(m_pFile);

		if (m_pFile->GetPosition() > m_end) {
			VERBOSE_READ(GetVerbosity(), 
				printf("ReadProperties: insufficient data for property: %s pos 0x%llx atom end 0x%llx\n",
					m_pProperties[i]->GetName(), 
					m_pFile->GetPosition(), m_end)); 

			throw new MP4Error("atom is too small", "Atom ReadProperties");
		}

		if (m_pProperties[i]->GetType() == TableProperty) {
			VERBOSE_READ_TABLE(GetVerbosity(), 
				printf("Read: "); m_pProperties[i]->Dump(stdout));
		} else {
			VERBOSE_READ(GetVerbosity(), 
				printf("Read: "); m_pProperties[i]->Dump(stdout));
		}
	}
}

void MP4Atom::ReadChildAtoms()
{
	VERBOSE_READ(GetVerbosity(), 
		printf("ReadChildAtoms: of %s\n", m_type[0] ? m_type : "root"));

	// read any child atoms
	while (m_pFile->GetPosition() < m_end) {
		MP4Atom* pChildAtom = MP4Atom::ReadAtom(m_pFile, this);

		AddChildAtom(pChildAtom);

		MP4AtomInfo* pChildAtomInfo = FindAtomInfo(pChildAtom->GetType());

		// if child atom is of known type
		// but not expected here print warning
		if (pChildAtomInfo == NULL && !pChildAtom->IsUnknownType()) {
			VERBOSE_READ(GetVerbosity(),
				printf("Warning: In atom %s unexpected child atom %s\n",
					GetType(), pChildAtom->GetType()));
		}

		// if child atoms should have just one instance
		// and this is more than one, print warning
		if (pChildAtomInfo) {
			pChildAtomInfo->m_count++;

			if (pChildAtomInfo->m_onlyOne && pChildAtomInfo->m_count > 1) {
				VERBOSE_READ(GetVerbosity(),
					printf("Warning: In atom %s multiple child atoms %s\n",
						GetType(), pChildAtom->GetType()));
			}
		}
	}

	// if mandatory child atom doesn't exist, print warning
	u_int32_t numAtomInfo = m_pChildAtomInfos.Size();
	for (u_int32_t i = 0; i < numAtomInfo; i++) {
		if (m_pChildAtomInfos[i]->m_mandatory
		  && m_pChildAtomInfos[i]->m_count == 0) {
				VERBOSE_READ(GetVerbosity(),
					printf("Warning: In atom %s missing child atom %s\n",
						GetType(), m_pChildAtomInfos[i]->m_name));
		}
	}

	VERBOSE_READ(GetVerbosity(), 
		printf("ReadChildAtoms: finished %s\n", m_type));
}

MP4AtomInfo* MP4Atom::FindAtomInfo(const char* name)
{
	u_int32_t numAtomInfo = m_pChildAtomInfos.Size();
	for (u_int32_t i = 0; i < numAtomInfo; i++) {
		if (ATOMID(m_pChildAtomInfos[i]->m_name) == ATOMID(name)) {
			return m_pChildAtomInfos[i];
		}
	}
	return NULL;
}

// generic write
void MP4Atom::Write()
{
	// TBD use of 64 bit write
	bool use64 = false;

	ASSERT(m_pFile);

	BeginWrite(use64);

	WriteProperties();

	WriteChildAtoms();

	EndWrite(use64);
}

void MP4Atom::BeginWrite(bool use64)
{
	m_start = m_pFile->GetPosition();
	if (use64) {
		m_pFile->WriteUInt32(1);
		m_pFile->WriteUInt64(0);
	} else {
		m_pFile->WriteUInt32(0);
	}
	m_pFile->WriteBytes((u_int8_t*)&m_type[0], 4);
	if (ATOMID(m_type) == ATOMID("uuid")) {
		m_pFile->WriteBytes(m_extendedType, sizeof(m_extendedType));
	}
}

void MP4Atom::EndWrite(bool use64)
{
	m_end = m_pFile->GetPosition();
	m_size = (m_end - m_start);
	if (use64) {
		m_pFile->SetPosition(m_start + 4);
		m_pFile->WriteUInt64(m_size);
	} else {
		ASSERT(m_size <= (u_int64_t)0xFFFFFFFF);
		m_pFile->SetPosition(m_start);
		m_pFile->WriteUInt32(m_size);
	}
	m_pFile->SetPosition(m_end);

	// adjust size to just reflect data portion of atom
	m_size -= (use64 ? 16 : 8);
	if (ATOMID(m_type) == ATOMID("uuid")) {
		m_size -= sizeof(m_extendedType);
	}
}

void MP4Atom::WriteProperties(u_int32_t startIndex, u_int32_t count)
{
	u_int32_t numProperties = MIN(count, m_pProperties.Size() - startIndex);

	for (u_int32_t i = startIndex; i < startIndex + numProperties; i++) {
		m_pProperties[i]->Write(m_pFile);
	}
}

void MP4Atom::WriteChildAtoms()
{
	u_int32_t size = m_pChildAtoms.Size();
	for (u_int32_t i = 0; i < size; i++) {
		m_pChildAtoms[i]->Write();
	}
}

u_int8_t MP4Atom::GetVersion()
{
	if (strcmp("version", m_pProperties[0]->GetName())) {
		return 0;
	}
	return ((MP4Integer8Property*)m_pProperties[0])->GetValue();
}

void MP4Atom::SetVersion(u_int8_t version) 
{
	if (strcmp("version", m_pProperties[0]->GetName())) {
		return;
	}
	return ((MP4Integer8Property*)m_pProperties[0])->SetValue(version);
}

u_int32_t MP4Atom::GetFlags()
{
	if (strcmp("flags", m_pProperties[1]->GetName())) {
		return 0;
	}
	return ((MP4Integer24Property*)m_pProperties[1])->GetValue();
}

void MP4Atom::SetFlags(u_int32_t flags) 
{
	if (strcmp("flags", m_pProperties[1]->GetName())) {
		return;
	}
	return ((MP4Integer24Property*)m_pProperties[1])->SetValue(flags);
}

void MP4Atom::Dump(FILE* pFile)
{
	u_int32_t depth = GetDepth();

	if (depth > 0) {
		Indent(pFile, depth); fprintf(pFile, "type %s\n", m_type);
	}

	u_int32_t i;
	u_int32_t size;

	// dump our properties
	size = m_pProperties.Size();
	for (i = 0; i < size; i++) {

		/* skip details of tables unless we're told to be verbose */
		if (m_pProperties[i]->GetType() == TableProperty
		  && !(GetVerbosity() & MP4_DETAILS_READ_TABLE)) {
			continue;
		}

		m_pProperties[i]->Dump(pFile);
	}

	// dump our children
	size = m_pChildAtoms.Size();
	for (int i = 0; i < size; i++) {
		m_pChildAtoms[i]->Dump(pFile);
	}
}

u_int32_t MP4Atom::GetVerbosity() 
{
	ASSERT(m_pFile);
	return m_pFile->GetVerbosity();
}

u_int8_t MP4Atom::GetDepth()
{
	// TBD can we cache the depth once we've been created???

	MP4Atom *pAtom = this;
	u_int8_t depth = 0;

	while ((pAtom = pAtom->GetParentAtom()) != NULL) {
		depth++;
		ASSERT(depth < 100);
	}
	return depth;
}

