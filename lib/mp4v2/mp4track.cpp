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

MP4Track::MP4Track(MP4File* pFile, MP4Atom* pTrakAtom) 
{
	m_pFile = pFile;
	m_pTrakAtom = pTrakAtom;
	m_writeSampleId = 1;

	bool success = true;

	MP4Integer32Property* pTrackIdProperty;
	success &= m_pTrakAtom->FindProperty(
		"trak.tkhd.trackId",
		(MP4Property**)&pTrackIdProperty);
	if (success) {
		m_trackId = pTrackIdProperty->GetValue();
	}
	
	MP4Integer32Property* pTypeProperty;
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.hdlr.handlerType",
		(MP4Property**)&pTypeProperty);
	if (success) {
		INT32TOSTR(pTypeProperty->GetValue(), m_type);
	}

	// get handles on sample size information

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.sampleSize",
		(MP4Property**)&m_pFixedSampleSizeProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsz.entries[0].sampleSize",
		(MP4Property**)&m_pSampleSizeProperty);

	// get handles on information needed to map sample id's to file offsets

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entryCount",
		(MP4Property**)&m_pStscCountProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries[0].firstChunk",
		(MP4Property**)&m_pStscFirstChunkProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries[0].samplesPerChunk",
		(MP4Property**)&m_pStscSamplesPerChunkProperty);

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stsc.entries[0].firstSample",
		(MP4Property**)&m_pStscFirstSampleProperty);

	bool haveStco = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stco.entries[0].chunkOffset",
		(MP4Property**)&m_pChunkOffsetProperty);

	if (!haveStco) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.co64.entries[0].chunkOffset",
			(MP4Property**)&m_pChunkOffsetProperty);
	}

	// get handles on sample timing info

	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entryCount",
		(MP4Property**)&m_pSttsCountProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entries[0].sampleCount",
		(MP4Property**)&m_pSttsSampleCountProperty);
	
	success &= m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stts.entries[0].sampleDelta",
		(MP4Property**)&m_pSttsSampleDeltaProperty);
	
	// get handles on rendering offset info if it exists

	m_pCttsCountProperty = NULL;
	m_pCttsSampleCountProperty = NULL;
	m_pCttsSampleOffsetProperty = NULL;

	bool haveCtts = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.ctts.entryCount",
		(MP4Property**)&m_pCttsCountProperty);

	if (haveCtts) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.ctts.entries[0].sampleCount",
			(MP4Property**)&m_pCttsSampleCountProperty);

		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.ctts.entries[0].sampleOffset",
			(MP4Property**)&m_pCttsSampleOffsetProperty);
	}

	// get handles on sync sample info if it exists

	m_pStssCountProperty = NULL;
	m_pStssSampleProperty = NULL;

	bool haveStss = m_pTrakAtom->FindProperty(
		"trak.mdia.minf.stbl.stss.entryCount",
		(MP4Property**)&m_pStssCountProperty);

	if (haveStss) {
		success &= m_pTrakAtom->FindProperty(
			"trak.mdia.minf.stbl.stss.entries[0].sampleNumber",
			(MP4Property**)&m_pStssSampleProperty);
	}

	// was everything found?
	if (!success) {
		throw new MP4Error("invalid track", "MP4Track::MP4Track");
	}
}

void MP4Track::ReadSample(MP4SampleId sampleId,
	u_int8_t** ppBytes, u_int32_t* pNumBytes, 
	MP4Timestamp* pStartTime, MP4Duration* pDuration,
	MP4Duration* pRenderingOffset, bool* pIsSyncSample)
{
	if (sampleId == 0) {
		throw MP4Error("sample id can't be zero", "MP4Track::ReadSample");
	}

	u_int64_t fileOffset = GetSampleFileOffset(sampleId);

	*pNumBytes = GetSampleSize(sampleId);

	VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
		printf("ReadSample: id %u offset 0x"LLX" size %u (0x%x)\n",
			sampleId, fileOffset, *pNumBytes, *pNumBytes));

	*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);

	try { 
		m_pFile->SetPosition(fileOffset);
		m_pFile->ReadBytes(*ppBytes, *pNumBytes);

		if (pStartTime || pDuration) {
			GetSampleTimes(sampleId, pStartTime, pDuration);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  start "LLU" duration "LLD"\n",
					(pStartTime ? *pStartTime : 0), 
					(pDuration ? *pDuration : 0)));
		}
		if (pRenderingOffset) {
			*pRenderingOffset = GetSampleRenderingOffset(sampleId);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  renderingOffset "LLD"\n",
					*pRenderingOffset));
		}
		if (pIsSyncSample) {
			*pIsSyncSample = IsSyncSample(sampleId);

			VERBOSE_READ_SAMPLE(m_pFile->GetVerbosity(),
				printf("ReadSample:  isSyncSample %u\n",
					*pIsSyncSample));
		}
	}

	catch (MP4Error* e) {
		// let's not leak memory
		MP4Free(*ppBytes);
		*ppBytes = NULL;
		throw e;
	}
}

