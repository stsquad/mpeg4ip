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
#include "descriptors.h"

MP4IODescriptorProperty::MP4IODescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4IODescrTag, name)
{
	/* N.B. other member functions depend on the property indicies */

	AddProperty( /* 0 */
		new MP4BitfieldProperty("objectDescriptorId", 10));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("URLFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("includeInlineProfileLevelFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("reserved", 4));
	AddProperty( /* 4 */
		new MP4StringProperty("URL", true));
	AddProperty( /* 5 */
		new MP4Integer8Property("ODProfileLevelId"));
	AddProperty( /* 6 */
		new MP4Integer8Property("sceneProfileLevelId"));
	AddProperty( /* 7 */
		new MP4Integer8Property("audioProfileLevelId"));
	AddProperty( /* 8 */
		new MP4Integer8Property("visualProfileLevelId"));
	AddProperty( /* 9 */
		new MP4Integer8Property("graphicsProfileLevelId"));
	AddProperty( /* 10 */ 
		new MP4ESDescriptorProperty("esDescr"));
	// TBD AddProperty( /* 11 */ 
		// TBD new MP4OCIDescriptorProperty("ociDescr"));
	AddProperty( /* 12 */
		new MP4IPMPPtrDescriptorProperty("ipmpDescrPtr"));
	AddProperty( /* 13 */
		new MP4ExtensionDescriptorProperty("extDescr"));
}

void MP4IODescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* read the first two properties */
		ReadProperties(pFile, GetCount() - 1, 0, 2);

		/* which allows us to reconfigure ourselves */
		Mutate();

		/* and read the remaining properties */
		ReadProperties(pFile, GetCount() - 1, 2);
	}
}

void MP4IODescriptorProperty::Write(MP4File* pFile, u_int32_t index)
{
	Mutate();
	MP4DescriptorProperty::Write(pFile, index);
}

void MP4IODescriptorProperty::Mutate()
{
	bool urlFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	m_pProperties[4]->SetImplicit(!urlFlag);
	for (u_int32_t i = 5; i <= 9; i++) {
		m_pProperties[i]->SetImplicit(urlFlag);
	}
}

