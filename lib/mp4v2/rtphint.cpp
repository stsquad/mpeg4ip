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

/* rtp hint track operations */

MP4RtpHintTrack::MP4RtpHintTrack(MP4File* pFile, MP4Atom* pTrakAtom)
	: MP4Track(pFile, pTrakAtom)
{
	m_pRefTrack = NULL;

	m_payloadName = NULL;
	m_payloadNumber = 0;
	m_maxPayloadSize = 1460;

	m_hintId = MP4_INVALID_SAMPLE_ID;
	m_pHint = NULL;
	m_packetId = 0;

	m_pTrpy = NULL;
	m_pNump = NULL;
	m_pTpyl = NULL;
	m_pMaxr = NULL;
	m_pDmed = NULL;
	m_pDimm = NULL;
	m_pPmax = NULL;
	m_pDmax = NULL;

	m_pMaxPdu = NULL;
	m_pAvgPdu = NULL;
	m_pMaxBitRate = NULL;
	m_pAvgBitRate = NULL;

	m_thisSec = 0;
	m_bytesThisSec = 0;
	m_bytesThisHint = 0;
	m_bytesThisPacket = 0;
}

MP4RtpHintTrack::~MP4RtpHintTrack()
{
	MP4Free(m_payloadName);
	delete m_pHint;
}

void MP4RtpHintTrack::InitRefTrack()
{
	if (!m_pRefTrack) {
		MP4Integer32Property* pRefTrackIdProperty = NULL;
		m_pTrakAtom->FindProperty("trak.tref.hint.entries[0].trackId",
			(MP4Property**)&pRefTrackIdProperty);
		ASSERT(pRefTrackIdProperty);

		m_pRefTrack = m_pFile->GetTrack(pRefTrackIdProperty->GetValue());
	}
}

void MP4RtpHintTrack::SetPayload(
	const char* payloadName,
	u_int8_t payloadNumber,
	u_int16_t maxPayloadSize)
{
	InitRefTrack();

	MP4Free(m_payloadName);
	m_payloadName = MP4Stralloc(payloadName);

	m_payloadNumber = payloadNumber;

	if (maxPayloadSize) {
		m_maxPayloadSize = maxPayloadSize;
	}

	char* rtpMapBuf = (char*)MP4Malloc(strlen(payloadName) + 16);
	sprintf(rtpMapBuf, "%s/%u", payloadName, GetTimeScale());
	
	ASSERT(m_pRefTrack);

	// set sdp media type
	const char* sdpMediaType;
	if (!strcmp(m_pRefTrack->GetType(), MP4_AUDIO_TRACK_TYPE)) {
		sdpMediaType = "audio";
	} else if (!strcmp(m_pRefTrack->GetType(), MP4_VIDEO_TRACK_TYPE)) {
		sdpMediaType = "video";
	} else {
		sdpMediaType = "application";
	}

	char* sdpBuf = (char*)MP4Malloc(
		strlen(sdpMediaType) + strlen(rtpMapBuf) + 256);
	sprintf(sdpBuf, 
		"m=%s 0 RTP/AVP %u\015\012"
		"a=rtpmap:%u %s\015\012"
		"a=control:trackID=%u\015\012" 
		"a=mpeg4-esid:%u\015\012",
		sdpMediaType, payloadNumber,
		payloadNumber, rtpMapBuf,
		m_trackId,
		m_pRefTrack->GetId());

	MP4Integer32Property* pPayloadProperty = NULL;
	m_pTrakAtom->FindProperty("trak.udta.hinf.payt.payloadNumber",
		(MP4Property**)&pPayloadProperty);
	ASSERT(pPayloadProperty);
	pPayloadProperty->SetValue(payloadNumber);

	MP4StringProperty* pRtpMapProperty = NULL;
	m_pTrakAtom->FindProperty("trak.udta.hinf.payt.rtpMap",
		(MP4Property**)&pRtpMapProperty);
	ASSERT(pRtpMapProperty);
	pRtpMapProperty->SetValue(rtpMapBuf);

	MP4StringProperty* pSdpProperty = NULL;
	m_pTrakAtom->FindProperty("trak.udta.hnti.sdp .sdpText",
		(MP4Property**)&pSdpProperty);
	ASSERT(pSdpProperty);
	pSdpProperty->SetValue(sdpBuf);

	MP4Integer32Property* pMaxPacketProperty = NULL;
	m_pTrakAtom->FindProperty("trak.mdia.minf.stbl.stsd.rtp .maxPacketSize",
		(MP4Property**)&pMaxPacketProperty);
	ASSERT(pMaxPacketProperty);
	pMaxPacketProperty->SetValue(m_maxPayloadSize);

	// cleanup
	MP4Free(rtpMapBuf);
	MP4Free(sdpBuf);
}

