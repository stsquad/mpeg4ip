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

class MP4RootAtom : public MP4Atom {
public:
	MP4RootAtom() : MP4Atom(NULL) {
		ExpectChildAtom("ftyp", Required, OnlyOne);
		ExpectChildAtom("moov", Required, OnlyOne);
		ExpectChildAtom("mdat", Optional, Many);
		ExpectChildAtom("free", Optional, Many);
		ExpectChildAtom("skip", Optional, Many);
		ExpectChildAtom("udta", Optional, Many);
	}
	void Write() {
		WriteChildAtoms();
	}
};

/* PROPOSED */
class MP4FtypAtom : public MP4Atom {
public:
	MP4FtypAtom() : MP4Atom("ftyp") {
		AddProperty(
			new MP4Integer32Property("majorBrand"));
		AddProperty(
			new MP4Integer32Property("minorVersion"));

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("compatibleBrandsCount"); 
		pCount->SetImplicit();
		AddProperty(pCount);

		MP4TableProperty* pTable = 
			new MP4TableProperty("compatibleBrands", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("brand"));
	}

	void Generate() {
		((MP4Integer32Property*)m_pProperties[0])->SetValue(STRTOINT32("isom"));
	}

	void Read() {
		// table entry count computed from atom size
		((MP4Integer32Property*)m_pProperties[2])->SetReadOnly(false);
		((MP4Integer32Property*)m_pProperties[2])->SetValue((m_size - 8) / 4);
		((MP4Integer32Property*)m_pProperties[2])->SetReadOnly(true);

		MP4Atom::Read();
	}
};

class MP4MdatAtom : public MP4Atom {
public:
	MP4MdatAtom() : MP4Atom("mdat") { }

	void Read() {
		Skip();
	}
	// LATER option to use 64 bit write 
};

class MP4MoovAtom : public MP4Atom {
public:
	MP4MoovAtom() : MP4Atom("moov") {
		ExpectChildAtom("mvhd", Required, OnlyOne);
		ExpectChildAtom("iods", Required, OnlyOne);
		ExpectChildAtom("trak", Required, Many);
		ExpectChildAtom("udta", Optional, Many);
	}
};

class MP4MvhdAtom : public MP4Atom {
public:
	MP4MvhdAtom() : MP4Atom("mvhd") {
		AddVersionAndFlags();
	}
	void AddVersion0Properties() {
		AddProperty(
			new MP4Integer32Property("creationTime"));
		AddProperty(
			new MP4Integer32Property("modificationTime"));
		AddProperty(
			new MP4Integer32Property("timeScale"));
		AddProperty(
			new MP4Integer32Property("duration"));
		AddReserved("reserved", 76);
		AddProperty(
			new MP4Integer32Property("nextTrackId"));
	}
	void AddVersion1Properties() {
		AddProperty(
			new MP4Integer64Property("creationTime"));
		AddProperty(
			new MP4Integer64Property("modificationTime"));
		AddProperty(
			new MP4Integer32Property("timeScale"));
		AddProperty(
			new MP4Integer64Property("duration"));
		AddReserved("reserved", 76);
		AddProperty(
			new MP4Integer32Property("nextTrackId"));
	}
	void Generate() {
		SetVersion(1);
		AddVersion1Properties();

		// set creation and modification times
		MP4Timestamp now = MP4GetAbsTimestamp();
		((MP4Integer64Property*)m_pProperties[2])->SetValue(now);
		((MP4Integer64Property*)m_pProperties[3])->SetValue(now);

		// set next track id
		((MP4Integer32Property*)m_pProperties[7])->SetValue(1);
	}
	void Read() {
		/* read atom version */
		ReadProperties(0, 1);

		/* need to create the properties based on the atom version */
		if (GetVersion() == 1) {
			AddVersion1Properties();
		} else {
			AddVersion0Properties();
		}

		/* now we can read the remaining properties */
		ReadProperties(1);

		Skip();	// to end of atom
	}
};

class MP4IodsAtom : public MP4Atom {
public:
	MP4IodsAtom() : MP4Atom("iods") {
		AddVersionAndFlags();
		AddProperty(
			new MP4DescriptorProperty(NULL, 
				MP4IODescrTag, 0, Required, OnlyOne));
	}
};

class MP4TrakAtom : public MP4Atom {
public:
	MP4TrakAtom() : MP4Atom("trak") {
		ExpectChildAtom("tkhd", Required, OnlyOne);
		ExpectChildAtom("tref", Optional, OnlyOne);
		ExpectChildAtom("edts", Optional, OnlyOne);
		ExpectChildAtom("mdia", Required, OnlyOne);
		ExpectChildAtom("udta", Optional, Many);
	}
};

