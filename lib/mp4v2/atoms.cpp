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

#define Required	true
#define Optional	false
#define OnlyOne		true
#define Many		false

class MP4MdatAtom : public MP4Atom {
public:
	MP4MdatAtom() : MP4Atom("mdat") { }

	void Read() {
		Skip();
	}
	void Write() {
		// TBD mdat atom with 64 bit size 
		MP4Atom::Write();
	}
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
	MP4MvhdAtom() : MP4Atom("mvhd") { }
};

class MP4IodsAtom : public MP4Atom {
public:
	MP4IodsAtom() : MP4Atom("iods") {
		AddVersionAndFlags();
		AddProperty(new MP4IODescriptorProperty());
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
};

class MP4TrefAtom : public MP4Atom {
public:
	MP4TrefAtom() : MP4Atom("tref") {
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

	void Read() {
		/* read atom version */
		m_pProperties[0]->Read(m_pFile);
		u_int8_t version = 
			((MP4IntegerProperty<u_int8_t>*)m_pProperties[0])->GetValue();

		/* need to create the properties based on the atom version */
		if (version == 1) {
			AddProperty(
				new MP4IntegerProperty<u_int64_t>("creationTime"));
			AddProperty(
				new MP4IntegerProperty<u_int64_t>("modificationTime"));
			AddProperty(
				new MP4IntegerProperty<u_int32_t>("timeScale"));
			AddProperty(
				new MP4IntegerProperty<u_int64_t>("duration"));
		} else {
			AddProperty(
				new MP4IntegerProperty<u_int32_t>("creationTime"));
			AddProperty(
				new MP4IntegerProperty<u_int32_t>("modificationTime"));
			AddProperty(
				new MP4IntegerProperty<u_int32_t>("timeScale"));
			AddProperty(
				new MP4IntegerProperty<u_int32_t>("duration"));
		}
		AddProperty(
			new MP4IntegerProperty<u_int16_t>("language"));
		AddReserved("reserved", 2);

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
			new MP4IntegerProperty<u_int32_t>("handlerType"));
		AddReserved("reserved2", 12);
		// ??? QT says Pascal string, ISO says C string ???
		AddProperty(
			new MP4StringProperty("name"));
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
			new MP4IntegerProperty<u_int16_t>("maxPduSize")); 
		AddProperty(	
			new MP4IntegerProperty<u_int16_t>("avgPduSize")); 
		AddProperty(	
			new MP4IntegerProperty<u_int32_t>("maxBitRate")); 
		AddProperty(	
			new MP4IntegerProperty<u_int32_t>("avgBitRate")); 
		AddProperty(	
			new MP4IntegerProperty<u_int32_t>("slidingAvgBitRate")); 
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

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		pCount->SetReadOnly();
		AddProperty(pCount);

		ExpectChildAtom("url ", Optional, Many);
		ExpectChildAtom("urn ", Optional, Many);
	}

