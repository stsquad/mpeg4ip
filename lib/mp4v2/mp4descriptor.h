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

#ifndef __MP4_DESCRIPTOR_INCLUDED__
#define __MP4_DESCRIPTOR_INCLUDED__

class MP4Descriptor {
public:
	MP4Descriptor(u_int8_t tag = 0) {
		m_tag = tag;
	}

	u_int8_t GetTag() {
		return m_tag;
	}
	void SetTag(u_int8_t tag) {
		m_tag = tag;
	}

	void SetParentAtom(MP4Atom* pParentAtom) {
		m_pParentAtom = pParentAtom;
		for (u_int32_t i = 0; i < m_pProperties.Size(); i++) {
			m_pProperties[i]->SetParentAtom(pParentAtom);
		}
	}

	void AddProperty(MP4Property* pProperty) {
		ASSERT(pProperty);
		m_pProperties.Add(pProperty);
		pProperty->SetParentAtom(m_pParentAtom);
	}

	virtual void Read(MP4File* pFile);
	virtual void Write(MP4File* pFile);
	virtual void Dump(FILE* pFile);

	bool FindProperty(char* name, 
	  MP4Property** ppProperty, u_int32_t* pIndex) {
		return FindContainedProperty(name, ppProperty, pIndex);
	}

protected:
	void ReadHeader(MP4File* pFile);
	void ReadProperties(MP4File* pFile,
		u_int32_t startIndex = 0, u_int32_t count = 0xFFFFFFFF);

	virtual void Mutate() {
		// default is a no-op
	};

	bool FindContainedProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	MP4Atom*			m_pParentAtom;
	u_int8_t			m_tag;
	u_int64_t			m_start;
	u_int32_t			m_size;
	MP4PropertyArray	m_pProperties;
};

#endif /* __MP4_DESCRIPTOR_INCLUDED__ */