MP4ESDescriptorProperty::MP4ESDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4ESDescrTag, name)
{
	/* N.B. other class functions depend on the property indicies */

	AddProperty( /* 0 */
		new MP4Integer16Property("ESID"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("streamDependenceFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("URLFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("OCRstreamFlag", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("streamPriority", 5));
	AddProperty( /* 5 */
		new MP4Integer16Property("dependsOnESID"));
	AddProperty( /* 6 */
		new MP4StringProperty("URL", true));
	AddProperty( /* 7 */
		new MP4Integer16Property("OCRESID"));
	AddProperty( /* 8 */
		new MP4DecConfigDescriptorProperty("decConfigDescr"));
	AddProperty( /* 9 */
		new MP4SLConfigDescriptorProperty("slConfigDescr"));
	AddProperty( /* 10 */
		new MP4IPIPtrDescriptorProperty("ipiPtr"));
#ifdef TBD
	AddProperty( /* 11 */
		new MP4IPIdDataSetDescriptorProperty("ipIds"));
#endif
	AddProperty( /* 12 */
		new MP4IPMPPtrDescriptorProperty("ipmpDescrPtr"));
#ifdef TBD
	AddProperty( /* 13 */
		new MP4LanguageDescriptorProperty("langDescr"));
#endif
	AddProperty( /* 14 */
		new MP4QosDescriptorProperty("qosDescr"));
	AddProperty( /* 15 */
		new MP4RegistrationDescriptorProperty("regDescr"));
	AddProperty( /* 16 */
		new MP4ExtensionDescriptorProperty("extDescr"));
}

void MP4ESDescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* read the first five properties */
		ReadProperties(pFile, GetCount() - 1, 0, 5);

		/* which allows us to reconfigure ourselves */
		Mutate();

		/* and read the remaining properties */
		ReadProperties(pFile, GetCount() - 1, 5);
	}
}

void MP4ESDescriptorProperty::Write(MP4File* pFile, u_int32_t index)
{
	Mutate();
	MP4DescriptorProperty::Write(pFile, index);
}

void MP4ESDescriptorProperty::Mutate()
{
	bool streamDependFlag = 
		((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	m_pProperties[5]->SetImplicit(!streamDependFlag);

	bool urlFlag = 
		((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
	m_pProperties[6]->SetImplicit(!urlFlag);

	bool ocrFlag = 
		((MP4BitfieldProperty*)m_pProperties[3])->GetValue();
	m_pProperties[7]->SetImplicit(!ocrFlag);
}

MP4DecConfigDescriptorProperty::MP4DecConfigDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4DecConfigDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("objectTypeId"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("streamType", 6));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("upStream", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("reserved", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("bufferSizeDB", 24));
	AddProperty( /* 5 */
		new MP4Integer32Property("maxBitrate"));
	AddProperty( /* 6 */
		new MP4Integer32Property("avgBitrate"));
	AddProperty( /* 7 */
		new MP4DecSpecificDescriptorProperty("decSpecificInfo"));
	AddProperty( /* 8 */
		new MP4ExtProfileLevelDescriptorProperty("profileLevelIndicationIndexDescr"));
}

MP4DecSpecificDescriptorProperty::MP4DecSpecificDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4DecSpecificDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4BytesProperty("info"));
}

void MP4DecSpecificDescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* byte properties need to know how long they are before reading */
		((MP4BytesProperty*)m_pProperties[0])->SetValueSize(
			m_length, GetCount() - 1);

		ReadProperties(pFile, GetCount() - 1);
	}
}

MP4SLConfigDescriptorProperty::MP4SLConfigDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4SLConfigDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4BitfieldProperty("useAccessUnitStartFlag", 1));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("useAccessUnitEndFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("useRandomAccessPointFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("hasRandomAccessUnitsOnlyFlag", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("usePaddingFlag", 1));
	AddProperty( /* 5 */
		new MP4BitfieldProperty("useTimeStampsFlag", 1));
	AddProperty( /* 6 */
		new MP4BitfieldProperty("useIdleFlag", 1));
	AddProperty( /* 7 */
		new MP4BitfieldProperty("durationFlag", 1));
	AddProperty( /* 8 */
		new MP4Integer32Property("timeStampResolution"));
	AddProperty( /* 9 */
		new MP4Integer32Property("OCRResolution"));
	AddProperty( /* 10 */
		new MP4Integer8Property("timeStampLength"));
	AddProperty( /* 11 */
		new MP4Integer8Property("OCRLength"));
	AddProperty( /* 12 */
		new MP4Integer8Property("AULength"));
	AddProperty( /* 13 */
		new MP4Integer8Property("instantBitrateLength"));
	AddProperty( /* 14 */
		new MP4BitfieldProperty("degradationPriortyLength", 4));
	AddProperty( /* 15 */
		new MP4BitfieldProperty("AUSeqNumLength", 5));
	AddProperty( /* 16 */
		new MP4BitfieldProperty("packetSeqNumLength", 5));
	AddProperty( /* 17 */
		new MP4BitfieldProperty("reserved", 2));

	// if durationFlag 
	AddProperty( /* 18 */
		new MP4Integer32Property("timeScale"));
	AddProperty( /* 19 */
		new MP4Integer16Property("accessUnitDuration"));
	AddProperty( /* 20 */
		new MP4Integer16Property("compositionUnitDuration"));
	
	// if !useTimeStampsFlag
	AddProperty( /* 21 */
		new MP4BitfieldProperty("startDecodingTimeStamp", 64));
	AddProperty( /* 22 */
		new MP4BitfieldProperty("startCompositionTimeStamp", 64));
}

void MP4SLConfigDescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* read the first 18 properties */
		ReadProperties(pFile, GetCount() - 1, 0, 18);

		/* which allows us to reconfigure ourselves */
		Mutate();

		/* and read the remaining properties */
		ReadProperties(pFile, GetCount() - 1, 18);
	}
}

void MP4SLConfigDescriptorProperty::Write(MP4File* pFile, u_int32_t index)
{
	Mutate();
	MP4DescriptorProperty::Write(pFile, index);
}

void MP4SLConfigDescriptorProperty::Mutate()
{
	bool durationFlag = 
		((MP4BitfieldProperty*)m_pProperties[7])->GetValue();

	for (u_int32_t i = 18; i <= 20; i++) {
		m_pProperties[i]->SetImplicit(!durationFlag);
	}

	bool useTimeStampsFlag = 
		((MP4BitfieldProperty*)m_pProperties[5])->GetValue();

	for (u_int32_t i = 21; i <= 22; i++) {
		m_pProperties[i]->SetImplicit(useTimeStampsFlag);

		u_int8_t timeStampLength = MIN(64,
			((MP4Integer8Property*)m_pProperties[10])->GetValue());

		((MP4BitfieldProperty*)m_pProperties[i])->SetNumBits(timeStampLength);
		
	}
}

MP4IPIPtrDescriptorProperty::MP4IPIPtrDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4IPIPtrDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer16Property("IPIESId"));
}