	void Read() {
		/* do the usual read */
		MP4Atom::Read();

		// check that number of children == entryCount
		MP4IntegerProperty<u_int32_t>* pCount = 
			(MP4IntegerProperty<u_int32_t>*)m_pProperties[2];

		if (m_pChildAtoms.size() != pCount->GetValue()) {
			VERBOSE_READ(GetVerbosity(),
				printf("Warning: dref inconsistency with number of entries"));

			/* fix it */
			pCount->SetReadOnly(false);
			pCount->SetValue(m_pChildAtoms.size());
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
};

class MP4UrnAtom : public MP4Atom {
public:
	MP4UrnAtom() : MP4Atom("urn ") {
		AddVersionAndFlags();
		AddProperty(new MP4StringProperty("name"));
		AddProperty(new MP4StringProperty("location"));
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

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
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
		MP4IntegerProperty<u_int32_t>* pCount = 
			(MP4IntegerProperty<u_int32_t>*)m_pProperties[2];

		if (m_pChildAtoms.size() != pCount->GetValue()) {
			VERBOSE_READ(GetVerbosity(),
				printf("Warning: stsd inconsistency with number of entries"));

			/* fix it */
			pCount->SetReadOnly(false);
			pCount->SetValue(m_pChildAtoms.size());
			pCount->SetReadOnly(true);
		}
	}
};

class MP4Mp4aAtom : public MP4Atom {
public:
	MP4Mp4aAtom() : MP4Atom("mp4a") {
		AddReserved("reserved1", 6);
		AddProperty(
			new MP4IntegerProperty<u_int16_t>("dataReferenceIndex"));
		AddReserved("reserved2", 20);
		ExpectChildAtom("esds", Required, OnlyOne);
	}
};

class MP4Mp4sAtom : public MP4Atom {
public:
	MP4Mp4sAtom() : MP4Atom("mp4s") {
		AddReserved("reserved1", 6);
		AddProperty(
			new MP4IntegerProperty<u_int16_t>("dataReferenceIndex"));
		ExpectChildAtom("esds", Required, OnlyOne);
	}
};

class MP4Mp4vAtom : public MP4Atom {
public:
	MP4Mp4vAtom() : MP4Atom("mp4v") {
		AddReserved("reserved1", 6);
		AddProperty(
			new MP4IntegerProperty<u_int16_t>("dataReferenceIndex"));
		AddReserved("reserved2", 70);
		ExpectChildAtom("esds", Required, OnlyOne);
	}
};

class MP4EsdsAtom : public MP4Atom {
public:
	MP4EsdsAtom() : MP4Atom("esds") {
		AddVersionAndFlags();
		AddProperty(new MP4ESDescriptorProperty());
	}
};

class MP4SttsAtom : public MP4Atom {
public:
	MP4SttsAtom() : MP4Atom("stts") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleCount"));
		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleDelta"));
	}
};

class MP4CttsAtom : public MP4Atom {
public:
	MP4CttsAtom() : MP4Atom("ctts") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleCount"));
		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleOffset"));
	}
};

class MP4StszAtom : public MP4Atom {
public:
	MP4StszAtom() : MP4Atom("stsz") {
		AddVersionAndFlags();

		AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleSize")); 

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("sampleCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleSize"));
	}
};

class MP4StscAtom : public MP4Atom {
public:
	MP4StscAtom() : MP4Atom("stsc") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("firstChunk"));
		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("samplesPerChunk"));
		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleDescriptionIndex"));
	}
};

class MP4StcoAtom : public MP4Atom {
public:
	MP4StcoAtom() : MP4Atom("stco") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("chunkOffset"));
	}
};

class MP4Co64Atom : public MP4Atom {
public:
	MP4Co64Atom() : MP4Atom("co64") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int64_t>("chunkOffset"));
	}
};

class MP4StssAtom : public MP4Atom {
public:
	MP4StssAtom() : MP4Atom("stss") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("sampleNumber"));
	}
};

class MP4StshAtom : public MP4Atom {
public:
	MP4StshAtom() : MP4Atom("stsh") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("shadowedSampleNumber"));
		pTable->AddProperty(
			new MP4IntegerProperty<u_int32_t>("syncSampleNumber"));
	}
};

class MP4StdpAtom : public MP4Atom {
public:
	MP4StdpAtom() : MP4Atom("stdp") {
		AddVersionAndFlags();

		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		pCount->SetImplicit();
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		pTable->AddProperty(
			new MP4IntegerProperty<u_int16_t>("priority"));
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
	}

	void Read() {
		/* read atom version */
		m_pProperties[0]->Read(m_pFile);
		u_int8_t version = 
			((MP4IntegerProperty<u_int8_t>*)m_pProperties[0])->GetValue();

		/* need to create the properties based on the atom version */
		MP4IntegerProperty<u_int32_t>* pCount = 
			new MP4IntegerProperty<u_int32_t>("entryCount"); 
		AddProperty(pCount);

		MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
		AddProperty(pTable);

		if (version == 1) {
			pTable->AddProperty(
				new MP4IntegerProperty<u_int64_t>("segmentDuration"));
			pTable->AddProperty(
				new MP4IntegerProperty<u_int64_t>("mediaTime"));
		} else {
			pTable->AddProperty(
				new MP4IntegerProperty<u_int32_t>("segmentDuration"));
			pTable->AddProperty(
				new MP4IntegerProperty<u_int32_t>("mediaTime"));
		}
		pTable->AddProperty(
			new MP4IntegerProperty<u_int16_t>("mediaRate"));
		pTable->AddProperty(
			new MP4IntegerProperty<u_int16_t>("reserved"));

		/* now we can read the properties */
		ReadProperties(1);

		Skip();	// to end of atom
	}
};

