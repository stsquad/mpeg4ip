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

#ifdef NOTDEF

ReadSample()
{
	GetSampleId(when);

	u_int64_t fileOffset = GetSampleOffset(m_currentSample);
	u_int32_t sampleSize = GetSampleSize(m_currentSample);

	pSample = MP4Malloc(sampleSize);

	m_pFile->SetPosition(fileOffset);
	m_pFile->ReadBytes(pSample, sampleSize);

	// start and duration
}

when to sampleId
	stts
	for all stts
		if when >= cumulative sample duration
			break;
		add in cumulative duration of this entry += a * b;

when to sync sampleId

u_int32_t GetSampleSize(MP4SampleId sampleId)
{
	if (m_currentSample == 0) {
		m_cachedSampleSize = GetTrackIntegerProperty(m_trackId
			"mdia.minf.stbl.stsz.sampleSize");
	}

	if (m_cachedSampleSize != 0) {
		return m_cachedSampleSize;
	}

	MP4Integer32Property* m_pSampleSizeProperty;
	u_int32_t index;

	m_pTrakAtom->FindProperty("mdia.minf.stbl.stsz.entries[0].sampleSize"
		&m_pSampleSizeProperty, &index);

	return m_pSampleSizeProperty->GetValue(sampleId)
}


u_int64_t GetSampleOffset(MP4SampleId sampleId)
{
	for all stsc entries
		if sampleId < F(first-chunk) where F is TBD, "firstSample" property?
			assert not at first entry
			backup 1 entry,
			break;
	have chunkId
	"stco|co64""entries[].chunkOffset"	
		gets us chunkOffset
	need cumulative samples sizes from firstSample to sampleId - 1
	// TBD cache value for use next time
	return chunkOffset + cumulative sizes
}
#endif /* NOTDEF */