class MP4TkhdAtom : public MP4Atom {
public:
	MP4TkhdAtom() : MP4Atom("tkhd") {
		AddVersionAndFlags();
	}
	void AddVersion0Properties() {
		AddProperty(
			new MP4Integer32Property("creationTime"));
		AddProperty(
			new MP4Integer32Property("modificationTime"));
		AddProperty(
			new MP4Integer32Property("trackId"));
		AddReserved("reserved1", 4);
		AddProperty(
			new MP4Integer32Property("duration"));
		AddReserved("reserved2", 60);
	}
	void AddVersion1Properties() {
		AddProperty(
			new MP4Integer64Property("creationTime"));
		AddProperty(
			new MP4Integer64Property("modificationTime"));
		AddProperty(
			new MP4Integer32Property("trackId"));
		AddReserved("reserved1", 4);
		AddProperty(
			new MP4Integer64Property("duration"));
		AddReserved("reserved2", 60);
	}
	void Generate() {
		SetVersion(1);
		AddVersion1Properties();

		// set creation and modification times
		MP4Timestamp now = MP4GetAbsTimestamp();
		((MP4Integer64Property*)m_pProperties[2])->SetValue(now);
		((MP4Integer64Property*)m_pProperties[3])->SetValue(now);
	}
	void Read() {
		/* read atom version */
		ReadProperties(0, 1);

		/* need to create the properties based on the atom version */
		if (GetVersion() == 1) {
			AddVersion1Properties();
		} else {
			AddVersion0Properties();
		}

		/* now we can read the properties */
		ReadProperties(1);

		Skip();	// to end of atom
	}
};

class MP4TrefAtom : public MP4Atom {
public:
	MP4TrefAtom() : MP4Atom("tref") {
		ExpectChildAtom("hint", Optional, OnlyOne);
	}
};

