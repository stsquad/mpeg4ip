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

#ifndef __MP4_PROPERTY_INCLUDED__
#define __MP4_PROPERTY_INCLUDED__

// forward declarations
class MP4Atom;

class MP4Descriptor;
MP4ARRAY_DECL(MP4Descriptor, MP4Descriptor*);

enum MP4PropertyType {
	Integer8Property,
	Integer16Property,
	Integer24Property,
	Integer32Property,
	Integer64Property,
	Float32Property,
	StringProperty,
	BytesProperty,
	TableProperty,
	DescriptorProperty,
};

class MP4Property {
public:
	MP4Property(char *name = NULL) {
		m_name = name;
		m_pParentAtom = NULL;
		m_readOnly = false;
		m_implicit = false;
	}
	virtual ~MP4Property() { }

	MP4Atom* GetParentAtom() {
		return m_pParentAtom;
	}
	virtual void SetParentAtom(MP4Atom* pParentAtom) {
		m_pParentAtom = pParentAtom;
	}

	char *GetName() {
		return m_name;
	}

	virtual MP4PropertyType GetType() = NULL; 

	bool IsReadOnly() {
		return m_readOnly;
	}
	void SetReadOnly(bool value = true) {
		m_readOnly = value;
	}

	bool IsImplicit() {
		return m_implicit;
	}
	void SetImplicit(bool value = true) {
		m_implicit = value;
	}

	virtual u_int32_t GetCount() = NULL;
	virtual void SetCount(u_int32_t count) = NULL;

	virtual void Generate() { /* default is a no-op */ };
	virtual void Read(MP4File* pFile, u_int32_t index = 0) = NULL;
	virtual void Write(MP4File* pFile, u_int32_t index = 0) = NULL;
	virtual void Dump(FILE* pFile, u_int32_t index = 0) = NULL;

	virtual bool FindProperty(char* name,
	  MP4Property** ppProperty, u_int32_t* pIndex = NULL);

protected:
	MP4Atom* m_pParentAtom;
	char* m_name;
	bool m_readOnly;
	bool m_implicit;
};

MP4ARRAY_DECL(MP4Property, MP4Property*);

#define MP4INTEGER_PROPERTY_DECL2(isize, xsize) \
	class MP4Integer##xsize##Property : public MP4Property { \
	public: \
		MP4Integer##xsize##Property(char* name) \
			: MP4Property(name) { \
			SetCount(1); \
			m_values[0] = 0; \
		} \
		\
		MP4PropertyType GetType() { \
			return Integer##xsize##Property; \
		} \
		\
		u_int32_t GetCount() { \
			return m_values.Size(); \
		} \
		void SetCount(u_int32_t count) { \
			m_values.Resize(count); \
		} \
		\
		u_int##isize##_t GetValue(u_int32_t index = 0) { \
			return m_values[index]; \
		} \
		\
		void SetValue(u_int##isize##_t value, u_int32_t index = 0) { \
			if (m_readOnly) { \
				throw new MP4Error(EACCES, "property is read-only", m_name); \
			} \
			m_values[index] = value; \
		} \
		void IncrementValue(u_int32_t index = 0) { \
			m_values[index] += 1; \
		} \
		void Read(MP4File* pFile, u_int32_t index = 0) { \
			if (m_implicit) { \
				return; \
			} \
			m_values[index] = pFile->ReadUInt##xsize(); \
		} \
		\
		void Write(MP4File* pFile, u_int32_t index = 0) { \
			if (m_implicit) { \
				return; \
			} \
			pFile->WriteUInt##xsize(m_values[index]); \
		} \
		\
		void Dump(FILE* pFile, u_int32_t index = 0); \
	\
	protected: \
		MP4Integer##isize##Array m_values; \
	};

#define MP4INTEGER_PROPERTY_DECL(size) \
	MP4INTEGER_PROPERTY_DECL2(size, size)

MP4INTEGER_PROPERTY_DECL(8);
MP4INTEGER_PROPERTY_DECL(16);
MP4INTEGER_PROPERTY_DECL2(32, 24);
MP4INTEGER_PROPERTY_DECL(32);
MP4INTEGER_PROPERTY_DECL(64);

class MP4BitfieldProperty : public MP4Integer64Property {
public:
	MP4BitfieldProperty(char* name, u_int8_t numBits)
		: MP4Integer64Property(name) {
		m_numBits = numBits;
	}

	u_int8_t GetNumBits() {
		return m_numBits;
	}
	void SetNumBits(u_int8_t numBits) {
		m_numBits = numBits;
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int32_t index = 0);

protected:
	u_int8_t m_numBits;
};

