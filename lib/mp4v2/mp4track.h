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

#ifndef __MP4_TRACK_INCLUDED__
#define __MP4_TRACK_INCLUDED__

// forward declarations
class MP4File;
class MP4Atom;

class MP4Track {
public:
	MP4Track(MP4File* pFile, MP4Atom* pTrakAtom);

protected:
	MP4File*	m_pFile;
	MP4Atom* 	m_pTrakAtom;		// moov.trak[]
	MP4TrackId	m_trackId;			// moov.trak[].tkhd.trackId
	MP4SampleId m_currentSample;
	u_int32_t 	m_cachedSampleSize;
};

MP4ARRAY_DECL(MP4Track, MP4Track*);

#endif /* __MP4_TRACK_INCLUDED__ */