class MP4HintAtom : public MP4Atom {
public:
	MP4HintAtom() : MP4Atom("hint") {
		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		pCount->SetImplicit();
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("trackId"));
	}
	void Read() {
		// table entry count computed from atom size
		((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(false);
		((MP4Integer32Property*)m_pProperties[0])->SetValue(m_size / 4);
		((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(true);

		MP4Atom::Read();
	}
};

class MP4MdiaAtom : public MP4Atom {
public:
	MP4MdiaAtom() : MP4Atom("mdia") {
		ExpectChildAtom("mdhd", Required, OnlyOne);
		ExpectChildAtom("hdlr", Required, OnlyOne);
		ExpectChildAtom("minf", Required, OnlyOne);
	}
};

class MP4MdhdAtom : public MP4Atom {
public:
	MP4MdhdAtom() : MP4Atom("mdhd") {
		AddVersionAndFlags();
	}
	void AddVersion0Properties() {
		AddProperty(
			new MP4Integer32Property("creationTime"));
		AddProperty(
			new MP4Integer32Property("modificationTime"));
		AddProperty(
			new MP4Integer32Property("timeScale"));
		AddProperty(
			new MP4Integer32Property("duration"));
		AddProperty(
			new MP4Integer16Property("language"));
		AddReserved("reserved", 2);
	}
	void AddVersion1Properties() {
		AddProperty(
			new MP4Integer64Property("creationTime"));
		AddProperty(
			new MP4Integer64Property("modificationTime"));
		AddProperty(
			new MP4Integer32Property("timeScale"));
		AddProperty(
			new MP4Integer64Property("duration"));
		AddProperty(
			new MP4Integer16Property("language"));
		AddReserved("reserved", 2);
	}
	void Generate() {
		SetVersion(1);
		AddVersion1Properties();

		// set creation and modification times
		MP4Timestamp now = MP4GetAbsTimestamp();
		((MP4Integer64Property*)m_pProperties[2])->SetValue(now);
		((MP4Integer64Property*)m_pProperties[3])->SetValue(now);
	}
	void Read() {
		/* read atom version */
		ReadProperties(0, 1);

		/* need to create the properties based on the atom version */
		if (GetVersion() == 1) {
			AddVersion1Properties();
		} else {
			AddVersion0Properties();
		}

		/* now we can read the properties */
		ReadProperties(1);

		Skip();	// to end of atom
	}
};

class MP4HdlrAtom : public MP4Atom {
public:
	MP4HdlrAtom() : MP4Atom("hdlr") {
		AddVersionAndFlags();
		AddReserved("reserved1", 4);
		AddProperty(
			new MP4Integer32Property("handlerType"));
		AddReserved("reserved2", 12);
		AddProperty(
			new MP4StringProperty("name"));
	}
	// There is a spec incompatiblity between QT and MP4
	// QT says name field is a counted string
	// MP4 says name field is a null terminated string
	// Here we attempt to make all things work
	void Read() {
		u_int32_t numProps = m_pProperties.Size();

		// read all the properties but the "name" field
		ReadProperties(0, numProps - 1);

		// take a peek at the next byte
		u_int8_t strLength;
		m_pFile->PeekBytes(&strLength, 1);

		// if the value matches the remaining atom length
		if (strLength + 1 == m_end - m_pFile->GetPosition()) {
			// read a counted string
			MP4StringProperty* pNameProp = 
				(MP4StringProperty*)m_pProperties[numProps - 1];
			pNameProp->SetCountedFormat(true);
			ReadProperties(numProps - 1, 1);
			pNameProp->SetCountedFormat(false);
		} else {
			// read a null terminated string
			ReadProperties(numProps - 1, 1);
		}

		Skip();	// to end of atom
	}
};

class MP4MinfAtom : public MP4Atom {
public:
	MP4MinfAtom() : MP4Atom("minf") {
		ExpectChildAtom("vmhd", Optional, OnlyOne);
		ExpectChildAtom("smhd", Optional, OnlyOne);
		ExpectChildAtom("hmhd", Optional, OnlyOne);
		ExpectChildAtom("nmhd", Optional, OnlyOne);
		ExpectChildAtom("dinf", Required, OnlyOne);
		ExpectChildAtom("stbl", Required, OnlyOne);
	}
};

class MP4VmhdAtom : public MP4Atom {
public:
	MP4VmhdAtom() : MP4Atom("vmhd") {
		AddVersionAndFlags();
		AddReserved("reserved", 8);
	}
};

class MP4SmhdAtom : public MP4Atom {
public:
	MP4SmhdAtom() : MP4Atom("smhd") {
		AddVersionAndFlags();
		AddReserved("reserved", 4);
	}
};

class MP4HmhdAtom : public MP4Atom {
public:
	MP4HmhdAtom() : MP4Atom("hmhd") {
		AddVersionAndFlags();

		AddProperty(	
			new MP4Integer16Property("maxPduSize")); 
		AddProperty(	
			new MP4Integer16Property("avgPduSize")); 
		AddProperty(	
			new MP4Integer32Property("maxBitRate")); 
		AddProperty(	
			new MP4Integer32Property("avgBitRate")); 
		AddProperty(	
			new MP4Integer32Property("slidingAvgBitRate")); 
	}
};

class MP4NmhdAtom : public MP4Atom {
public:
	MP4NmhdAtom() : MP4Atom("nmhd") {
		AddVersionAndFlags();
	}
};

class MP4DinfAtom : public MP4Atom {
public:
	MP4DinfAtom() : MP4Atom("dinf") {
		ExpectChildAtom("dref", Required, OnlyOne);
	}
};

class MP4DrefAtom : public MP4Atom {
public:
	MP4DrefAtom() : MP4Atom("dref") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		pCount->SetReadOnly();
		AddProperty(pCount);

		ExpectChildAtom("url ", Optional, Many);
		ExpectChildAtom("urn ", Optional, Many);
	}

	void Read() {
		/* do the usual read */
		MP4Atom::Read();

		// check that number of children == entryCount
		MP4Integer32Property* pCount = 
			(MP4Integer32Property*)m_pProperties[2];

		if (m_pChildAtoms.Size() != pCount->GetValue()) {
			VERBOSE_READ(GetVerbosity(),
				printf("Warning: dref inconsistency with number of entries"));

			/* fix it */
			pCount->SetReadOnly(false);
			pCount->SetValue(m_pChildAtoms.Size());
			pCount->SetReadOnly(true);
		}
	}
};

class MP4UrlAtom : public MP4Atom {
public:
	MP4UrlAtom() : MP4Atom("url ") {
		AddVersionAndFlags();
		AddProperty(new MP4StringProperty("location"));
	}
	void Read() {
		// read the version and flags properties
		ReadProperties(0, 2);

		// check if self-contained flag is set
		if (!(GetFlags() & 1)) {
			// if not then read url location
			ReadProperties(2);
		}

		Skip();	// to end of atom
	}
	void Write() {
		MP4StringProperty* pLocationProp =
			(MP4StringProperty*)m_pProperties[2];

		// if no url location has been set
		// then set self-contained flag
		if (pLocationProp->GetValue() == NULL) {
			SetFlags(GetFlags() | 1);
		}

		// write atom as usual
		MP4Atom::Write();
	}
};

class MP4UrnAtom : public MP4Atom {
public:
	MP4UrnAtom() : MP4Atom("urn ") {
		AddVersionAndFlags();
		AddProperty(new MP4StringProperty("name"));
		AddProperty(new MP4StringProperty("location"));
	}
	void Read() {
		// read the version, flags, and name properties
		ReadProperties(0, 3);

		// check if location is present
		if (m_pFile->GetPosition() < m_end) {
			// read it
			ReadProperties(3);
		}

		Skip();	// to end of atom
	}
};

class MP4StblAtom : public MP4Atom {
public:
	MP4StblAtom() : MP4Atom("stbl") {
		ExpectChildAtom("stsd", Required, OnlyOne);
		ExpectChildAtom("stts", Required, OnlyOne);
		ExpectChildAtom("ctts", Optional, OnlyOne);
		ExpectChildAtom("stsz", Required, OnlyOne);
		ExpectChildAtom("stsc", Required, OnlyOne);
		ExpectChildAtom("stco", Optional, OnlyOne);
		ExpectChildAtom("co64", Optional, OnlyOne);
		ExpectChildAtom("stss", Optional, OnlyOne);
		ExpectChildAtom("stsh", Optional, OnlyOne);
		ExpectChildAtom("stdp", Optional, OnlyOne);
	}
};

class MP4StsdAtom : public MP4Atom {
public:
	MP4StsdAtom() : MP4Atom("stsd") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		pCount->SetReadOnly();
		AddProperty(pCount);

		ExpectChildAtom("mp4a", Optional, Many);
		ExpectChildAtom("mp4s", Optional, Many);
		ExpectChildAtom("mp4v", Optional, Many);
	}

	void Read() {
		/* do the usual read */
		MP4Atom::Read();

		// check that number of children == entryCount
		MP4Integer32Property* pCount = 
			(MP4Integer32Property*)m_pProperties[2];

		if (m_pChildAtoms.Size() != pCount->GetValue()) {
			VERBOSE_READ(GetVerbosity(),
				printf("Warning: stsd inconsistency with number of entries"));

			/* fix it */
			pCount->SetReadOnly(false);
			pCount->SetValue(m_pChildAtoms.Size());
			pCount->SetReadOnly(true);
		}
	}
};

class MP4Mp4aAtom : public MP4Atom {
public:
	MP4Mp4aAtom() : MP4Atom("mp4a") {
		AddReserved("reserved1", 6);
		AddProperty(
			new MP4Integer16Property("dataReferenceIndex"));
		AddReserved("reserved2", 20);
		ExpectChildAtom("esds", Required, OnlyOne);
	}
};

class MP4Mp4sAtom : public MP4Atom {
public:
	MP4Mp4sAtom() : MP4Atom("mp4s") {
		AddReserved("reserved1", 6);
		AddProperty(
			new MP4Integer16Property("dataReferenceIndex"));
		ExpectChildAtom("esds", Required, OnlyOne);
	}
};

class MP4Mp4vAtom : public MP4Atom {
public:
	MP4Mp4vAtom() : MP4Atom("mp4v") {
		AddReserved("reserved1", 6);
		AddProperty(
			new MP4Integer16Property("dataReferenceIndex"));
		AddReserved("reserved2", 70);
		ExpectChildAtom("esds", Required, OnlyOne);
	}
};

class MP4EsdsAtom : public MP4Atom {
public:
	MP4EsdsAtom() : MP4Atom("esds") {
		AddVersionAndFlags();
		AddProperty(
			new MP4DescriptorProperty(NULL, 
				MP4ESDescrTag, 0, Required, OnlyOne));
	}
};

class MP4SttsAtom : public MP4Atom {
public:
	MP4SttsAtom() : MP4Atom("stts") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("sampleCount"));
		pTable->AddProperty(
			new MP4Integer32Property("sampleDelta"));
	}
};

