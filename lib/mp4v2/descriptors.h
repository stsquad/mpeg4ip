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

const u_int8_t MP4IODescrTag				= 0x02; 
const u_int8_t MP4ESDescrTag				= 0x03; 
const u_int8_t MP4DecConfigDescrTag			= 0x04; 
const u_int8_t MP4DecSpecificDescrTag		= 0x05; 
const u_int8_t MP4SLConfigDescrTag		 	= 0x06; 
const u_int8_t MP4IPIPtrDescrTag		 	= 0x09; 
const u_int8_t MP4IPMPPtrDescrTag		 	= 0x0A; 
const u_int8_t MP4IPMPDescrTag			 	= 0x0B; 
const u_int8_t MP4QosDescrTag			 	= 0x0C; 
const u_int8_t MP4RegistrationDescrTag	 	= 0x0D; 
const u_int8_t MP4ExtProfileLevelDescrTag 	= 0x13; 
const u_int8_t MP4OCIDescrTagsStart	 		= 0x40; 
const u_int8_t MP4LanguageDescrTag	 		= 0x43; 
const u_int8_t MP4OCIDescrTagsEnd			= 0x5F; 
const u_int8_t MP4ExtDescrTagsStart			= 0x80; 
const u_int8_t MP4ExtDescrTagsEnd			= 0xFE; 

class MP4IODescriptorProperty : public MP4DescriptorProperty {
public:
	MP4IODescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);

protected:
	void Mutate();
};

class MP4ESDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4ESDescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);

protected:
	void Mutate();
};

class MP4DecConfigDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4DecConfigDescriptorProperty(char* name = NULL);
};

class MP4DecSpecificDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4DecSpecificDescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
};

class MP4SLConfigDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4SLConfigDescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);

protected:
	void Mutate();
};

class MP4IPIPtrDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4IPIPtrDescriptorProperty(char* name = NULL);
};

class MP4IPMPPtrDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4IPMPPtrDescriptorProperty(char* name = NULL);
};

class MP4IPMPDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4IPMPDescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
};

class MP4QosDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4QosDescriptorProperty(char* name = NULL);
};

const u_int8_t MP4MaxDelayQosTag			= 0x01; 
const u_int8_t MP4PrefMaxDelayQosTag		= 0x02; 
const u_int8_t MP4LossProbQosTag			= 0x03; 
const u_int8_t MP4MaxGapLossQosTag			= 0x04; 
const u_int8_t MP4MaxAUSizeQosTag			= 0x41; 
const u_int8_t MP4AvgAUSizeQosTag			= 0x42; 
const u_int8_t MP4MaxAURateQosTag			= 0x43; 

typedef MP4DescriptorProperty MP4QosQualifier;

class MP4RegistrationDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4RegistrationDescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
};

class MP4ExtProfileLevelDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4ExtProfileLevelDescriptorProperty(char* name = NULL);
};

class MP4ExtensionDescriptorProperty : public MP4DescriptorProperty {
public:
	MP4ExtensionDescriptorProperty(char* name = NULL);

	void Read(MP4File* pFile, u_int32_t index = 0);
};

#endif /* __DESCRIPTORS_INCLUDED__ */