class MP4UdtaAtom : public MP4Atom {
public:
	MP4UdtaAtom() : MP4Atom("udta") {
		ExpectChildAtom("cprt", Optional, Many);
	}

	void Read() {
		if (ATOMID(m_pParentAtom->GetType()) == ATOMID("trak")) {
			ExpectChildAtom("hinf", Optional, OnlyOne);
			ExpectChildAtom("hnti", Optional, OnlyOne);
		}

		MP4Atom::Read();
	}
};

class MP4CprtAtom : public MP4Atom {
public:
	MP4CprtAtom() : MP4Atom("cprt") {
		AddVersionAndFlags();
		AddProperty(
			new MP4IntegerProperty<u_int16_t>("language"));
		AddProperty(
			new MP4StringProperty("notice"));
	}
};

class MP4HntiAtom : public MP4Atom {
public:
	MP4HntiAtom() : MP4Atom("hnti") {
		ExpectChildAtom("sdp ", Optional, OnlyOne);
	}
};

class MP4SdpAtom : public MP4Atom {
public:
	MP4SdpAtom() : MP4Atom("sdp ") {
		AddProperty(
			new MP4StringProperty("string"));
	}
	void Read() {
		/* read sdp string */
		char* data = (char*)MP4Malloc(m_size + 1);
		m_pFile->ReadBytes((u_int8_t*)data, m_size);
		data[m_size] = '\0';
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
			new MP4IntegerProperty<u_int64_t>("bytes"));
	}
};

class MP4NumpAtom : public MP4Atom {
public:
	MP4NumpAtom() : MP4Atom("nump") {
		AddProperty( // packets sent
			new MP4IntegerProperty<u_int64_t>("packets"));
	}
};

class MP4TpylAtom : public MP4Atom {
public:
	MP4TpylAtom() : MP4Atom("tpyl") {
		AddProperty( // bytes sent of RTP payload data
			new MP4IntegerProperty<u_int64_t>("bytes"));
	}
};

class MP4MaxrAtom : public MP4Atom {
public:
	MP4MaxrAtom() : MP4Atom("maxr") {
		AddProperty(
			new MP4IntegerProperty<u_int32_t>("granularity"));
		AddProperty(
			new MP4IntegerProperty<u_int32_t>("maxBitRate"));
	}
};

class MP4DmedAtom : public MP4Atom {
public:
	MP4DmedAtom() : MP4Atom("dmed") {
		AddProperty( // bytes sent from media data
			new MP4IntegerProperty<u_int64_t>("bytes"));
	}
};

class MP4DimmAtom : public MP4Atom {
public:
	MP4DimmAtom() : MP4Atom("dimm") {
		AddProperty( // bytes of immediate data
			new MP4IntegerProperty<u_int64_t>("bytes"));
	}
};

class MP4DrepAtom : public MP4Atom {
public:
	MP4DrepAtom() : MP4Atom("drep") {
		AddProperty( // bytes of repeated data
			new MP4IntegerProperty<u_int64_t>("bytes"));
	}
};

class MP4TminAtom : public MP4Atom {
public:
	MP4TminAtom() : MP4Atom("tmin") {
		AddProperty( // min relative xmit time
			new MP4IntegerProperty<u_int32_t>("milliSecs"));
	}
};

class MP4TmaxAtom : public MP4Atom {
public:
	MP4TmaxAtom() : MP4Atom("tmax") {
		AddProperty( // max relative xmit time
			new MP4IntegerProperty<u_int32_t>("milliSecs"));
	}
};

class MP4PmaxAtom : public MP4Atom {
public:
	MP4PmaxAtom() : MP4Atom("pmax") {
		AddProperty( // max packet size 
			new MP4IntegerProperty<u_int32_t>("bytes"));
	}
};

class MP4DmaxAtom : public MP4Atom {
public:
	MP4DmaxAtom() : MP4Atom("dmax") {
		AddProperty( // max packet duration 
			new MP4IntegerProperty<u_int32_t>("milliSecs"));
	}
};

class MP4PaytAtom : public MP4Atom {
public:
	MP4PaytAtom() : MP4Atom("payt") {
		AddProperty( 
			new MP4IntegerProperty<u_int32_t>("payloadNumber"));
		AddProperty( 
			new MP4StringProperty("rtpMap", true));
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

	if (ATOMID(type) == ATOMID("mdat")) {
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
		pAtom->SetUnknownType();
	}

	return pAtom;
}

