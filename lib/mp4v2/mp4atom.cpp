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
#ifdef NOTDEF
#include <ctype.h>
#endif

MP4Atom* MP4Atom::ReadAtom(MP4File* pFile, MP4Atom* pParentAtom)
{
	u_int8_t hdrSize = 8;
	u_int8_t extendedType[16];

	u_int64_t pos = pFile->GetPosition();

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: starting at pos %llu\n", pos));

	u_int64_t dataSize = pFile->ReadUInt32();

	if (dataSize == 1) {
		dataSize = pFile->ReadUInt64(); 
		hdrSize += 8;
	}

	char type[5];
	pFile->ReadBytes((u_int8_t*)&type[0], 4);
	type[4] = '\0';

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: type = %s\n", type));

	// extended type
	if (ATOMID(type) == ATOMID("uuid")) {
		pFile->ReadBytes(extendedType, sizeof(extendedType));
		hdrSize += sizeof(extendedType);
	}

	if (dataSize == 0) {
		// extends to EOF
		dataSize = pFile->GetSize() - (pos + hdrSize);
	}

	dataSize -= hdrSize;

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: data size = %u\n", dataSize));

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
		pAtom->AddProperty(
			new MP4BytesProperty("data", dataSize));

		if (!IsReasonableType(pAtom->GetType())) {
			VERBOSE_READ(pFile->GetVerbosity(),
				printf("Warning: atom type %s is suspect\n", pAtom->GetType()));
		} else {
			VERBOSE_READ(pFile->GetVerbosity(),
				printf("Info: atom type %s is unknown\n", pAtom->GetType()));
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
	m_pFile->SetPosition(m_end);
}

MP4Property* MP4Atom::FindProperty(char *name)
{
	// check that our atom name is specified as the first component
	if (strcmp(m_type, "")) {
		char* atomName = strsep(&name, ".");
		if (strcasecmp(m_type, atomName)) {
			return NULL;
		}
	}

	char *propName = name;

	u_int32_t numProperties = m_pProperties.Size();

	// check all of our properties
	for (u_int32_t i = 0; i < numProperties; i++) {
		MP4Property* pProperty = m_pProperties[i]->FindProperty(propName); 

		if (pProperty) {
			return pProperty;
		}
	}

	// check all of our child atoms
	u_int32_t numChildren = m_pChildAtoms.Size();

	for (u_int32_t i = 0; i < numChildren; i++) {
		MP4Property* pProperty = 
			m_pChildAtoms[i]->FindProperty(propName);

		if (pProperty) {
			return pProperty;
		}
	}

	return NULL;
}

void MP4Atom::ReadProperties(u_int32_t startIndex)
{
	u_int32_t i;
	u_int32_t numProperties = m_pProperties.Size();

	// read any properties of the atom
	for (i = startIndex; i < numProperties; i++) {
		m_pProperties[i]->Read(m_pFile);

		if (m_pFile->GetPosition() > m_end) {
			VERBOSE_READ(GetVerbosity(), 
				printf("ReadProperties: insufficient data for property %s\n",
					 m_pProperties[i]->GetName())); 

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
		printf("ReadChildAtoms: of %s\n", m_type));

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
	u_int32_t i;
	u_int32_t size;

	ASSERT(m_pFile);

	BeginWrite();

	size = m_pProperties.Size();
	for (i = 0; i < size; i++) {
		m_pProperties[i]->Write(m_pFile);
	}

	size = m_pChildAtoms.Size();
	for (i = 0; i < size; i++) {
		m_pChildAtoms[i]->Write();
	}

	EndWrite();
}

void MP4Atom::BeginWrite()
{
	m_start = m_pFile->GetPosition();
	m_pFile->WriteUInt32(0);
	m_pFile->WriteBytes((u_int8_t*)&m_type[0], 4);
	if (ATOMID(m_type) == ATOMID("uuid")) {
		m_pFile->WriteBytes(m_extendedType, sizeof(m_extendedType));
	}
}

void MP4Atom::EndWrite()
{
	m_end = m_pFile->GetPosition();
	m_size = (m_end - m_start);
	m_pFile->SetPosition(m_start);
	ASSERT(m_size <= (u_int64_t)0xFFFFFFFF);
	m_pFile->WriteUInt32(m_size);
	m_pFile->SetPosition(m_end);

	// adjust size to just reflect data portion of atom
	m_size -= 8;
	if (ATOMID(m_type) == ATOMID("uuid")) {
		m_size -= sizeof(m_extendedType);
	}
}

void MP4Atom::Dump(FILE* pFile)
{
	u_int32_t depth = GetDepth();

	if (depth > 0) {
		Indent(pFile, depth); fprintf(pFile, "type %s\n", m_type);
		Indent(pFile, depth); fprintf(pFile, "size %llu\n", m_size);
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

		Indent(pFile, depth);
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
	MP4Atom *pAtom = this;
	u_int8_t depth = 0;

	while ((pAtom = pAtom->GetParentAtom()) != NULL) {
		depth++;
		ASSERT(depth < 100);
	}
	return depth;
}

void inline MP4Atom::Indent(FILE* pFile, u_int8_t depth)
{
	fprintf(pFile, "%*c", depth, ' ');
}
