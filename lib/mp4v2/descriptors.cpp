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
#include "descriptors.h"

#define Required	true
#define Optional	false
#define OnlyOne		true
#define Many		false
#define Counted		true

MP4IODescriptor::MP4IODescriptor()
	: MP4Descriptor(MP4IODescrTag)
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
		new MP4StringProperty("URL", Counted));
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
		new MP4DescriptorProperty("esIds", 
			MP4ESIDIncDescrTag, 0, Required, Many));
	AddProperty( /* 11 */ 
		new MP4DescriptorProperty("ociDescr", 
			MP4OCIDescrTagsStart, MP4OCIDescrTagsEnd, Optional, Many));
	AddProperty( /* 12 */
		new MP4DescriptorProperty("ipmpDescrPtr",
			MP4IPMPPtrDescrTag, 0, Optional, Many));
	AddProperty( /* 13 */
		new MP4DescriptorProperty("extDescr",
			MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));
}

void MP4IODescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* read the first two properties */
	ReadProperties(pFile, 0, 2);

	/* which allows us to reconfigure ourselves */
	Mutate();

	/* and read the remaining properties */
	ReadProperties(pFile, 2);
}

void MP4IODescriptor::Mutate()
{
	bool urlFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	m_pProperties[4]->SetImplicit(!urlFlag);
	for (u_int32_t i = 5; i <= 9; i++) {
		m_pProperties[i]->SetImplicit(urlFlag);
	}
}

MP4ESIDDescriptor::MP4ESIDDescriptor()
	: MP4Descriptor(MP4ESIDIncDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("id"));
}

MP4ESDescriptor::MP4ESDescriptor()
	: MP4Descriptor(MP4ESDescrTag)
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
		new MP4StringProperty("URL", Counted));
	AddProperty( /* 7 */
		new MP4Integer16Property("OCRESID"));
	AddProperty( /* 8 */
		new MP4DescriptorProperty("decConfigDescr",
			MP4DecConfigDescrTag, 0, Required, OnlyOne));
	AddProperty( /* 9 */
		new MP4DescriptorProperty("slConfigDescr",
			MP4SLConfigDescrTag, 0, Required, OnlyOne));
	AddProperty( /* 10 */
		new MP4DescriptorProperty("ipiPtr",
			MP4IPIPtrDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 11 */
		new MP4DescriptorProperty("ipIds",
			MP4ContentIdDescrTag, MP4SupplContentIdDescrTag, Optional, Many));
	AddProperty( /* 12 */
		new MP4DescriptorProperty("ipmpDescrPtr",
			MP4IPMPPtrDescrTag, 0, Optional, Many));
	AddProperty( /* 13 */
		new MP4DescriptorProperty("langDescr",
			MP4LanguageDescrTag, 0, Optional, Many));
	AddProperty( /* 14 */
		new MP4DescriptorProperty("qosDescr",
			MP4QosDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 15 */
		new MP4DescriptorProperty("regDescr",
			MP4RegistrationDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 16 */
		new MP4DescriptorProperty("extDescr",
			MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));
}

void MP4ESDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* read the first five properties */
	ReadProperties(pFile, 0, 5);

	/* which allows us to reconfigure ourselves */
	Mutate();

	/* and read the remaining properties */
	ReadProperties(pFile, 5);
}

void MP4ESDescriptor::Mutate()
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

MP4DecConfigDescriptor::MP4DecConfigDescriptor()
	: MP4Descriptor(MP4DecConfigDescrTag)
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
		new MP4DescriptorProperty("decSpecificInfo",
			MP4DecSpecificDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 8 */
		new MP4DescriptorProperty("profileLevelIndicationIndexDescr",
			MP4ExtProfileLevelDescrTag, 0, Optional, Many));
}

MP4DecSpecificDescriptor::MP4DecSpecificDescriptor()
	: MP4Descriptor(MP4DecSpecificDescrTag)
{
	AddProperty( /* 0 */
		new MP4BytesProperty("info"));
}

