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
try{
	m_pFile = pFile;
	m_pTrakAtom = pTrakAtom;

	// TBD cleaner if MP4Track had GetIntegerProperty() from trak atom
	m_cachedSampleSize = m_pFile->GetTrackIntegerProperty(m_trackId,
		"mdia.minf.stbl.stsz.sampleSize");

	u_int32_t index;

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stsz.entries[0].sampleSize",
		(MP4Property**)&m_pSampleSizeProperty, &index);

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stsc.entryCount",
		(MP4Property**)&m_pStscCountProperty, &index);

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stsc.entries[0].firstChunk",
		(MP4Property**)&m_pStscFirstChunkProperty, &index);

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stsc.entries[0].samplesPerChunk",
		(MP4Property**)&m_pStscSamplesPerChunkProperty, &index);

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stsc.entries[0].firstSample",
		(MP4Property**)&m_pStscFirstSampleProperty, &index);

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stco.entries[0].chunkOffset",
		(MP4Property**)&m_pChunkOffsetProperty, &index);

	// TBD co64
}
catch (MP4Error* e) {
	printf("MP4Track constructuor error\n");
}
}

MP4SampleId MP4Track::GetSampleIdFromTime(
	MP4Timestamp when, bool wantSyncSample)
{
	return 1;
}

void MP4Track::ReadSample(MP4SampleId sampleId,
	u_int8_t** ppBytes, u_int32_t* pNumBytes, 
	MP4Timestamp* pStartTime, MP4Duration* pDuration,
	MP4Duration* pRenderingOffset, bool* pIsSyncSample)
{
	u_int64_t fileOffset = GetSampleOffset(sampleId);

	*pNumBytes = GetSampleSize(sampleId);

	*ppBytes = (u_int8_t*)MP4Malloc(*pNumBytes);

	try { 
		m_pFile->SetPosition(fileOffset);
		m_pFile->ReadBytes(*ppBytes, *pNumBytes);
	}
	catch (MP4Error* e) {
		MP4Free(ppBytes);
		*ppBytes = NULL;
		throw e;
	}

	// TBD optional outputs
	if (pStartTime) {
	}
	if (pDuration) {
	}
	if (pRenderingOffset) {
		*pRenderingOffset = 0;
	}
	if (pIsSyncSample) {
		*pIsSyncSample = true;
	}
}

void MP4Track::WriteSample(u_int8_t* pBytes, u_int32_t numBytes,
	MP4Duration duration, MP4Duration renderingOffset, 
	bool isSyncSample)
{
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

u_int32_t MP4Track::GetSampleSize(MP4SampleId sampleId)
{
	if (m_cachedSampleSize != 0) {
		return m_cachedSampleSize;
	}
	return m_pSampleSizeProperty->GetValue(sampleId);
}

u_int64_t MP4Track::GetSampleOffset(MP4SampleId sampleId)
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

	u_int64_t chunkOffset =
		m_pChunkOffsetProperty->GetValue(chunkId);

	// need cumulative samples sizes from firstSample to sampleId - 1
	u_int32_t sampleOffset = 0;
	for (MP4SampleId i = firstSample; i < sampleId; i++) {
		sampleOffset += GetSampleSize(i);
	}

	return chunkOffset + sampleOffset;
}

#ifdef NOTDEF
when to sampleId
	stts
	for all stts
		if when >= cumulative sample duration
			break;
		add in cumulative duration of this entry += a * b;

#endif /* NOTDEF */
