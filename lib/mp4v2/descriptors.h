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

#ifndef __DESCRIPTORS_INCLUDED__
#define __DESCRIPTORS_INCLUDED__

const u_int8_t MP4ESDescrTag				= 0x03; 
const u_int8_t MP4DecConfigDescrTag			= 0x04; 
const u_int8_t MP4DecSpecificDescrTag		= 0x05; 
const u_int8_t MP4SLConfigDescrTag		 	= 0x06; 
const u_int8_t MP4ContentIdDescrTag		 	= 0x07; 
const u_int8_t MP4SupplContentIdDescrTag 	= 0x08; 
const u_int8_t MP4IPIPtrDescrTag		 	= 0x09; 
const u_int8_t MP4IPMPPtrDescrTag		 	= 0x0A; 
const u_int8_t MP4IPMPDescrTag			 	= 0x0B; 
const u_int8_t MP4QosDescrTag			 	= 0x0C; 
const u_int8_t MP4RegistrationDescrTag	 	= 0x0D; 
const u_int8_t MP4ESIDIncDescrTag			= 0x0E; 
const u_int8_t MP4IODescrTag				= 0x10; 
const u_int8_t MP4ExtProfileLevelDescrTag 	= 0x13; 
const u_int8_t MP4OCIDescrTagsStart	 		= 0x40; 
const u_int8_t MP4ContentClassDescrTag 		= 0x40; 
const u_int8_t MP4KeywordDescrTag 			= 0x41; 
const u_int8_t MP4RatingDescrTag 			= 0x42; 
const u_int8_t MP4LanguageDescrTag	 		= 0x43;
const u_int8_t MP4ShortTextDescrTag	 		= 0x44;
const u_int8_t MP4ExpandedTextDescrTag 		= 0x45;
const u_int8_t MP4ContentCreatorDescrTag	= 0x46;
const u_int8_t MP4ContentCreationDescrTag	= 0x47;
const u_int8_t MP4OCICreatorDescrTag		= 0x48;
const u_int8_t MP4OCICreationDescrTag		= 0x49;
const u_int8_t MP4SmpteCameraDescrTag		= 0x4A;
const u_int8_t MP4OCIDescrTagsEnd			= 0x5F; 
const u_int8_t MP4ExtDescrTagsStart			= 0x80; 
const u_int8_t MP4ExtDescrTagsEnd			= 0xFE; 

class MP4IODescriptor : public MP4Descriptor {
public:
	MP4IODescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4ESIDDescriptor : public MP4Descriptor {
public:
	MP4ESIDDescriptor();
};

class MP4ESDescriptor : public MP4Descriptor {
public:
	MP4ESDescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4DecConfigDescriptor : public MP4Descriptor {
public:
	MP4DecConfigDescriptor();
};

class MP4DecSpecificDescriptor : public MP4Descriptor {
public:
	MP4DecSpecificDescriptor();
	void Read(MP4File* pFile);
};

class MP4SLConfigDescriptor : public MP4Descriptor {
public:
	MP4SLConfigDescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4IPIPtrDescriptor : public MP4Descriptor {
public:
	MP4IPIPtrDescriptor();
};

class MP4ContentIdDescriptor : public MP4Descriptor {
public:
	MP4ContentIdDescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4SupplContentIdDescriptor : public MP4Descriptor {
public:
	MP4SupplContentIdDescriptor();
};

class MP4IPMPPtrDescriptor : public MP4Descriptor {
public:
	MP4IPMPPtrDescriptor();
};

class MP4IPMPDescriptor : public MP4Descriptor {
public:
	MP4IPMPDescriptor();
	void Read(MP4File* pFile);
};

class MP4QosDescriptor : public MP4Descriptor {
public:
	MP4QosDescriptor();
};

typedef MP4Descriptor MP4QosQualifier;

const u_int8_t MP4QosTagsStart				= 0x01; 
const u_int8_t MP4MaxDelayQosTag			= 0x01; 
const u_int8_t MP4PrefMaxDelayQosTag		= 0x02; 
const u_int8_t MP4LossProbQosTag			= 0x03; 
const u_int8_t MP4MaxGapLossQosTag			= 0x04; 
const u_int8_t MP4MaxAUSizeQosTag			= 0x41; 
const u_int8_t MP4AvgAUSizeQosTag			= 0x42; 
const u_int8_t MP4MaxAURateQosTag			= 0x43; 
const u_int8_t MP4QosTagsEnd				= 0xFF; 

class MP4MaxDelayQosQualifier : public MP4QosQualifier {
public:
	MP4MaxDelayQosQualifier();
};

class MP4PrefMaxDelayQosQualifier : public MP4QosQualifier {
public:
	MP4PrefMaxDelayQosQualifier();
};

class MP4LossProbQosQualifier : public MP4QosQualifier {
public:
	MP4LossProbQosQualifier();
};

class MP4MaxGapLossQosQualifier : public MP4QosQualifier {
public:
	MP4MaxGapLossQosQualifier();
};

class MP4MaxAUSizeQosQualifier : public MP4QosQualifier {
public:
	MP4MaxAUSizeQosQualifier();
};

class MP4AvgAUSizeQosQualifier : public MP4QosQualifier {
public:
	MP4AvgAUSizeQosQualifier();
};

class MP4MaxAURateQosQualifier : public MP4QosQualifier {
public:
	MP4MaxAURateQosQualifier();
};

class MP4UnknownQosQualifier : public MP4QosQualifier {
public:
	MP4UnknownQosQualifier();
	void Read(MP4File* pFile);
};

class MP4RegistrationDescriptor : public MP4Descriptor {
public:
	MP4RegistrationDescriptor();
	void Read(MP4File* pFile);
};

class MP4ExtProfileLevelDescriptor : public MP4Descriptor {
public:
	MP4ExtProfileLevelDescriptor();
};

class MP4ContentClassDescriptor : public MP4Descriptor {
public:
	MP4ContentClassDescriptor();
	void Read(MP4File* pFile);
};

class MP4KeywordDescriptor : public MP4Descriptor {
public:
	MP4KeywordDescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4RatingDescriptor : public MP4Descriptor {
public:
	MP4RatingDescriptor();
	void Read(MP4File* pFile);
};

class MP4LanguageDescriptor : public MP4Descriptor {
public:
	MP4LanguageDescriptor();
};

class MP4ShortTextDescriptor : public MP4Descriptor {
public:
	MP4ShortTextDescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4ExpandedTextDescriptor : public MP4Descriptor {
public:
	MP4ExpandedTextDescriptor();
	void Read(MP4File* pFile);
protected:
	void Mutate();
};

class MP4CreatorDescriptor : public MP4Descriptor {
public:
	MP4CreatorDescriptor(u_int8_t tag);
};

class MP4CreationDescriptor : public MP4Descriptor {
public:
	MP4CreationDescriptor(u_int8_t tag);
};

class MP4SmpteCameraDescriptor : public MP4Descriptor {
public:
	MP4SmpteCameraDescriptor();
};

class MP4UnknownOCIDescriptor : public MP4Descriptor {
public:
	MP4UnknownOCIDescriptor();
	void Read(MP4File* pFile);
};

class MP4ExtensionDescriptor : public MP4Descriptor {
public:
	MP4ExtensionDescriptor();
	void Read(MP4File* pFile);
};

#endif /* __DESCRIPTORS_INCLUDED__ */