class MP4CttsAtom : public MP4Atom {
public:
	MP4CttsAtom() : MP4Atom("ctts") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("sampleCount"));
		pTable->AddProperty(
			new MP4Integer32Property("sampleOffset"));
	}
};

class MP4StszAtom : public MP4Atom {
public:
	MP4StszAtom() : MP4Atom("stsz") {
		AddVersionAndFlags();

		AddProperty(
			new MP4Integer32Property("sampleSize")); 

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("sampleCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("sampleSize"));
	}
	void Read() {
		ReadProperties(0, 4);

		u_int32_t sampleSize = 
			((MP4Integer32Property*)m_pProperties[2])->GetValue();

		// only attempt to read entries table if sampleSize is zero
		// i.e sample size is not constant
		if (sampleSize == 0) {
			ReadProperties(4);
		}

		Skip();	// to end of atom
	}
	void Write() {
		u_int32_t sampleSize = 
			((MP4Integer32Property*)m_pProperties[2])->GetValue();

		// only attempt to write entries table if sampleSize is zero
		// i.e sample size is not constant
		m_pProperties[4]->SetImplicit(sampleSize != 0);

		MP4Atom::Write();
	}
};

class MP4StscAtom : public MP4Atom {
public:
	MP4StscAtom() : MP4Atom("stsc") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("firstChunk"));
		pTable->AddProperty(
			new MP4Integer32Property("samplesPerChunk"));
		pTable->AddProperty(
			new MP4Integer32Property("sampleDescriptionIndex"));

