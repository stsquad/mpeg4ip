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

void MP4File::MakeIsmaCompliant()
{
	if (m_useIsma) {
		// already done
		return;
	}
	m_useIsma = true;

	// find first audio and/or video tracks

	MP4TrackId audioTrackId = 
		MP4FindTrackId(this, 0, MP4_AUDIO_TRACK_TYPE);

	MP4TrackId videoTrackId = 
		MP4FindTrackId(this, 0, MP4_VIDEO_TRACK_TYPE);

	// automatically add OD track
	if (m_odTrackId == MP4_INVALID_TRACK_ID) {
		AddODTrack();

		if (audioTrackId != MP4_INVALID_TRACK_ID) {
			AddTrackToOd(audioTrackId);
		}

		if (videoTrackId != MP4_INVALID_TRACK_ID) {
			AddTrackToOd(videoTrackId);
		}
	}

	// automatically add scene track
	MP4TrackId sceneTrackId = 
			MP4FindTrackId(this, 0, MP4_SCENE_TRACK_TYPE);
	if (sceneTrackId == MP4_INVALID_TRACK_ID) {
		sceneTrackId = AddSceneTrack();
	}

	// write OD Update Command
	WriteIsmaODUpdateCommand(m_odTrackId, audioTrackId, videoTrackId);

	// write BIFS Scene Replace Command
	WriteIsmaSceneCommand(sceneTrackId, audioTrackId, videoTrackId);
}

void MP4File::WriteIsmaODUpdateCommand(
	MP4TrackId odTrackId,
	MP4TrackId audioTrackId, 
	MP4TrackId videoTrackId)
{
	MP4Descriptor* pCommandDescriptor = 
		CreateODCommand(MP4ODUpdateODCommandTag);
	pCommandDescriptor->Generate();

	for (u_int8_t i = 0; i < 2; i++) {
		MP4TrackId trackId;
		u_int16_t odId;

		if (i == 0) {
			trackId = audioTrackId;
			odId = 10;
		} else {
			trackId = videoTrackId;
			odId = 20;
		}

		if (trackId == MP4_INVALID_TRACK_ID) {
			continue;
		}

		u_int32_t mpodIndex = FindTrackReference(
			MakeTrackName(odTrackId, "tref.mpod"), 
			trackId);

		ASSERT(mpodIndex != 0);

		MP4Descriptor* pObjDescriptor =
			((MP4DescriptorProperty*)(pCommandDescriptor->GetProperty(0)))
			->AddDescriptor(MP4ODescrTag);
		pObjDescriptor->Generate();

		MP4BitfieldProperty* pOdIdProperty = NULL;
		pObjDescriptor->FindProperty("objectDescriptorId", 
			(MP4Property**)&pOdIdProperty);
		ASSERT(pOdIdProperty);

		pOdIdProperty->SetValue(odId);

		MP4DescriptorProperty* pEsIdsDescriptorProperty = NULL;
		pObjDescriptor->FindProperty("esIds", 
			(MP4Property**)&pEsIdsDescriptorProperty);
		ASSERT(pEsIdsDescriptorProperty);

		pEsIdsDescriptorProperty->SetTags(MP4ESIDRefDescrTag);

		MP4Descriptor *pRefDescriptor =
			pEsIdsDescriptorProperty->AddDescriptor(MP4ESIDRefDescrTag);
		pRefDescriptor->Generate();

		MP4Integer16Property* pRefIndexProperty = NULL;
		pRefDescriptor->FindProperty("refIndex", 
			(MP4Property**)&pRefIndexProperty);
		ASSERT(pRefIndexProperty);

		pRefIndexProperty->SetValue(mpodIndex);
	}

	WriteODCommand(odTrackId, pCommandDescriptor);
}

void MP4File::WriteODCommand(
	MP4TrackId odTrackId,
	MP4Descriptor* pDescriptor)
{
	u_int8_t* pBytes;
	u_int64_t numBytes;

	// use write buffer to save descriptor in memory
	// instead of going directly to disk
	EnableWriteBuffer();
	pDescriptor->Write(this);
	GetWriteBuffer(&pBytes, &numBytes);
	DisableWriteBuffer();

	WriteSample(odTrackId, pBytes, numBytes, 1);
}

void MP4File::WriteIsmaSceneCommand(
	MP4TrackId sceneTrackId, 
	MP4TrackId audioTrackId, 
	MP4TrackId videoTrackId)
{
	// from ISMA 1.0 Tech Spec Appendix E
	static u_int8_t bifsAudioOnly[] = {
		0xC0, 0x10, 0x12, 
		0x81, 0x30, 0x2A, 0x05, 0x7C
	};
	static u_int8_t bifsVideoOnly[] = {
		0xC0, 0x10, 0x12, 
		0x61, 0x04, 0x88, 0x50, 0x45, 0x05, 0x3F, 0x00
	};
	static u_int8_t bifsAudioVideo[] = {
		0xC0, 0x10, 0x12, 
		0x81, 0x30, 0x2A, 0x05, 0x72,
		0x61, 0x04, 0x88, 0x50, 0x45, 0x05, 0x3F, 0x00
	};

	if (audioTrackId != MP4_INVALID_TRACK_ID 
	  && videoTrackId != MP4_INVALID_TRACK_ID) {
		WriteSample(sceneTrackId, 
			bifsAudioVideo, sizeof(bifsAudioVideo), 1);
	} else if (audioTrackId != MP4_INVALID_TRACK_ID) {
		WriteSample(sceneTrackId, 
			bifsAudioOnly, sizeof(bifsAudioOnly), 1);
	} else if (videoTrackId != MP4_INVALID_TRACK_ID) {
		WriteSample(sceneTrackId, 
			bifsVideoOnly, sizeof(bifsVideoOnly), 1);
	}
}	