void MP4DecSpecificDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4SLConfigDescriptor::MP4SLConfigDescriptor()
	: MP4Descriptor(MP4SLConfigDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("predefined"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("useAccessUnitStartFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("useAccessUnitEndFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("useRandomAccessPointFlag", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("hasRandomAccessUnitsOnlyFlag", 1));
	AddProperty( /* 5 */
		new MP4BitfieldProperty("usePaddingFlag", 1));
	AddProperty( /* 6 */
		new MP4BitfieldProperty("useTimeStampsFlag", 1));
	AddProperty( /* 7 */
		new MP4BitfieldProperty("useIdleFlag", 1));
	AddProperty( /* 8 */
		new MP4BitfieldProperty("durationFlag", 1));
	AddProperty( /* 9 */
		new MP4Integer32Property("timeStampResolution"));
	AddProperty( /* 10 */
		new MP4Integer32Property("OCRResolution"));
	AddProperty( /* 11 */
		new MP4Integer8Property("timeStampLength"));
	AddProperty( /* 12 */
		new MP4Integer8Property("OCRLength"));
	AddProperty( /* 13 */
		new MP4Integer8Property("AULength"));
	AddProperty( /* 14 */
		new MP4Integer8Property("instantBitrateLength"));
	AddProperty( /* 15 */
		new MP4BitfieldProperty("degradationPriortyLength", 4));
	AddProperty( /* 16 */
		new MP4BitfieldProperty("AUSeqNumLength", 5));
	AddProperty( /* 17 */
		new MP4BitfieldProperty("packetSeqNumLength", 5));
	AddProperty( /* 18 */
		new MP4BitfieldProperty("reserved", 2));

	// if durationFlag 
	AddProperty( /* 19 */
		new MP4Integer32Property("timeScale"));
	AddProperty( /* 20 */
		new MP4Integer16Property("accessUnitDuration"));
	AddProperty( /* 21 */
		new MP4Integer16Property("compositionUnitDuration"));
	
	// if !useTimeStampsFlag
	AddProperty( /* 22 */
		new MP4BitfieldProperty("startDecodingTimeStamp", 64));
	AddProperty( /* 23 */
		new MP4BitfieldProperty("startCompositionTimeStamp", 64));
}

void MP4SLConfigDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* read the first property, 'predefined' */
	ReadProperties(pFile, 0, 1);

	/* if predefined == 0 */
	if (((MP4Integer8Property*)m_pProperties[0])->GetValue() == 0) {

		/* read the next 18 properties */
		ReadProperties(pFile, 1, 18);

		/* which allows us to reconfigure ourselves */
		Mutate();

		/* and read the remaining properties */
		ReadProperties(pFile, 19);

	} else {
		// everything else is implicit
		for (u_int32_t i = 1; i < m_pProperties.Size(); i++) {
			m_pProperties[i]->SetImplicit(true);
		}
	}
}

void MP4SLConfigDescriptor::Mutate()
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

MP4IPIPtrDescriptor::MP4IPIPtrDescriptor()
	: MP4Descriptor(MP4IPIPtrDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer16Property("IPIESId"));
}

MP4ContentIdDescriptor::MP4ContentIdDescriptor()
	: MP4Descriptor(MP4ContentIdDescrTag)
{
	AddProperty( /* 0 */
		new MP4BitfieldProperty("compatibility", 2));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("contentTypeFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("contentIdFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("protectedContent", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("reserved", 3));
	AddProperty( /* 5 */
		new MP4Integer8Property("contentType"));
	AddProperty( /* 6 */
		new MP4Integer8Property("contentIdType"));
	AddProperty( /* 7 */
		new MP4BytesProperty("contentId"));
}

void MP4ContentIdDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* read the first property, 'compatiblity' */
	ReadProperties(pFile, 0, 1);

	/* if compatiblity != 0 */
	if (((MP4Integer8Property*)m_pProperties[0])->GetValue() != 0) {
		/* we don't understand it */
		VERBOSE_READ(pFile->GetVerbosity(),
			printf("incompatible content id descriptor\n"));
		return;
	}

	/* read the next four properties */
	ReadProperties(pFile, 1, 4);

	/* which allows us to reconfigure ourselves */
	Mutate();

	/* read the remaining properties */
	ReadProperties(pFile, 5);
}

void MP4ContentIdDescriptor::Mutate()
{
	bool contentTypeFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	m_pProperties[5]->SetImplicit(!contentTypeFlag);

	bool contentIdFlag = ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
	m_pProperties[6]->SetImplicit(!contentIdFlag);
	m_pProperties[7]->SetImplicit(!contentIdFlag);
}

MP4SupplContentIdDescriptor::MP4SupplContentIdDescriptor()
	: MP4Descriptor(MP4SupplContentIdDescrTag)
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4StringProperty("title", Counted));
	AddProperty( /* 2 */
		new MP4StringProperty("value", Counted));
}

