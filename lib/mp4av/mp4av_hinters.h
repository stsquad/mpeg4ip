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

#ifndef __MP4AV_HINTERS_INCLUDED__
#define __MP4AV_HINTERS_INCLUDED__ 

// Generic Audio Hinters

typedef u_int32_t (*MP4AV_AudioSampleSizer)(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId); 

typedef void (*MP4AV_AudioConcatenator)(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration);

typedef void (*MP4AV_AudioFragmenter)(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t sampleSize, 
	MP4Duration sampleDuration);

void MP4AV_AudioConsecutiveHinter( 
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4Duration sampleDuration, 
	u_int8_t perPacketHeaderSize,
	u_int8_t perSampleHeaderSize,
	u_int8_t maxSamplesPerPacket,
	MP4AV_AudioSampleSizer pSizer,
	MP4AV_AudioConcatenator pConcatenator,
	MP4AV_AudioFragmenter pFragmenter);

void MP4AV_AudioInterleaveHinter( 
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4Duration sampleDuration, 
	u_int8_t stride, 
	u_int8_t bundle,
	MP4AV_AudioConcatenator pConcatenator);


// Specific Audio Hinters

void MP4AV_Rfc2250Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId);

void MP4AV_Rfc3119Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	bool interleave = false);

void MP4AV_RfcIsmaHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	bool interleave = false);


// Specific Video Hinters

void MP4AV_Rfc3016Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId);

#endif /* __MP4AV_HINTERS_INCLUDED__ */ 