void MP4Track::WriteSample(u_int8_t* pBytes, u_int32_t numBytes,
	MP4Duration duration, MP4Duration renderingOffset, 
	bool isSyncSample)
{
	// move to EOF
	u_int64_t samplePosition = m_pFile->GetSize();
	m_pFile->SetPosition(samplePosition);

	m_pFile->WriteBytes(pBytes, numBytes);

	UpdateSampleSizes(m_writeSampleId, numBytes);

	//UpdateSampleFileOffsets(m_writeSampleId, samplePosition);

	//UpdateSampleTimes(m_writeSampleId, duration);

	//UpdateRenderingOffsets(m_writeSampleId, renderingOffset);

	UpdateSyncSamples(m_writeSampleId, isSyncSample);

	m_writeSampleId++;
}

u_int32_t MP4Track::GetSampleSize(MP4SampleId sampleId)
{
	u_int32_t fixedSampleSize = m_pFixedSampleSizeProperty->GetValue(); 
	if (fixedSampleSize != 0) {
		return fixedSampleSize;
	}
	return m_pSampleSizeProperty->GetValue(sampleId - 1);
}

void MP4Track::UpdateSampleSizes(MP4SampleId sampleId, u_int32_t numBytes)
{
	// TBD fixed sample size case

	m_pSampleSizeProperty->SetCount(sampleId);
	m_pSampleSizeProperty->SetValue(sampleId - 1, numBytes);
	// TBD count property
}

u_int64_t MP4Track::GetSampleFileOffset(MP4SampleId sampleId)
{
	u_int32_t stscIndex;
	u_int32_t numStscs = m_pStscCountProperty->GetValue();

	for (stscIndex = 0; stscIndex < numStscs; stscIndex++) {
		if (sampleId < m_pStscFirstSampleProperty->GetValue(stscIndex)) {
			ASSERT(stscIndex != 0);
			stscIndex -= 1;
			break;
		}
	}
	if (stscIndex == numStscs) {
		ASSERT(stscIndex != 0);
		stscIndex -= 1;
	}

	u_int32_t firstChunk = 
		m_pStscFirstChunkProperty->GetValue(stscIndex);

	MP4SampleId firstSample = 
		m_pStscFirstSampleProperty->GetValue(stscIndex);

	u_int32_t samplesPerChunk = 
		m_pStscSamplesPerChunkProperty->GetValue(stscIndex);

	u_int32_t chunkId = firstChunk +
		((sampleId - firstSample) / samplesPerChunk);

	u_int64_t chunkOffset;
	if (m_pChunkOffsetProperty->GetType() == Integer32Property) {
		chunkOffset = ((MP4Integer32Property*)m_pChunkOffsetProperty)
			->GetValue(chunkId - 1);
	} else { /* Integer64Property */
		chunkOffset = ((MP4Integer64Property*)m_pChunkOffsetProperty)
			->GetValue(chunkId - 1);
	}

	// need cumulative samples sizes from firstSample to sampleId - 1
	u_int32_t sampleOffset = 0;
	for (MP4SampleId i = firstSample; i < sampleId; i++) {
		sampleOffset += GetSampleSize(i);
	}

	return chunkOffset + sampleOffset;
}

MP4Duration MP4Track::GetFixedSampleDuration()
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();

	if (numStts != 1) {
		return 0;	// sample duration is not fixed
	}
	return m_pSttsSampleDeltaProperty->GetValue(0);
}

void MP4Track::GetSampleTimes(MP4SampleId sampleId,
	MP4Timestamp* pStartTime, MP4Duration* pDuration)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	MP4Duration elapsed = 0;

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		u_int32_t sampleCount = 
			m_pSttsSampleCountProperty->GetValue(sttsIndex);
		u_int32_t sampleDelta = 
			m_pSttsSampleDeltaProperty->GetValue(sttsIndex);

		if (sampleId <= sid + sampleCount - 1) {
			if (pStartTime) {
				*pStartTime = elapsed + ((sampleId - sid) * sampleDelta);
			}
			if (pDuration) {
				*pDuration = sampleDelta;
			}
			return;
		}
		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	throw MP4Error("sample id out of range", 
		"MP4Track::GetSampleTimes");
}