MP4IPMPPtrDescriptor::MP4IPMPPtrDescriptor()
	: MP4Descriptor(MP4IPMPPtrDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("IPMPDescriptorId"));
}

MP4IPMPDescriptor::MP4IPMPDescriptor()
	: MP4Descriptor(MP4IPMPDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("IPMPDescriptorId"));
	AddProperty( /* 1 */
		new MP4Integer16Property("IPMPSType"));
	AddProperty( /* 2 */
		new MP4BytesProperty("IPMPData"));
	/* note: if IPMPSType == 0, IPMPData is an URL */
}

void MP4IPMPDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 3);

	ReadProperties(pFile);
}

MP4QosDescriptor::MP4QosDescriptor()
	: MP4Descriptor(MP4QosDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("predefined"));
	AddProperty( /* 1 */
		new MP4QosQualifierProperty("qualifiers",
			MP4QosTagsStart, MP4QosTagsEnd, Optional, Many));
}

MP4MaxDelayQosQualifier::MP4MaxDelayQosQualifier()
	: MP4QosQualifier(MP4MaxDelayQosTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxDelay"));
}

MP4PrefMaxDelayQosQualifier::MP4PrefMaxDelayQosQualifier()
	: MP4QosQualifier(MP4PrefMaxDelayQosTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("prefMaxDelay"));
}

MP4LossProbQosQualifier::MP4LossProbQosQualifier()
	: MP4QosQualifier(MP4LossProbQosTag)
{
	AddProperty( /* 0 */
		new MP4Float32Property("lossProb"));
}

MP4MaxGapLossQosQualifier::MP4MaxGapLossQosQualifier()
	: MP4QosQualifier(MP4MaxGapLossQosTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxGapLoss"));
}

MP4MaxAUSizeQosQualifier::MP4MaxAUSizeQosQualifier()
	: MP4QosQualifier(MP4MaxAUSizeQosTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxAUSize"));
}

MP4AvgAUSizeQosQualifier::MP4AvgAUSizeQosQualifier()
	: MP4QosQualifier(MP4AvgAUSizeQosTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("avgAUSize"));
}

MP4MaxAURateQosQualifier::MP4MaxAURateQosQualifier()
	: MP4QosQualifier(MP4MaxAURateQosTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("maxAURate"));
}

MP4UnknownQosQualifier::MP4UnknownQosQualifier()
	: MP4QosQualifier()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("data"));
}

void MP4UnknownQosQualifier::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4RegistrationDescriptor::MP4RegistrationDescriptor()
	: MP4Descriptor(MP4RegistrationDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("formatIdentifier"));
	AddProperty( /* 1 */
		new MP4BytesProperty("additionalIdentificationInfo"));
}

void MP4RegistrationDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[1])->SetValueSize(m_size - 4);

	ReadProperties(pFile);
}

MP4ExtProfileLevelDescriptor::MP4ExtProfileLevelDescriptor()
	: MP4Descriptor(MP4ExtProfileLevelDescrTag)
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

MP4ContentClassDescriptor::MP4ContentClassDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4Integer32Property("classificationEntity"));
	AddProperty( /* 1 */
		new MP4Integer16Property("classificationTable"));
	AddProperty( /* 2 */
		new MP4BytesProperty("contentClassificationData"));
}

void MP4ContentClassDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 6);

	ReadProperties(pFile);
}

MP4KeywordDescriptor::MP4KeywordDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 7));
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("keywordCount");
	AddProperty(pCount); /* 3 */

	MP4TableProperty* pTable = new MP4TableProperty("keywords", pCount);
	AddProperty(pTable); /* 4 */

	pTable->AddProperty(
		new MP4StringProperty("string", Counted));
}

void MP4KeywordDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);
	ReadProperties(pFile, 0, 2);
	Mutate();
	ReadProperties(pFile, 2);
}

void MP4KeywordDescriptor::Mutate()
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	MP4Property* pProperty =
		((MP4TableProperty*)m_pProperties[4])->GetProperty(0);
	ASSERT(pProperty);
	((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);
}

MP4RatingDescriptor::MP4RatingDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4Integer32Property("ratingEntity"));
	AddProperty( /* 1 */
		new MP4Integer16Property("ratingCriteria"));
	AddProperty( /* 2 */
		new MP4BytesProperty("ratingInfo"));
}

void MP4RatingDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 6);

	ReadProperties(pFile);
}

MP4LanguageDescriptor::MP4LanguageDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
}