class MP4Float32Property : public MP4Property {
public:
	MP4Float32Property(char* name)
		: MP4Property(name) {
		SetCount(1);
		m_values[0] = 0.0;
	}

	MP4PropertyType GetType() {
		return Float32Property;
	}

	u_int32_t GetCount() {
		return m_values.Size();
	}
	void SetCount(u_int32_t count) {
		m_values.Resize(count);
	}

	float GetValue(u_int32_t index = 0) {
		return m_values[index];
	}

	void SetValue(float value, u_int32_t index = 0) {
		if (m_readOnly) {
			throw new MP4Error(EACCES, "property is read-only", m_name);
		}
		m_values[index] = value;
	}

	void Read(MP4File* pFile, u_int32_t index = 0) {
		if (m_implicit) {
			return;
		}
		m_values[index] = pFile->ReadFloat();
	}

	void Write(MP4File* pFile, u_int32_t index = 0) {
		if (m_implicit) {
			return;
		}
		pFile->WriteFloat(m_values[index]);
	}

	void Dump(FILE* pFile, u_int32_t index = 0);

protected:
	MP4Float32Array m_values;
};

class MP4StringProperty : public MP4Property {
public:
	MP4StringProperty(char* name, 
	  bool useCountedFormat = false, bool useUnicode = false)
		: MP4Property(name) {
		SetCount(1);
		m_values[0] = NULL;
		m_useCountedFormat = useCountedFormat;
		m_useExpandedCount = false;
		m_useUnicode = useUnicode;
	}
	~MP4StringProperty() {
		u_int32_t count = GetCount();
		for (u_int32_t i = 0; i < count; i++) {
			MP4Free(m_values[i]);
		}
	}

	MP4PropertyType GetType() {
		return StringProperty;
	}

	u_int32_t GetCount() {
		return m_values.Size();
	}

	void SetCount(u_int32_t count) {
		m_values.Resize(count);
	}

	const char* GetValue(u_int32_t index = 0) {
		return m_values[index];
	}

	void SetValue(char* value, u_int32_t index = 0) {
		if (m_readOnly) {
			throw new MP4Error(EACCES, "property is read-only", m_name);
		}
		MP4Free(m_values[index]);
		if (value) {
			m_values[index] = MP4Stralloc(value);
		} else {
			m_values[index] = NULL;
		}
	}

	bool IsCountedFormat() {
		return m_useCountedFormat;
	}

	void SetCountedFormat(bool useCountedFormat) {
		m_useCountedFormat = useCountedFormat;
	}

	bool IsExpandedCountedFormat() {
		return m_useExpandedCount;
	}

	void SetExpandedCountedFormat(bool useExpandedCount) {
		m_useExpandedCount = useExpandedCount;
	}

	bool IsUnicode() {
		return m_useUnicode;
	}

	void SetUnicode(bool useUnicode) {
		m_useUnicode = useUnicode;
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int32_t index = 0);

protected:
	bool m_useCountedFormat;
	bool m_useExpandedCount;
	bool m_useUnicode;
	MP4StringArray m_values;
};

class MP4BytesProperty : public MP4Property {
public:
	MP4BytesProperty(char* name, u_int32_t valueSize = 0)
		: MP4Property(name) {
		SetCount(1);
		m_values[0] = (u_int8_t*)MP4Calloc(valueSize);
		m_valueSizes[0] = valueSize;
	}
	~MP4BytesProperty() {
		u_int32_t count = GetCount();
		for (u_int32_t i = 0; i < count; i++) {
			MP4Free(m_values[i]);
		}
	}

	MP4PropertyType GetType() {
		return BytesProperty;
	}

	u_int32_t GetCount() {
		return m_values.Size();
	}
	void SetCount(u_int32_t count) {
		m_values.Resize(count);
		m_valueSizes.Resize(count);
	}

	void GetValue(u_int8_t** ppValue, u_int32_t* pValueSize, 
	  u_int32_t index = 0) {
		// N.B. caller must free memory
		*ppValue = (u_int8_t*)MP4Malloc(m_valueSizes[index]);
		memcpy(*ppValue, m_values[index], m_valueSizes[index]);
		*pValueSize = m_valueSizes[index];
	}

	void SetValue(u_int8_t* pValue, u_int32_t valueSize, 
	  u_int32_t index = 0) {
		if (m_readOnly) {
			throw new MP4Error(EACCES, "property is read-only", m_name);
		}
		MP4Free(m_values[index]);
		if (pValue) {
			m_values[index] = (u_int8_t*)MP4Malloc(valueSize);
			memcpy(m_values[index], pValue, valueSize);
			m_valueSizes[index] = valueSize;
		} else {
			m_values[index] = NULL;
			m_valueSizes[index] = 0;
		}
	}