void MP4RtpHintTrack::AddHint(bool isBframe, u_int32_t timestampOffset)
{
	// on first hint, need to lookup the reference track
	if (m_hintId == MP4_INVALID_SAMPLE_ID) {
		InitRefTrack();
		InitStats();
	}

	if (m_pHint) {
		throw new MP4Error("unwritten hint is still pending", "MP4AddRtpHint");
	}

	m_pHint = new MP4RtpHint();
	m_pHint->Set(isBframe, timestampOffset);
	m_bytesThisHint = 0;
	m_hintId++;
}

void MP4RtpHintTrack::AddPacket(bool setMbit)
{
	if (m_pHint == NULL) {
		throw new MP4Error("no hint pending", "MP4RtpAddPacket");
	}

	MP4RtpPacket* pPacket = m_pHint->AddPacket();

	pPacket->Set(m_payloadNumber, m_packetId++, setMbit);

	m_bytesThisHint += 12;
	if (m_bytesThisPacket > m_pPmax->GetValue()) {
		m_pPmax->SetValue(m_bytesThisPacket);
	}
	m_bytesThisPacket = 12;
	m_pNump->IncrementValue();
	m_pTrpy->IncrementValue(12); // RTP packet header size
}

void MP4RtpHintTrack::AddImmediateData(
	const u_int8_t* pBytes,
	u_int32_t numBytes)
{
	if (m_pHint == NULL) {
		throw new MP4Error("no hint pending", "MP4RtpAddImmediateData");
	}

	MP4RtpPacket* pPacket = m_pHint->GetCurrentPacket();
	if (pPacket == NULL) {
		throw new MP4Error("no packet pending", "MP4RtpAddImmediateData");
	}

	if (pBytes == NULL || numBytes == 0) {
		throw new MP4Error("no data",
			"AddImmediateData");
	}
	if (numBytes > 14) {
		throw new MP4Error("data size is larger than 14 bytes",
			"AddImmediateData");
	}

	MP4RtpImmediateData* pData = new MP4RtpImmediateData();
	pData->Set(pBytes, numBytes);

	pPacket->AddData(pData);

	m_bytesThisHint += numBytes;
	m_bytesThisPacket += numBytes;
	m_pDimm->IncrementValue(numBytes);
	m_pTpyl->IncrementValue(numBytes);
	m_pTrpy->IncrementValue(numBytes);
}

void MP4RtpHintTrack::AddSampleData(
	MP4SampleId sampleId,
	u_int32_t dataOffset,
	u_int32_t dataLength)
{
	if (m_pHint == NULL) {
		throw new MP4Error("no hint pending", "MP4RtpAddSampleData");
	}

	MP4RtpPacket* pPacket = m_pHint->GetCurrentPacket();
	if (pPacket == NULL) {
		throw new MP4Error("no packet pending", "MP4RtpAddSampleData");
	}

	MP4RtpSampleData* pData = new MP4RtpSampleData();

	pData->Set(sampleId, dataOffset, dataLength);

	pPacket->AddData(pData);

	m_bytesThisHint += dataLength;
	m_bytesThisPacket += dataLength;
	m_pDmed->IncrementValue(dataLength);
	m_pTpyl->IncrementValue(dataLength);
	m_pTrpy->IncrementValue(dataLength);
}

void MP4RtpHintTrack::AddESConfigurationPacket()
{
#ifdef NOTDEF
	if (m_pHint == NULL) {
		throw new MP4Error("no hint pending", 
			"MP4RtpAddESConfigurationPacket");
	}

	u_int8_t pConfig = NULL;
	u_int32_t configSize = 0;

	m_pFile->GetTrackESConfiguration(m_pRefTrack->GetTrackId(),
		&pConfig, &configSize);

	if (pConfig == NULL) {
		return;
	}

	if (configSize > m_maxPayloadSize) {
		throw new MP4Error("ES configuration is too large for RTP payload",
			"MP4RtpAddESConfigurationPacket");
	}

	AddPacket(true);

	MP4RtpPacket* pPacket = m_pHint->GetCurrentPacket();
	ASSERT(pPacket);
	
	// This is ugly!
	// To get the ES configuration data somewhere known
	// we create a sample data reference that points to 
	// this hint track (not the media track)
	// and this sample of the hint track 
	// the offset into this sample needs to be filled in
	// when WriteHint is called
	MP4RtpSampleData* pData = new MP4RtpSampleData();
	pData->SetTrackReference((u_int8_t)-1);
	pData->Set(m_writeSampleId, 0, configSize);

	pPacket->AddData(pData);

	m_bytesThisHint += configSize;
	m_bytesThisPacket += configSize;
	m_pTpyl->IncrementValue(configSize);
	m_pTrpy->IncrementValue(configSize);
#endif
}