MP4ShortTextDescriptor::MP4ShortTextDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 7));
	AddProperty( /* 3 */
		new MP4StringProperty("eventName", Counted));
	AddProperty( /* 4 */
		new MP4StringProperty("eventText", Counted));
}

void MP4ShortTextDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);
	ReadProperties(pFile, 0, 2);
	Mutate();
	ReadProperties(pFile, 2);
}

void MP4ShortTextDescriptor::Mutate()
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);
	((MP4StringProperty*)m_pProperties[4])->SetUnicode(!utf8Flag);
}

MP4ExpandedTextDescriptor::MP4ExpandedTextDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 7));
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("itemCount");
	AddProperty(pCount); /* 3 */

	MP4TableProperty* pTable = new MP4TableProperty("items", pCount);
	AddProperty(pTable); /* 4 */

	pTable->AddProperty( /* Table 0 */
		new MP4StringProperty("itemDescription", Counted));
	pTable->AddProperty( /* Table 1 */
		new MP4StringProperty("itemText", Counted));

	AddProperty( /* 5 */
		new MP4StringProperty("nonItemText"));
	((MP4StringProperty*)m_pProperties[5])->SetExpandedCountedFormat(true);
}

void MP4ExpandedTextDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);
	ReadProperties(pFile, 0, 2);
	Mutate();
	ReadProperties(pFile, 2);
}

void MP4ExpandedTextDescriptor::Mutate()
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

	MP4Property* pProperty =
		((MP4TableProperty*)m_pProperties[4])->GetProperty(0);
	ASSERT(pProperty);
	((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);

	pProperty = ((MP4TableProperty*)m_pProperties[4])->GetProperty(1);
	ASSERT(pProperty);
	((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);

	((MP4StringProperty*)m_pProperties[5])->SetUnicode(!utf8Flag);
}

class MP4CreatorTableProperty : public MP4TableProperty {
public:
	MP4CreatorTableProperty(char* name, MP4Integer8Property* pCountProperty) :
		MP4TableProperty(name, pCountProperty) {
	};
protected:
	void ReadEntry(MP4File* pFile, u_int32_t index);
	void WriteEntry(MP4File* pFile, u_int32_t index);
};

MP4CreatorDescriptor::MP4CreatorDescriptor(u_int8_t tag)
	: MP4Descriptor(tag)
{
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("creatorCount");
	AddProperty(pCount); /* 0 */

	MP4TableProperty* pTable = new MP4CreatorTableProperty("creators", pCount);
	AddProperty(pTable); /* 1 */

	pTable->AddProperty( /* Table 0 */
		new MP4BytesProperty("languageCode", 3));
	pTable->AddProperty( /* Table 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	pTable->AddProperty( /* Table 2 */
		new MP4BitfieldProperty("reserved", 7));
	pTable->AddProperty( /* Table 3 */
		new MP4StringProperty("name", Counted));
}

void MP4CreatorTableProperty::ReadEntry(MP4File* pFile, u_int32_t index)
{
	m_pProperties[0]->Read(pFile, index);
	m_pProperties[1]->Read(pFile, index);

	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue(index);
	((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);

	m_pProperties[2]->Read(pFile, index);
	m_pProperties[3]->Read(pFile, index);
}

void MP4CreatorTableProperty::WriteEntry(MP4File* pFile, u_int32_t index)
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue(index);
	((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);

	MP4TableProperty::WriteEntry(pFile, index);
}

MP4CreationDescriptor::MP4CreationDescriptor(u_int8_t tag)
	: MP4Descriptor(tag)
{
	AddProperty( /* 0 */
		new MP4BitfieldProperty("contentCreationDate", 40));
}

MP4SmpteCameraDescriptor::MP4SmpteCameraDescriptor()
	: MP4Descriptor()
{
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("parameterCount"); 
	AddProperty(pCount);

	MP4TableProperty* pTable = new MP4TableProperty("parameters", pCount);
	AddProperty(pTable);

	pTable->AddProperty(
		new MP4Integer8Property("id"));
	pTable->AddProperty(
		new MP4Integer32Property("value"));
}

MP4UnknownOCIDescriptor::MP4UnknownOCIDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("data"));
}

void MP4UnknownOCIDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4ExtensionDescriptor::MP4ExtensionDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("data"));
}

