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

#ifndef __RTPHINT_INCLUDED__
#define __RTPHINT_INCLUDED__

// LATER Read functionality

class MP4RtpData : public MP4Container {
public:
	MP4RtpData();
};

MP4ARRAY_DECL(MP4RtpData, MP4RtpData*)

class MP4RtpImmediateData : public MP4RtpData {
public:
	MP4RtpImmediateData();

	void Set(const u_int8_t* pBytes, u_int8_t numBytes);
};

class MP4RtpSampleData : public MP4RtpData {
public:
	MP4RtpSampleData();

	void Set(MP4SampleId sampleId,
		u_int32_t sampleOffset, u_int16_t sampleLength);
};

class MP4RtpSampleDescriptionData : public MP4RtpData {
public:
	MP4RtpSampleDescriptionData();
};

class MP4RtpPacket : public MP4Container {
public:
	MP4RtpPacket();

	~MP4RtpPacket();

	void Set(u_int8_t payloadNumber, u_int32_t packetId, bool setMbit);

	void SetBframe(bool isBframe);

	void SetTimestampOffset(u_int32_t timestampOffset);

	void AddData(MP4RtpData* pData);

	void Write(MP4File* pFile);

	void Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits);

protected:
	MP4RtpDataArray		m_rtpData;
};

MP4ARRAY_DECL(MP4RtpPacket, MP4RtpPacket*)

class MP4RtpHint : public MP4Container {
public:
	MP4RtpHint();

	~MP4RtpHint();

	void Set(bool isBframe, u_int32_t timestampOffset);

	MP4RtpPacket* AddPacket();

	MP4RtpPacket* GetCurrentPacket() {
		if (m_rtpPackets.Size() == 0) {
			return NULL;
		}
		return m_rtpPackets[m_rtpPackets.Size() - 1];
	}

	void Write(MP4File* pFile);

	void Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits);

protected:
	bool 				m_isBframe;
	u_int32_t 			m_timestampOffset;
	MP4RtpPacketArray	m_rtpPackets;
};

class MP4RtpHintTrack : public MP4Track {
public:
	MP4RtpHintTrack(MP4File* pFile, MP4Atom* pTrakAtom);

	~MP4RtpHintTrack();

	void InitRefTrack();

	void InitStats();

	void SetPayload(
		const char* payloadName,
		u_int8_t payloadNumber,
		u_int16_t maxPayloadSize);

	void AddHint(bool isBframe, u_int32_t timestampOffset);

	void AddPacket(bool setMbit);

	void AddImmediateData(const u_int8_t* pBytes, u_int32_t numBytes);

	void AddSampleData(MP4SampleId sampleId,
		 u_int32_t dataOffset, u_int32_t dataLength);

	void WriteHint(MP4Duration duration, bool isSyncSample);

	void FinishWrite();

protected:
	MP4Track*	m_pRefTrack;

	char*		m_payloadName;
	u_int8_t	m_payloadNumber;
	u_int32_t	m_maxPayloadSize;

	MP4SampleId	m_hintId;
	MP4RtpHint*	m_pHint;

	u_int32_t	m_packetId;

	// statistics
	// in trak.udta.hinf
	MP4Integer64Property*	m_pTrpy;
	MP4Integer64Property*	m_pNump;
	MP4Integer64Property*	m_pTpyl;
	MP4Integer64Property*	m_pMaxr;
	MP4Integer64Property*	m_pDmed;
	MP4Integer64Property*	m_pDimm;
	MP4Integer32Property*	m_pPmax;
	MP4Integer32Property*	m_pDmax;

	// in trak.mdia.minf.hmhd
	MP4Integer16Property*	m_pMaxPdu;
	MP4Integer16Property*	m_pAvgPdu;
	MP4Integer32Property*	m_pMaxBitRate;
	MP4Integer32Property*	m_pAvgBitRate;

	MP4Timestamp			m_thisSec;
	u_int32_t				m_bytesThisSec;
	u_int32_t				m_bytesThisHint;
	u_int32_t				m_bytesThisPacket;
};

#endif /* __RTPHINT_INCLUDED__ */