void MP4RtpHintTrack::WriteHint(MP4Duration duration, bool isSyncSample)
{
	if (m_pHint == NULL) {
		throw new MP4Error("no hint pending", "MP4WriteRtpHint");
	}

	u_int8_t* pBytes;
	u_int8_t* writeBuffer;
	u_int64_t numBytes;

	m_pFile->EnableWriteBuffer();
	m_pHint->Write(m_pFile);
	m_pFile->GetWriteBuffer(&writeBuffer, &numBytes);
	pBytes = (u_int8_t*)MP4Malloc(numBytes);
	memcpy(pBytes, writeBuffer, numBytes);
	m_pFile->DisableWriteBuffer();

	WriteSample(pBytes, numBytes, duration, 0, isSyncSample);

	MP4Free(pBytes);

	// update statistics
	if (m_bytesThisPacket > m_pPmax->GetValue()) {
		m_pPmax->SetValue(m_bytesThisPacket);
	}

	if (duration > m_pDmax->GetValue()) {
		m_pDmax->SetValue(duration);
	}

	MP4Timestamp startTime;

	GetSampleTimes(m_hintId, &startTime, NULL);

	if (startTime < m_thisSec + GetTimeScale()) {
		m_bytesThisSec += m_bytesThisHint;
	} else {
		if (m_bytesThisSec > m_pMaxr->GetValue()) {
			m_pMaxr->SetValue(m_bytesThisSec);
		}
		m_thisSec = startTime - (startTime % GetTimeScale());
		m_bytesThisSec = m_bytesThisHint;
	}

	// cleanup
	delete m_pHint;
	m_pHint = NULL;
}

void MP4RtpHintTrack::FinishWrite()
{
	if (m_hintId != MP4_INVALID_SAMPLE_ID) {
		m_pMaxPdu->SetValue(m_pPmax->GetValue());
		if (m_pNump->GetValue()) {
			m_pAvgPdu->SetValue(m_pTrpy->GetValue() / m_pNump->GetValue());
		}

		m_pMaxBitRate->SetValue(m_pMaxr->GetValue() * 8);
		if (GetDuration()) {
			m_pAvgBitRate->SetValue(
				m_pTrpy->GetValue() * 8 * GetTimeScale() / GetDuration());
		}
	}

	MP4Track::FinishWrite();
}

void MP4RtpHintTrack::InitStats()
{
	MP4Atom* pHinfAtom = m_pTrakAtom->FindAtom("trak.udta.hinf");

	ASSERT(pHinfAtom);

	pHinfAtom->FindProperty("hinf.trpy.bytes", (MP4Property**)&m_pTrpy); 
	pHinfAtom->FindProperty("hinf.nump.packets", (MP4Property**)&m_pNump); 
	pHinfAtom->FindProperty("hinf.tpyl.bytes", (MP4Property**)&m_pTpyl); 
	pHinfAtom->FindProperty("hinf.maxr.bytes", (MP4Property**)&m_pMaxr); 
	pHinfAtom->FindProperty("hinf.dmed.bytes", (MP4Property**)&m_pDmed); 
	pHinfAtom->FindProperty("hinf.dimm.bytes", (MP4Property**)&m_pDimm); 
	pHinfAtom->FindProperty("hinf.pmax.bytes", (MP4Property**)&m_pPmax); 
	pHinfAtom->FindProperty("hinf.dmax.milliSecs", (MP4Property**)&m_pDmax); 

	MP4Atom* pHmhdAtom = m_pTrakAtom->FindAtom("trak.mdia.minf.hmhd");

	ASSERT(pHmhdAtom);

	pHmhdAtom->FindProperty("hmhd.maxPduSize", (MP4Property**)&m_pMaxPdu); 
	pHmhdAtom->FindProperty("hmhd.avgPduSize", (MP4Property**)&m_pAvgPdu); 
	pHmhdAtom->FindProperty("hmhd.maxBitRate", (MP4Property**)&m_pMaxBitRate); 
	pHmhdAtom->FindProperty("hmhd.avgBitRate", (MP4Property**)&m_pAvgBitRate); 

	MP4Integer32Property* pMaxrPeriod = NULL;
	pHinfAtom->FindProperty("hinf.maxr.granularity",
		 (MP4Property**)&pMaxrPeriod); 
	if (pMaxrPeriod) {
		pMaxrPeriod->SetValue(1000);	// 1 second
	}
}