		// As an optimization we add an implicit property to this table,
		// "firstSample" that corresponds to the first sample of the firstChunk
		MP4Integer32Property* pSample =
			new MP4Integer32Property("firstSample");
		pSample->SetImplicit();
		pTable->AddProperty(pSample);
	}

	void Read() {
		// Read as usual
		MP4Atom::Read();

		// Compute the firstSample values for later use
		u_int32_t count = 
			((MP4Integer32Property*)m_pProperties[2])->GetValue();

		MP4Integer32Property* pFirstChunk = (MP4Integer32Property*)
			((MP4TableProperty*)m_pProperties[3])->GetProperty(0);
		MP4Integer32Property* pSamplesPerChunk = (MP4Integer32Property*)
			((MP4TableProperty*)m_pProperties[3])->GetProperty(1);
		MP4Integer32Property* pFirstSample = (MP4Integer32Property*)
			((MP4TableProperty*)m_pProperties[3])->GetProperty(3);

		MP4SampleId sampleId = 1;

		for (u_int32_t i = 0; i < count; i++) {
			pFirstSample->SetValue(sampleId, i);

			if (i < count - 1) {
				sampleId +=
					(pFirstChunk->GetValue(i+1) - pFirstChunk->GetValue(i))
					 * pSamplesPerChunk->GetValue(i);
			}
		}
	}
};

class MP4StcoAtom : public MP4Atom {
public:
	MP4StcoAtom() : MP4Atom("stco") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("chunkOffset"));
	}
};

class MP4Co64Atom : public MP4Atom {
public:
	MP4Co64Atom() : MP4Atom("co64") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer64Property("chunkOffset"));
	}
};

class MP4StssAtom : public MP4Atom {
public:
	MP4StssAtom() : MP4Atom("stss") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("sampleNumber"));
	}
};

class MP4StshAtom : public MP4Atom {
public:
	MP4StshAtom() : MP4Atom("stsh") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer32Property("shadowedSampleNumber"));
		pTable->AddProperty(
			new MP4Integer32Property("syncSampleNumber"));
	}
};