MP4SampleId MP4Track::GetSampleIdFromTime(
	MP4Timestamp when, bool wantSyncSample)
{
	u_int32_t numStts = m_pSttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	MP4Duration elapsed = 0;

	for (u_int32_t sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		u_int32_t sampleCount = 
			m_pSttsSampleCountProperty->GetValue(sttsIndex);
		u_int32_t sampleDelta = 
			m_pSttsSampleDeltaProperty->GetValue(sttsIndex);

		MP4Duration d = when - elapsed;

		if (d <= sampleCount * sampleDelta) {
			MP4SampleId sampleId;

			sampleId = sid + (d / sampleDelta);
			if (d % sampleDelta) {
				sampleId++;
			}

			if (wantSyncSample) {
				return GetNextSyncSample(sampleId);
			}
			return sampleId;
		}

		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	throw MP4Error("time out of range", 
		"MP4Track::GetSampleIdFromTime");
}

u_int32_t MP4Track::GetSampleRenderingOffset(MP4SampleId sampleId)
{
	if (m_pCttsCountProperty == NULL) {
		return 0;
	}

	u_int32_t numCtts = m_pCttsCountProperty->GetValue();
	MP4SampleId sid = 1;
	
	for (u_int32_t cttsIndex = 0; cttsIndex < numCtts; cttsIndex++) {
		u_int32_t sampleCount = 
			m_pCttsSampleCountProperty->GetValue(cttsIndex);

		if (sampleId <= sid + sampleCount - 1) {
			return m_pCttsSampleOffsetProperty->GetValue(cttsIndex);
		}
		sid += sampleCount;
	}

	throw MP4Error("sample id out of range", 
		"MP4Track::GetSampleRenderingOffset");
}

bool MP4Track::IsSyncSample(MP4SampleId sampleId)
{
	if (m_pStssCountProperty == NULL) {
		return true;
	}

	u_int32_t numStss = m_pStssCountProperty->GetValue();
	
	for (u_int32_t stssIndex = 0; stssIndex < numStss; stssIndex++) {
		MP4SampleId syncSampleId = 
			m_pStssSampleProperty->GetValue(stssIndex);

		if (sampleId == syncSampleId) {
			return true;
		} 
		if (sampleId > syncSampleId) {
			break;
		}
	}

	return false;
}

// N.B. "next" is inclusive of this sample id
MP4SampleId MP4Track::GetNextSyncSample(MP4SampleId sampleId)
{
	if (m_pStssCountProperty == NULL) {
		return sampleId;
	}

	u_int32_t numStss = m_pStssCountProperty->GetValue();
	
	for (u_int32_t stssIndex = 0; stssIndex < numStss; stssIndex++) {
		MP4SampleId syncSampleId = 
			m_pStssSampleProperty->GetValue(stssIndex);

		if (sampleId > syncSampleId) {
			continue;
		}
		return syncSampleId;
	}

	return (MP4SampleId)-1;
}

void MP4Track::UpdateSyncSamples(MP4SampleId sampleId, bool isSyncSample)
{
	if (isSyncSample) {
		// if stss atom exists, add entry
		if (m_pStssSampleProperty) {
			AddSyncSample(sampleId);
		} // else nothing to do (yet)

	} else { // !isSyncSample
		// if stss atom doesn't exist, create one
		if (m_pStssSampleProperty == NULL) {
			MP4Atom* pStssAtom = MP4Atom::CreateAtom("stss");
			ASSERT(pStssAtom);

			MP4Atom* pStblAtom = m_pTrakAtom->FindAtom("trak.mdia.minf.stbl");
			ASSERT(pStblAtom);

			pStssAtom->SetFile(m_pFile);
			pStssAtom->SetParentAtom(pStblAtom);
			pStblAtom->AddChildAtom(pStssAtom);

			pStssAtom->FindProperty(
				"stss.entryCount",
				(MP4Property**)&m_pStssCountProperty);

			pStssAtom->FindProperty(
				"stss.entries[0].sampleNumber",
				(MP4Property**)&m_pStssSampleProperty);

			for (MP4SampleId sid = 1; sid < sampleId; sid++) {
				AddSyncSample(sid);
			}
		} // else nothing to do
	}
}

void MP4Track::AddSyncSample(MP4SampleId sampleId)
{
	u_int32_t numStss = m_pStssCountProperty->GetValue();
	m_pStssSampleProperty->SetCount(numStss + 1);
	m_pStssSampleProperty->SetValue(numStss, sampleId);
	m_pStssCountProperty->IncrementValue();
}

// map track type name aliases to official names

const char* MP4Track::NormalizeTrackType(const char* type)
{
	if (!strcasecmp(type, "vide")
	  || !strcasecmp(type, "video")
	  || !strcasecmp(type, "mp4v")) {
		return "vide";
	}

	if (!strcasecmp(type, "soun")
	  || !strcasecmp(type, "sound")
	  || !strcasecmp(type, "audio")
	  || !strcasecmp(type, "mp4a")) {
		return "soun";
	}

	if (!strcasecmp(type, "sdsm")
	  || !strcasecmp(type, "scene")
	  || !strcasecmp(type, "bifs")) {
		return "sdsm";
	}

	if (!strcasecmp(type, "odsm")
	  || !strcasecmp(type, "od")) {
		return "odsm";
	}

	return type;
}