MP4RtpHint::MP4RtpHint()
{
	AddProperty( /* 0 */
		new MP4Integer16Property("packetCount"));
	AddProperty( /* 1 */
		new MP4Integer16Property("reserved"));
}

MP4RtpHint::~MP4RtpHint()
{
	for (u_int32_t i = 0; i < m_rtpPackets.Size(); i++) {
		delete m_rtpPackets[i];
	}
}

void MP4RtpHint::Set(bool isBframe, u_int32_t timestampOffset)
{
	m_isBframe = isBframe;
	m_timestampOffset = timestampOffset;
}

MP4RtpPacket* MP4RtpHint::AddPacket() 
{
	MP4RtpPacket* pPacket = new MP4RtpPacket();
	m_rtpPackets.Add(pPacket);

	// packetCount property
	((MP4Integer16Property*)m_pProperties[0])->IncrementValue();

	pPacket->SetBframe(m_isBframe);
	pPacket->SetTimestampOffset(m_timestampOffset);

	return pPacket;
}

void MP4RtpHint::Write(MP4File* pFile)
{
	MP4Container::Write(pFile);

	for (u_int32_t i = 0; i < m_rtpPackets.Size(); i++) {
		m_rtpPackets[i]->Write(pFile);
	}

	VERBOSE_WRITE_HINT(pFile->GetVerbosity(),
		printf("WriteRtpHint:\n"); Dump(stdout, 14, false));
}

void MP4RtpHint::Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits)
{
	MP4Container::Dump(pFile, indent, dumpImplicits);

	for (u_int32_t i = 0; i < m_rtpPackets.Size(); i++) {
		Indent(pFile, indent);
		fprintf(pFile, "RtpPacket: %u\n", i);
		m_rtpPackets[i]->Dump(pFile, indent + 1, dumpImplicits);
	}
}

MP4RtpPacket::MP4RtpPacket()
{
	AddProperty( /* 0 */
		new MP4Integer32Property("relativeXmitTime"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("reserved1", 2));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("Pbit", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("Xbit", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("reserved2", 4));
	AddProperty( /* 5 */
		new MP4BitfieldProperty("Mbit", 1));
	AddProperty( /* 6 */
		new MP4BitfieldProperty("payloadType", 7));
	AddProperty( /* 7  */
		new MP4Integer16Property("sequenceNumber"));
	AddProperty( /* 8 */
		new MP4BitfieldProperty("reserved3", 13));
	AddProperty( /* 9 */
		new MP4BitfieldProperty("extraFlag", 1));
	AddProperty( /* 10 */
		new MP4BitfieldProperty("bFrameFlag", 1));
	AddProperty( /* 11 */
		new MP4BitfieldProperty("repeatFlag", 1));
	AddProperty( /* 12 */
		new MP4Integer16Property("entryCount"));
}
 
MP4RtpPacket::~MP4RtpPacket()
{
	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		delete m_rtpData[i];
	}
}

void MP4RtpPacket::Set(u_int8_t payloadNumber, 
	u_int32_t packetId, bool setMbit)
{
	((MP4BitfieldProperty*)m_pProperties[5])->SetValue(setMbit);
	((MP4Integer8Property*)m_pProperties[6])->SetValue(payloadNumber);
	((MP4Integer32Property*)m_pProperties[7])->SetValue(packetId);
}

void MP4RtpPacket::SetBframe(bool isBframe)
{
	((MP4BitfieldProperty*)m_pProperties[10])->SetValue(isBframe);
}

void MP4RtpPacket::SetTimestampOffset(u_int32_t timestampOffset)
{
	if (timestampOffset == 0) {
		return;
	}

	ASSERT(((MP4BitfieldProperty*)m_pProperties[9])->GetValue() == 0);

	// set X bit
	((MP4BitfieldProperty*)m_pProperties[9])->SetValue(1);

	AddProperty( /* 13 */
		new MP4Integer32Property("extraInformationLength"));

	// This is a bit of a hack, since the tlv entries are really defined 
	// as atoms but there is only one type defined now, rtpo, and getting 
	// our atom code hooked up here would be a major pain with little gain
	AddProperty( /* 14 */
		new MP4Integer32Property("tlvLength"));
	AddProperty( /* 15 */
		new MP4StringProperty("tlvType"));
	AddProperty( /* 16 */
		new MP4Integer32Property("timestampOffset"));

	((MP4Integer32Property*)m_pProperties[13])->SetValue(16);
	((MP4Integer32Property*)m_pProperties[14])->SetValue(12);
	((MP4StringProperty*)m_pProperties[15])->SetFixedLength(4);
	((MP4StringProperty*)m_pProperties[15])->SetValue("rtpo");
	((MP4Integer32Property*)m_pProperties[16])->SetValue(timestampOffset);
}

void MP4RtpPacket::AddData(MP4RtpData* pData)
{
	m_rtpData.Add(pData);

	// increment entry count property
	((MP4Integer16Property*)m_pProperties[12])->IncrementValue();
}

void MP4RtpPacket::Write(MP4File* pFile)
{
	MP4Container::Write(pFile);

	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		m_rtpData[i]->Write(pFile);
	}
}

