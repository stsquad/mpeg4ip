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

#include "mp4.h"

MP4PropertyType MP4IntegerProperty<u_int8_t>::GetType()
{
	return Integer8Property;
}

void MP4IntegerProperty<u_int8_t>::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	m_values[index] = pFile->ReadUInt8();
}

void MP4IntegerProperty<u_int8_t>::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteUInt8(m_values[index]);
}

void MP4IntegerProperty<u_int8_t>::Dump(FILE* pFile, u_int32_t index)
{
	fprintf(pFile, "%s = %u (0x%02x)\n", m_name, m_values[index], m_values[index]);
}

MP4PropertyType MP4IntegerProperty<u_int16_t>::GetType()
{
	return Integer16Property;
}

void MP4IntegerProperty<u_int16_t>::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	m_values[index] = pFile->ReadUInt16();
}

void MP4IntegerProperty<u_int16_t>::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteUInt16(m_values[index]);
}

void MP4IntegerProperty<u_int16_t>::Dump(FILE* pFile, u_int32_t index)
{
	fprintf(pFile, "%u (0x%04x) ", m_values[index], m_values[index]);
}

MP4PropertyType MP4IntegerProperty<u_int32_t>::GetType()
{
	return Integer32Property;
}

void MP4IntegerProperty<u_int32_t>::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	m_values[index] = pFile->ReadUInt32();
}

void MP4IntegerProperty<u_int32_t>::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteUInt32(m_values[index]);
}

void MP4IntegerProperty<u_int32_t>::Dump(FILE* pFile, u_int32_t index)
{
	fprintf(pFile, "%u (0x%08x) ", m_values[index], m_values[index]);
}

MP4PropertyType MP4IntegerProperty<u_int64_t>::GetType()
{
	return Integer64Property;
}

void MP4IntegerProperty<u_int64_t>::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	m_values[index] = pFile->ReadUInt64();
}

void MP4IntegerProperty<u_int64_t>::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteUInt64(m_values[index]);
}

void MP4IntegerProperty<u_int64_t>::Dump(FILE* pFile, u_int32_t index)
{
	fprintf(pFile, "%llu (0x%016x) ", m_values[index], m_values[index]);
}

void MP4BitfieldProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	m_values[index] = pFile->ReadBits(m_numBits);
}

void MP4BitfieldProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteBits(m_values[index], m_numBits);
}

void MP4StringProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	if (m_usePascalFormat) {
		m_values[index] = pFile->ReadPascalString();
	} else {
		m_values[index] = pFile->ReadCString();
	}
}

void MP4StringProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	if (m_usePascalFormat) {
		pFile->WritePascalString(m_values[index]);
	} else {
		pFile->WriteCString(m_values[index]);
	}
}

void MP4StringProperty::Dump(FILE* pFile, u_int32_t index)
{
	fprintf(pFile, "%s = %s\n", m_name, m_values[index]);
}

void MP4BytesProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	MP4Free(m_values[index]);
	WARNING(m_valueSizes[index] == 0);
	m_values[index] = (u_int8_t*)MP4Malloc(m_valueSizes[index]);
	pFile->ReadBytes(m_values[index], m_valueSizes[index]);
}

void MP4BytesProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	WARNING(m_values[index] == NULL);
	WARNING(m_valueSizes[index] == 0);
	pFile->WriteBytes(m_values[index], m_valueSizes[index]);
}

void MP4BytesProperty::Dump(FILE* pFile, u_int32_t index)
{
	fprintf(pFile, "%s = <%u bytes>", m_name, m_valueSizes[index]);
	for (u_int32_t i = 0; i < m_valueSizes[index]; i++) {
		if ((i % 16) == 0) {
			fprintf(pFile, "\n");
		}
		fprintf(pFile, "%02x ", m_values[index][i]);
	}
	fprintf(pFile, "\n");
}

MP4Property* MP4TableProperty::FindProperty(char *name)
{
	// separate table name from the index
	char* dontcare = name;
	char* tableName = strsep(&dontcare, "[");

	// check the table name 
	if (strcasecmp(m_name, tableName)) {
		return NULL;
	}

	// get name of table property
	char *tablePropName = name;
	dontcare = strsep(&tablePropName, ".");

	return FindContainedProperty(tablePropName);
}

MP4Property* MP4TableProperty::FindContainedProperty(char *name)
{
	u_int32_t numProperties = m_pProperties.size();

	for (u_int32_t i = 0; i < numProperties; i++) {
		MP4Property* pProperty = m_pProperties[i]->FindProperty(name); 

		if (pProperty) {
			return pProperty;
		}
	}
	return NULL;
}