	u_int32_t GetValueSize(u_int32_t valueSize, u_int32_t index = 0) {
		return m_valueSizes[index];
	}
	void SetValueSize(u_int32_t valueSize, u_int32_t index = 0) {
		if (m_values[index] != NULL) {
			m_values[index] = (u_int8_t*)MP4Realloc(m_values[index], valueSize);
		}
		m_valueSizes[index] = valueSize;
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int32_t index = 0);

protected:
	MP4Integer32Array	m_valueSizes;
	MP4BytesArray		m_values;
};

class MP4TableProperty : public MP4Property {
public:
	MP4TableProperty(char* name, MP4Property* pCountProperty)
		: MP4Property(name) {
		ASSERT(pCountProperty->GetType() == Integer8Property
			|| pCountProperty->GetType() == Integer32Property);
		m_pCountProperty = pCountProperty;
		m_pCountProperty->SetReadOnly();
	}

	MP4PropertyType GetType() {
		return TableProperty;
	}

	void SetParentAtom(MP4Atom* pParentAtom) {
		m_pParentAtom = pParentAtom;
		for (u_int32_t i = 0; i < m_pProperties.Size(); i++) {
			m_pProperties[i]->SetParentAtom(pParentAtom);
		}
	}

	void AddProperty(MP4Property* pProperty) {
		ASSERT(pProperty);
		ASSERT(pProperty->GetType() != TableProperty);
		ASSERT(pProperty->GetType() != DescriptorProperty);
		m_pProperties.Add(pProperty);
		pProperty->SetParentAtom(m_pParentAtom);
	}

	MP4Property* GetProperty(u_int32_t index) {
		return m_pProperties[index];
	}

	u_int32_t GetCount() {
		if (m_pCountProperty->GetType() == Integer8Property) {
			return ((MP4Integer8Property*)m_pCountProperty)->GetValue();
		} else {
			return ((MP4Integer32Property*)m_pCountProperty)->GetValue();
		} 
	}
	void SetCount(u_int32_t count) {
		if (m_pCountProperty->GetType() == Integer8Property) {
			((MP4Integer8Property*)m_pCountProperty)->SetValue(count);
		} else {
			((MP4Integer32Property*)m_pCountProperty)->SetValue(count);
		} 
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int32_t index = 0);

	bool FindProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	virtual void ReadEntry(MP4File* pFile, u_int32_t index);
	virtual void WriteEntry(MP4File* pFile, u_int32_t index);

	bool FindContainedProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	MP4Property*		m_pCountProperty;
	MP4PropertyArray	m_pProperties;
};

class MP4DescriptorProperty : public MP4Property {
public:
	MP4DescriptorProperty(char* name = NULL, 
	  u_int8_t tagsStart = 0, u_int8_t tagsEnd = 0,
	  bool mandatory = false, bool onlyOne = false)
		: MP4Property(name) { 
		m_tagsStart = tagsStart;
		m_tagsEnd = tagsEnd;
		m_sizeLimit = 0;
		m_mandatory = mandatory;
		m_onlyOne = onlyOne;
	}

	MP4PropertyType GetType() {
		return DescriptorProperty;
	}

	void SetParentAtom(MP4Atom* pParentAtom);

	void SetSizeLimit(u_int64_t sizeLimit) {
		m_sizeLimit = sizeLimit;
	}

	u_int32_t GetCount() {
		return m_pDescriptors.Size();
	}
	void SetCount(u_int32_t count) {
		return m_pDescriptors.Resize(count);
	}

	void Generate();
	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int32_t index = 0);

	bool FindProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	virtual MP4Descriptor* CreateDescriptor(u_int8_t tag);

	bool FindContainedProperty(char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	u_int8_t			m_tagsStart;
	u_int8_t			m_tagsEnd;
	u_int64_t			m_sizeLimit;
	bool				m_mandatory;
	bool				m_onlyOne;
	MP4DescriptorArray	m_pDescriptors;
};

class MP4QosQualifierProperty : public MP4DescriptorProperty {
public:
	MP4QosQualifierProperty(char* name = NULL, 
	  u_int8_t tagsStart = 0, u_int8_t tagsEnd = 0,
	  bool mandatory = false, bool onlyOne = false) :
	MP4DescriptorProperty(name, tagsStart, tagsEnd, mandatory, onlyOne) { }

protected:
	MP4Descriptor* CreateDescriptor(u_int8_t tag);
};

#endif /* __MP4_PROPERTY_INCLUDED__ */