void MP4RtpPacket::Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits)
{
	MP4Container::Dump(pFile, indent, dumpImplicits);

	for (u_int32_t i = 0; i < m_rtpData.Size(); i++) {
		Indent(pFile, indent);
		fprintf(pFile, "RtpData: %u\n", i);
		m_rtpData[i]->Dump(pFile, indent + 1, dumpImplicits);
	}
}

MP4RtpData::MP4RtpData()
{
	AddProperty( /* 0 */
		new MP4Integer8Property("type"));
}

MP4RtpImmediateData::MP4RtpImmediateData()
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(1);

	AddProperty( /* 1 */
		new MP4Integer8Property("count"));
	AddProperty( /* 2 */
		new MP4BytesProperty("data", 14));

	((MP4BytesProperty*)m_pProperties[2])->SetFixedSize(14);
}

void MP4RtpImmediateData::Set(const u_int8_t* pBytes, u_int8_t numBytes)
{
	((MP4Integer8Property*)m_pProperties[1])->SetValue(numBytes);
	((MP4BytesProperty*)m_pProperties[2])->SetValue(pBytes, numBytes);
}

MP4RtpSampleData::MP4RtpSampleData()
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(2);

	AddProperty( /* 1 */
		new MP4Integer8Property("trackRefIndex"));
	AddProperty( /* 2 */
		new MP4Integer16Property("length"));
	AddProperty( /* 3 */
		new MP4Integer32Property("sampleNumber"));
	AddProperty( /* 4 */
		new MP4Integer32Property("sampleOffset"));
	AddProperty( /* 5 */
		new MP4Integer16Property("bytesPerBlock"));
	AddProperty( /* 6 */
		new MP4Integer16Property("samplesPerBlock"));

	((MP4Integer32Property*)m_pProperties[5])->SetValue(1);
	((MP4Integer32Property*)m_pProperties[6])->SetValue(1);
}

void MP4RtpSampleData::Set(MP4SampleId sampleId,
	u_int32_t sampleOffset, u_int16_t sampleLength)
{
	((MP4Integer16Property*)m_pProperties[2])->SetValue(sampleLength);
	((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleId);
	((MP4Integer32Property*)m_pProperties[4])->SetValue(sampleOffset);
}

void MP4RtpSampleData::SetTrackReference(u_int8_t trackRefIndex)
{
	((MP4Integer8Property*)m_pProperties[1])->SetValue(trackRefIndex);
}

MP4RtpSampleDescriptionData::MP4RtpSampleDescriptionData()
{
	((MP4Integer8Property*)m_pProperties[0])->SetValue(3);

	AddProperty( /* 1 */
		new MP4Integer8Property("trackRefIndex"));
	AddProperty( /* 2 */
		new MP4Integer16Property("length"));
	AddProperty( /* 3 */
		new MP4Integer32Property("sampleDescriptionIndex"));
	AddProperty( /* 4 */
		new MP4Integer32Property("sampleDescriptionOffset"));
	AddProperty( /* 5 */
		new MP4Integer32Property("reserved"));
}

void MP4RtpSampleDescriptionData::Set(u_int32_t sampleDescrIndex,
	u_int32_t offset, u_int16_t length)
{
	((MP4Integer16Property*)m_pProperties[2])->SetValue(length);
	((MP4Integer32Property*)m_pProperties[3])->SetValue(sampleDescrIndex);
	((MP4Integer32Property*)m_pProperties[4])->SetValue(offset);
}