MP4IPMPPtrDescriptorProperty::MP4IPMPPtrDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4IPMPPtrDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("IPMPDescriptorId"));
}

MP4IPMPDescriptorProperty::MP4IPMPDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4IPMPDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("IPMPDescriptorId"));
	AddProperty( /* 1 */
		new MP4Integer16Property("IPMPSType"));
	AddProperty( /* 2 */
		new MP4BytesProperty("IPMPData"));
	/* note: if IPMPSType == 0, IPMPData is an URL */
}

void MP4IPMPDescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* byte properties need to know how long they are before reading */
		((MP4BytesProperty*)m_pProperties[2])->SetValueSize(
			m_length - 3, GetCount() - 1);

		ReadProperties(pFile, GetCount() - 1);
	}
}

MP4QosDescriptorProperty::MP4QosDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4QosDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("predefined"));
#ifdef TBD
	AddProperty( /* 1 */
		new MP4QosQualifierXXX("qualifiers"));
#endif
}

#ifdef TBD
MP4MaxDelayQosQualifier::MP4MaxDelayQosQualifier(char *name)
	: MP4QosQualifier(MP4MaxDelayQosTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxDelay"));
}

MP4PrefMaxDelayQosQualifier::MP4PrefMaxDelayQosQualifier(char *name)
	: MP4QosQualifier(MP4PrefMaxDelayQosTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("prefMaxDelay"));
}

MP4LossProbQosQualifier::MP4LossProbQosQualifier(char *name)
	: MP4QosQualifier(MP4LossProbQosTag, name)
{
	// ??? what does ISO mean by double(32) ??? 
	// for now pretend it's a 32 bit int
	AddProperty( /* 0 */
		new MP4Integer32Property("lossProb"));
}

MP4MaxGapLossQosQualifier::MP4MaxGapLossQosQualifier(char *name)
	: MP4QosQualifier(MP4MaxGapLossQosTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxGapLoss"));
}

MP4MaxAUSizeQosQualifier::MP4MaxAUSizeQosQualifier(char *name)
	: MP4QosQualifier(MP4MaxAUSizeQosTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxAUSize"));
}

MP4AvgAUSizeQosQualifier::MP4AvgAUSizeQosQualifier(char *name)
	: MP4QosQualifier(MP4AvgAUSizeQosTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("avgAUSize"));
}

MP4MaxAURateQosQualifier::MP4MaxAURateQosQualifier(char *name)
	: MP4QosQualifier(MP4MaxAURateQosTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxAURate"));
}
#endif

MP4RegistrationDescriptorProperty::MP4RegistrationDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4RegistrationDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("formatIdentifier"));
	AddProperty( /* 1 */
		new MP4BytesProperty("additionalIdentificationInfo"));
}

void MP4RegistrationDescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (PeekTag(pFile, m_tag)) {
		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* byte properties need to know how long they are before reading */
		((MP4BytesProperty*)m_pProperties[1])->SetValueSize(m_length - 4, 
			GetCount() - 1);

		ReadProperties(pFile, GetCount() - 1);
	}
}

MP4ExtProfileLevelDescriptorProperty::MP4ExtProfileLevelDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4ExtProfileLevelDescrTag, name)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("profileLevelIndicationIndex"));
	AddProperty( /* 1 */
		new MP4Integer8Property("ODProfileLevelIndication"));
	AddProperty( /* 2 */
		new MP4Integer8Property("sceneProfileLevelIndication"));
	AddProperty( /* 3 */
		new MP4Integer8Property("audioProfileLevelIndication"));
	AddProperty( /* 4 */
		new MP4Integer8Property("visualProfileLevelIndication"));
	AddProperty( /* 5 */
		new MP4Integer8Property("graphicsProfileLevelIndication"));
	AddProperty( /* 6 */
		new MP4Integer8Property("MPEGJProfileLevelIndication"));
}

MP4ExtensionDescriptorProperty::MP4ExtensionDescriptorProperty(char* name)
	: MP4DescriptorProperty(MP4ExtDescrTagsStart, name)
{
	AddProperty( /* 0 */
		new MP4BytesProperty("data"));
}

void MP4ExtensionDescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}

	while (true) {
		u_int8_t tag;
		pFile->PeekBytes(&tag, 1);
		if (tag < MP4ExtDescrTagsStart || tag > MP4ExtDescrTagsEnd) {
			break;
		}
		SetTag(tag);

		SetCount(GetCount() + 1);

		ReadHeader(pFile);

		/* byte properties need to know how long they are before reading */
		((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_length,
			 GetCount() - 1);

		ReadProperties(pFile, GetCount() - 1);
	}
}