class MP4StdpAtom : public MP4Atom {
public:
	MP4StdpAtom() : MP4Atom("stdp") {
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		pCount->SetImplicit();
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4Integer16Property("priority"));
	}
	void Read() {
		// table entry count computed from atom size
		((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(false);
		((MP4Integer32Property*)m_pProperties[0])->SetValue((m_size - 4) / 2);
		((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(true);

		MP4Atom::Read();
	}
};

class MP4EdtsAtom : public MP4Atom {
public:
	MP4EdtsAtom() : MP4Atom("edts") { 
		ExpectChildAtom("elst", Required, OnlyOne);
	}
};

class MP4ElstAtom : public MP4Atom {
public:
	MP4ElstAtom() : MP4Atom("elst") { 
		AddVersionAndFlags();

		MP4Integer32Property* pCount = 
			new MP4Integer32Property("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);
	}
	void AddVersion0Properties() {
		MP4TableProperty* pTable = (MP4TableProperty*)m_pProperties[3];

		pTable->AddProperty(
			new MP4Integer32Property("segmentDuration"));
		pTable->AddProperty(
			new MP4Integer32Property("mediaTime"));
		pTable->AddProperty(
			new MP4Integer16Property("mediaRate"));
		pTable->AddProperty(
			new MP4Integer16Property("reserved"));
	}
	void AddVersion1Properties() {
		MP4TableProperty* pTable = (MP4TableProperty*)m_pProperties[3];

		pTable->AddProperty(
			new MP4Integer64Property("segmentDuration"));
		pTable->AddProperty(
			new MP4Integer64Property("mediaTime"));
		pTable->AddProperty(
			new MP4Integer16Property("mediaRate"));
		pTable->AddProperty(
			new MP4Integer16Property("reserved"));
	}
	void Generate() {
		SetVersion(0);
		AddVersion0Properties();
	}
	void Read() {
		/* read atom version */
		ReadProperties(0, 1);

		/* need to create the properties based on the atom version */
		if (GetVersion() == 1) {
			AddVersion1Properties();
		} else {
			AddVersion0Properties();
		}

		/* now we can read the remaining properties */
		ReadProperties(1);

		Skip();	// to end of atom
	}
};

class MP4UdtaAtom : public MP4Atom {
public:
	MP4UdtaAtom() : MP4Atom("udta") {
		ExpectChildAtom("cprt", Optional, Many);
		ExpectChildAtom("hnti", Optional, OnlyOne);
	}

	void Read() {
		if (ATOMID(m_pParentAtom->GetType()) == ATOMID("trak")) {
			ExpectChildAtom("hinf", Optional, OnlyOne);
		}

		MP4Atom::Read();
	}
};

class MP4CprtAtom : public MP4Atom {
public:
	MP4CprtAtom() : MP4Atom("cprt") {
		AddVersionAndFlags();
		AddProperty(
			new MP4Integer16Property("language"));
		AddProperty(
			new MP4StringProperty("notice"));
	}
};

class MP4HntiAtom : public MP4Atom {
public:
	MP4HntiAtom() : MP4Atom("hnti") {
	}
	void Read() {
		MP4Atom* grandParent = m_pParentAtom->GetParentAtom();
		ASSERT(grandParent);
		if (ATOMID(grandParent->GetType()) == ATOMID("trak")) {
			ExpectChildAtom("sdp ", Optional, OnlyOne);
		} else {
			ExpectChildAtom("rtp ", Optional, OnlyOne);
		}

		MP4Atom::Read();
	}
};

class MP4RtpAtom : public MP4Atom {
public:
	MP4RtpAtom() : MP4Atom("rtp ") {
		AddProperty(
			new MP4Integer32Property("descriptionFormat"));
		AddProperty(
			new MP4StringProperty("sdpText"));
	}
	void Read() {
		ReadProperties(0, 1);

		/* read sdp string, length is implicit in size of atom */
		u_int64_t size = m_end - m_pFile->GetPosition();
		char* data = (char*)MP4Malloc(size + 1);
		m_pFile->ReadBytes((u_int8_t*)data, size);
		data[size] = '\0';
		((MP4StringProperty*)m_pProperties[1])->SetValue(data);
		MP4Free(data);
	}
};

class MP4SdpAtom : public MP4Atom {
public:
	MP4SdpAtom() : MP4Atom("sdp ") {
		AddProperty(
			new MP4StringProperty("sdpText"));
	}
	void Read() {
		/* read sdp string, length is implicit in size of atom */
		u_int64_t size = m_end - m_pFile->GetPosition();
		char* data = (char*)MP4Malloc(size + 1);
		m_pFile->ReadBytes((u_int8_t*)data, size);
		data[size] = '\0';
		((MP4StringProperty*)m_pProperties[0])->SetValue(data);
		MP4Free(data);
	}
};

class MP4HinfAtom : public MP4Atom {
public:
	MP4HinfAtom() : MP4Atom("hinf") {
		ExpectChildAtom("trpy", Optional, OnlyOne);
		ExpectChildAtom("nump", Optional, OnlyOne);
		ExpectChildAtom("tpyl", Optional, OnlyOne);
		ExpectChildAtom("maxr", Optional, Many);
		ExpectChildAtom("dmed", Optional, OnlyOne);
		ExpectChildAtom("dimm", Optional, OnlyOne);
		ExpectChildAtom("drep", Optional, OnlyOne);
		ExpectChildAtom("tmin", Optional, OnlyOne);
		ExpectChildAtom("tmax", Optional, OnlyOne);
		ExpectChildAtom("pmax", Optional, OnlyOne);
		ExpectChildAtom("dmax", Optional, OnlyOne);
		ExpectChildAtom("payt", Optional, OnlyOne);
	}
};

class MP4TrpyAtom : public MP4Atom {
public:
	MP4TrpyAtom() : MP4Atom("trpy") {
		AddProperty( // bytes sent including RTP headers
			new MP4Integer64Property("bytes"));
	}
};

class MP4NumpAtom : public MP4Atom {
public:
	MP4NumpAtom() : MP4Atom("nump") {
		AddProperty( // packets sent
			new MP4Integer64Property("packets"));
	}
};

class MP4TpylAtom : public MP4Atom {
public:
	MP4TpylAtom() : MP4Atom("tpyl") {
		AddProperty( // bytes sent of RTP payload data
			new MP4Integer64Property("bytes"));
	}
};

class MP4MaxrAtom : public MP4Atom {
public:
	MP4MaxrAtom() : MP4Atom("maxr") {
		AddProperty(
			new MP4Integer32Property("granularity"));
		AddProperty(
			new MP4Integer32Property("maxBitRate"));
	}
};

class MP4DmedAtom : public MP4Atom {
public:
	MP4DmedAtom() : MP4Atom("dmed") {
		AddProperty( // bytes sent from media data
			new MP4Integer64Property("bytes"));
	}
};

class MP4DimmAtom : public MP4Atom {
public:
	MP4DimmAtom() : MP4Atom("dimm") {
		AddProperty( // bytes of immediate data
			new MP4Integer64Property("bytes"));
	}
};

class MP4DrepAtom : public MP4Atom {
public:
	MP4DrepAtom() : MP4Atom("drep") {
		AddProperty( // bytes of repeated data
			new MP4Integer64Property("bytes"));
	}
};

class MP4TminAtom : public MP4Atom {
public:
	MP4TminAtom() : MP4Atom("tmin") {
		AddProperty( // min relative xmit time
			new MP4Integer32Property("milliSecs"));
	}
};

class MP4TmaxAtom : public MP4Atom {
public:
	MP4TmaxAtom() : MP4Atom("tmax") {
		AddProperty( // max relative xmit time
			new MP4Integer32Property("milliSecs"));
	}
};

class MP4PmaxAtom : public MP4Atom {
public:
	MP4PmaxAtom() : MP4Atom("pmax") {
		AddProperty( // max packet size 
			new MP4Integer32Property("bytes"));
	}
};

class MP4DmaxAtom : public MP4Atom {
public:
	MP4DmaxAtom() : MP4Atom("dmax") {
		AddProperty( // max packet duration 
			new MP4Integer32Property("milliSecs"));
	}
};

class MP4PaytAtom : public MP4Atom {
public:
	MP4PaytAtom() : MP4Atom("payt") {
		AddProperty( 
			new MP4Integer32Property("payloadNumber"));
		AddProperty( 
			new MP4StringProperty("rtpMap", Counted));
	}
};

class MP4FreeAtom : public MP4Atom {
public:
	MP4FreeAtom() : MP4Atom("free") { }

	void Read() {
		Skip();
	}
};

MP4Atom* MP4Atom::CreateAtom(char* type)
{
	MP4Atom* pAtom = NULL;

	if (type == NULL) {
		pAtom = new MP4RootAtom();
	} else if (ATOMID(type) == ATOMID("ftyp")) {
		pAtom = new MP4FtypAtom();
	} else if (ATOMID(type) == ATOMID("mdat")) {
		pAtom = new MP4MdatAtom();
	} else if (ATOMID(type) == ATOMID("moov")) {
		pAtom = new MP4MoovAtom();
	} else if (ATOMID(type) == ATOMID("mvhd")) {
		pAtom = new MP4MvhdAtom();
	} else if (ATOMID(type) == ATOMID("iods")) {
		pAtom = new MP4IodsAtom();
	} else if (ATOMID(type) == ATOMID("trak")) {
		pAtom = new MP4TrakAtom();
	} else if (ATOMID(type) == ATOMID("tkhd")) {
		pAtom = new MP4TkhdAtom();
	} else if (ATOMID(type) == ATOMID("tref")) {
		pAtom = new MP4TrefAtom();
	} else if (ATOMID(type) == ATOMID("edts")) {
		pAtom = new MP4EdtsAtom();
	} else if (ATOMID(type) == ATOMID("elst")) {
		pAtom = new MP4ElstAtom();
	} else if (ATOMID(type) == ATOMID("mdia")) {
		pAtom = new MP4MdiaAtom();
	} else if (ATOMID(type) == ATOMID("mdhd")) {
		pAtom = new MP4MdhdAtom();
	} else if (ATOMID(type) == ATOMID("hdlr")) {
		pAtom = new MP4HdlrAtom();
	} else if (ATOMID(type) == ATOMID("minf")) {
		pAtom = new MP4MinfAtom();
	} else if (ATOMID(type) == ATOMID("vmhd")) {
		pAtom = new MP4VmhdAtom();
	} else if (ATOMID(type) == ATOMID("smhd")) {
		pAtom = new MP4SmhdAtom();
	} else if (ATOMID(type) == ATOMID("hmhd")) {
		pAtom = new MP4HmhdAtom();
	} else if (ATOMID(type) == ATOMID("nmhd")) {
		pAtom = new MP4NmhdAtom();
	} else if (ATOMID(type) == ATOMID("dinf")) {
		pAtom = new MP4DinfAtom();
	} else if (ATOMID(type) == ATOMID("dref")) {
		pAtom = new MP4DrefAtom();
	} else if (ATOMID(type) == ATOMID("url ")) {
		pAtom = new MP4UrlAtom();
	} else if (ATOMID(type) == ATOMID("urn ")) {
		pAtom = new MP4UrnAtom();
	} else if (ATOMID(type) == ATOMID("stbl")) {
		pAtom = new MP4StblAtom();
	} else if (ATOMID(type) == ATOMID("stsd")) {
		pAtom = new MP4StsdAtom();
	} else if (ATOMID(type) == ATOMID("mp4a")) {
		pAtom = new MP4Mp4aAtom();
	} else if (ATOMID(type) == ATOMID("mp4s")) {
		pAtom = new MP4Mp4sAtom();
	} else if (ATOMID(type) == ATOMID("mp4v")) {
		pAtom = new MP4Mp4vAtom();
	} else if (ATOMID(type) == ATOMID("esds")) {
		pAtom = new MP4EsdsAtom();
	} else if (ATOMID(type) == ATOMID("stts")) {
		pAtom = new MP4SttsAtom();
	} else if (ATOMID(type) == ATOMID("ctts")) {
		pAtom = new MP4CttsAtom();
	} else if (ATOMID(type) == ATOMID("stsz")) {
		pAtom = new MP4StszAtom();
	} else if (ATOMID(type) == ATOMID("stsc")) {
		pAtom = new MP4StscAtom();
	} else if (ATOMID(type) == ATOMID("stco")) {
		pAtom = new MP4StcoAtom();
	} else if (ATOMID(type) == ATOMID("co64")) {
		pAtom = new MP4Co64Atom();
	} else if (ATOMID(type) == ATOMID("stss")) {
		pAtom = new MP4StssAtom();
	} else if (ATOMID(type) == ATOMID("stsh")) {
		pAtom = new MP4StshAtom();
	} else if (ATOMID(type) == ATOMID("stdp")) {
		pAtom = new MP4StdpAtom();
	} else if (ATOMID(type) == ATOMID("udta")) {
		pAtom = new MP4UdtaAtom();
	} else if (ATOMID(type) == ATOMID("cprt")) {
		pAtom = new MP4CprtAtom();
	} else if (ATOMID(type) == ATOMID("hnti")) {
		pAtom = new MP4HntiAtom();
	} else if (ATOMID(type) == ATOMID("rtp ")) {
		pAtom = new MP4RtpAtom();
	} else if (ATOMID(type) == ATOMID("sdp ")) {
		pAtom = new MP4SdpAtom();
	} else if (ATOMID(type) == ATOMID("hinf")) {
		pAtom = new MP4HinfAtom();
	} else if (ATOMID(type) == ATOMID("trpy")) {
		pAtom = new MP4TrpyAtom();
	} else if (ATOMID(type) == ATOMID("nump")) {
		pAtom = new MP4NumpAtom();
	} else if (ATOMID(type) == ATOMID("tpyl")) {
		pAtom = new MP4TpylAtom();
	} else if (ATOMID(type) == ATOMID("maxr")) {
		pAtom = new MP4MaxrAtom();
	} else if (ATOMID(type) == ATOMID("dmed")) {
		pAtom = new MP4DmedAtom();
	} else if (ATOMID(type) == ATOMID("dimm")) {
		pAtom = new MP4DimmAtom();
	} else if (ATOMID(type) == ATOMID("drep")) {
		pAtom = new MP4DrepAtom();
	} else if (ATOMID(type) == ATOMID("tmin")) {
		pAtom = new MP4TminAtom();
	} else if (ATOMID(type) == ATOMID("tmax")) {
		pAtom = new MP4TmaxAtom();
	} else if (ATOMID(type) == ATOMID("pmax")) {
		pAtom = new MP4PmaxAtom();
	} else if (ATOMID(type) == ATOMID("dmax")) {
		pAtom = new MP4DmaxAtom();
	} else if (ATOMID(type) == ATOMID("payt")) {
		pAtom = new MP4PaytAtom();
	} else if (ATOMID(type) == ATOMID("free")) {
		pAtom = new MP4FreeAtom();
	} else if (ATOMID(type) == ATOMID("skip")) {
		pAtom = new MP4FreeAtom();
		pAtom->SetType("skip");
	} else {
		pAtom = new MP4Atom(type);
		pAtom->SetUnknownType(true);
	}

	return pAtom;
}