void MP4ExtensionDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4Descriptor* MP4DescriptorProperty::CreateDescriptor(u_int8_t tag) 
{
	MP4Descriptor* pDescriptor = NULL;

	switch (tag) {
	case MP4ESDescrTag:
		pDescriptor = new MP4ESDescriptor();
		break;
	case MP4DecConfigDescrTag:
		pDescriptor = new MP4DecConfigDescriptor();
		break;
	case MP4DecSpecificDescrTag:
		pDescriptor = new MP4DecSpecificDescriptor();
		break;
	case MP4SLConfigDescrTag:
		pDescriptor = new MP4SLConfigDescriptor();
		break;
	case MP4ContentIdDescrTag:
		pDescriptor = new MP4ContentIdDescriptor();
		break;
	case MP4SupplContentIdDescrTag:
		pDescriptor = new MP4SupplContentIdDescriptor();
		break;
	case MP4IPIPtrDescrTag:
		pDescriptor = new MP4IPIPtrDescriptor();
		break;
	case MP4IPMPPtrDescrTag:
		pDescriptor = new MP4IPMPPtrDescriptor();
		break;
	case MP4IPMPDescrTag:
		pDescriptor = new MP4IPMPDescriptor();
		break;
	case MP4QosDescrTag:
		pDescriptor = new MP4QosDescriptor();
		break;
	case MP4RegistrationDescrTag:
		pDescriptor = new MP4RegistrationDescriptor();
		break;
	case MP4ESIDIncDescrTag:
		pDescriptor = new MP4ESIDDescriptor();
		break;
	case MP4IODescrTag:
		pDescriptor = new MP4IODescriptor();
		break;
	case MP4ExtProfileLevelDescrTag:
		pDescriptor = new MP4ExtProfileLevelDescriptor();
		break;
	case MP4ContentClassDescrTag:
		pDescriptor = new MP4ContentClassDescriptor();
		break;
	case MP4KeywordDescrTag:
		pDescriptor = new MP4KeywordDescriptor();
		break;
	case MP4RatingDescrTag:
		pDescriptor = new MP4RatingDescriptor();
		break;
	case MP4LanguageDescrTag:
		pDescriptor = new MP4LanguageDescriptor();
		break;
	case MP4ShortTextDescrTag:
		pDescriptor = new MP4ShortTextDescriptor();
		break;
	case MP4ExpandedTextDescrTag:
		pDescriptor = new MP4ExpandedTextDescriptor();
		break;
	case MP4ContentCreatorDescrTag:
	case MP4OCICreatorDescrTag:
		pDescriptor = new MP4CreatorDescriptor(tag);
		break;
	case MP4ContentCreationDescrTag:
	case MP4OCICreationDescrTag:
		pDescriptor = new MP4CreationDescriptor(tag);
		break;
	case MP4SmpteCameraDescrTag:
		pDescriptor = new MP4SmpteCameraDescriptor();
		break;
	}

	if (pDescriptor == NULL) {
		if (tag >= MP4OCIDescrTagsStart && tag <= MP4OCIDescrTagsEnd) {
			pDescriptor = new MP4UnknownOCIDescriptor();
			pDescriptor->SetTag(tag);
		}

		if (tag >= MP4ExtDescrTagsStart && tag <= MP4ExtDescrTagsEnd) {
			pDescriptor = new MP4ExtensionDescriptor();
			pDescriptor->SetTag(tag);
		}
	}

	return pDescriptor;
}

MP4Descriptor* MP4QosQualifierProperty::CreateDescriptor(u_int8_t tag) 
{
	MP4Descriptor* pDescriptor = NULL;

	switch (tag) {
	case MP4MaxDelayQosTag:
		pDescriptor = new MP4MaxDelayQosQualifier();
		break;
	case MP4PrefMaxDelayQosTag:
		pDescriptor = new MP4PrefMaxDelayQosQualifier();
		break;
	case MP4LossProbQosTag:
		pDescriptor = new MP4LossProbQosQualifier();
		break;
	case MP4MaxGapLossQosTag:
		pDescriptor = new MP4MaxGapLossQosQualifier();
		break;
	case MP4MaxAUSizeQosTag:
		pDescriptor = new MP4MaxAUSizeQosQualifier();
		break;
	case MP4AvgAUSizeQosTag:
		pDescriptor = new MP4AvgAUSizeQosQualifier();
		break;
	case MP4MaxAURateQosTag:
		pDescriptor = new MP4MaxAURateQosQualifier();
		break;
	default:
		pDescriptor = new MP4UnknownQosQualifier();
		pDescriptor->SetTag(tag);
	}
	
	return pDescriptor;
}