void MP4TableProperty::Read(MP4File* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit) {
		return;
	}

	u_int32_t numProperties = m_pProperties.size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	u_int32_t numEntries = m_pCountProperty->GetValue();

	/* for each property set size */
	for (u_int32_t j = 0; j < numProperties; j++) {
		m_pProperties[j]->SetCount(numEntries);
	}

	for (u_int32_t i = 0; i < numEntries; i++) {
		for (u_int32_t j = 0; j < numProperties; j++) {
			m_pProperties[j]->Read(pFile, i);
		}
	}
}

void MP4TableProperty::Write(MP4File* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit) {
		return;
	}

	u_int32_t numProperties = m_pProperties.size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	u_int32_t numEntries = m_pCountProperty->GetValue();

	ASSERT(m_pProperties[0]->GetCount() == numEntries);

	for (u_int32_t i = 0; i < numEntries; i++) {
		for (u_int32_t j = 0; j < numProperties; j++) {
			m_pProperties[j]->Write(pFile, i);
		}
	}
}

void MP4TableProperty::Dump(FILE* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	u_int32_t numProperties = m_pProperties.size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	u_int32_t numEntries = m_pCountProperty->GetValue();

	ASSERT(m_pProperties[0]->GetCount() == numEntries);

	for (u_int32_t j = 0; j < numProperties; j++) {
		fprintf(pFile, "%s ", m_pProperties[j]->GetName());
	}
	fprintf(pFile, "\n");

	for (u_int32_t i = 0; i < numEntries; i++) {
		for (u_int32_t j = 0; j < numProperties; j++) {
			m_pProperties[j]->Dump(pFile, i);
		}
		fprintf(pFile, "\n");
	}
}

MP4Property* MP4DescriptorProperty::FindProperty(char *name)
{
	/* if we are named, then check it */
	if (strcmp(m_name, "")) {
		// TBD must handle index
		char* firstName = strsep(&name, ".");
		if (strcasecmp(m_name, firstName)) {
			return NULL;
		}
		/* ok, continue on to check rest of name */
	} /* else we're unnamed so keep going */

	return FindContainedProperty(name);
}

MP4Property* MP4DescriptorProperty::FindContainedProperty(char *name)
{
	u_int32_t numProperties = m_pProperties.size();

	for (u_int32_t i = 0; i < numProperties; i++) {
		MP4Property* pProperty = m_pProperties[i]->FindProperty(name); 

		if (pProperty) {
			return pProperty;
		}
	}
	return NULL;
}

bool PeekTag(MP4File* pFile, u_int8_t desiredTag)
{
	u_int8_t tag;
	pFile->PeekBytes(&tag, 1);
	return (tag == desiredTag);
}

void MP4DescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		ReadProperties(pFile, GetCount() - 1);
	}
}

void MP4DescriptorProperty::ReadHeader(MP4File* pFile)
{
	// read tag and length
	u_int8_t tag = pFile->ReadUInt8();
	ASSERT(tag == m_tag);
	m_length = pFile->ReadMpegLength();
	m_start = pFile->GetPosition();
}

void MP4DescriptorProperty::ReadProperties(MP4File* pFile, u_int32_t index = 0, u_int32_t propStartIndex = 0, u_int32_t propCount = 0xFFFFFFFF)
{
	u_int32_t numProperties = MIN(propCount, 
		m_pProperties.size() - propStartIndex);

	for (u_int32_t i = propStartIndex; i < numProperties; i++) {
		m_pProperties[i]->Read(pFile, index);

		// check file position versus length of descriptor
		u_int64_t endPos = pFile->GetPosition();
		if (endPos - m_start > m_length) {
			VERBOSE_ERROR(pFile->GetVerbosity(),
				printf("Overran descriptor, tag %u length %u start %llu property %u position %llu\n",
					m_tag, m_length, i, m_start, endPos));
			throw new MP4Error(ERANGE, "MP4DescriptorProperty::Read");
		}
	}
}

void MP4DescriptorProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	u_int32_t count = GetCount();

	u_int32_t numProperties = m_pProperties.size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	// write tag and length placeholder
	pFile->WriteUInt8(m_tag);
	u_int64_t lengthPos = pFile->GetPosition();
	pFile->WriteMpegLength(0);
	u_int64_t startPos = pFile->GetPosition();

	for (u_int32_t i = 0; i < count; i++) {
		for (u_int32_t j = 0; j < numProperties; j++) {
			m_pProperties[j]->Write(pFile, i);
		}
	}

	// go back and write correct length
	u_int64_t endPos = pFile->GetPosition();
	pFile->SetPosition(lengthPos);
	pFile->WriteMpegLength(endPos - startPos);
	pFile->SetPosition(endPos);
}

void MP4DescriptorProperty::Dump(FILE* pFile, u_int32_t index)
{
	u_int32_t count = GetCount();

	u_int32_t numProperties = m_pProperties.size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	for (u_int32_t i = 0; i < count; i++) {
		for (u_int32_t j = 0; j < numProperties; j++) {
			m_pProperties[i]->Dump(pFile, i);
		}
	}
}

