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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#ifndef __MEDIA_FRAME_H__
#define __MEDIA_FRAME_H__

#include <sys/types.h>
#include <SDL.h>
#include "media_time.h"

typedef u_int16_t MediaType;

class CMediaFrame {
public:
	CMediaFrame(
		MediaType type = 0, 
		void* pData = NULL, 
		u_int32_t dataLength = 0, 
		Timestamp timestamp = 0, 
		Duration duration = 0, 
		u_int32_t durationScale = TimestampTicks) {

		m_pMutex = SDL_CreateMutex();
		if (m_pMutex == NULL) {
			debug_message("CMediaFrame CreateMutex error");
		}
		m_refcnt = 1;
		m_type = type;
		m_pData = pData;
		m_dataLength = dataLength;
		m_timestamp = timestamp;
		m_duration = duration;
		m_durationScale = durationScale;
	}

	void AddReference(void) {
		if (SDL_LockMutex(m_pMutex) == -1) {
			debug_message("AddReference LockMutex error");
		}
		m_refcnt++;
		if (SDL_UnlockMutex(m_pMutex) == -1) {
			debug_message("AddReference UnlockMutex error");
		}
	}

	void RemoveReference(void) {
		if (SDL_LockMutex(m_pMutex) == -1) {
			debug_message("RemoveReference LockMutex error");
		}
		m_refcnt--;
		if (SDL_UnlockMutex(m_pMutex) == -1) {
			debug_message("RemoveReference UnlockMutex error");
		}
	}

	void operator delete(void* p) {
		CMediaFrame* me = (CMediaFrame*)p;
		if (SDL_LockMutex(me->m_pMutex) == -1) {
			debug_message("CMediaFrame delete LockMutex error");
		}
		if (me->m_refcnt > 0) {
			me->m_refcnt--;
		}
		if (me->m_refcnt > 0) {
			if (SDL_UnlockMutex(me->m_pMutex) == -1) {
				debug_message("CMediaFrame delete UnlockMutex error");
			}
			return;
		}
		free(me->m_pData);
		SDL_DestroyMutex(me->m_pMutex);
		free(me);
	}

	// predefined types of frames
	static const MediaType UndefinedFrame 	=	0;

	static const MediaType PcmAudioFrame	=	1;
	static const MediaType Mp3AudioFrame 	=	2;
	static const MediaType AacAudioFrame 	=	3;
	static const MediaType Ac3AudioFrame	=	4;
	static const MediaType VorbisAudioFrame	=	5;

	static const MediaType YuvVideoFrame	=	11;
	static const MediaType RgbVideoFrame	=	12;
	static const MediaType Mpeg2VideoFrame	=	13;
	static const MediaType Mpeg4VideoFrame	=	14;
	static const MediaType ReconstructYuvVideoFrame 	=	15;

	// get methods for properties

	MediaType GetType(void) {
		return m_type;
	}
	void* GetData(void) {
		return m_pData;
	}
	u_int32_t GetDataLength(void) {
		return m_dataLength;
	}
	Timestamp GetTimestamp(void) {
		return m_timestamp;
	}
	Duration GetDuration(void) {
		return m_duration;
	}
	u_int32_t GetDurationScale(void) {
		return m_durationScale;
	}

	u_int32_t ConvertDuration(u_int32_t newScale) {
		if (m_durationScale == newScale) {
			return m_duration;	// for newer code
		}
		// for older code
		return (((m_duration * newScale) / (m_durationScale >> 1)) + 1) >> 1; 
	}

protected:
	SDL_mutex*	m_pMutex;
	u_int16_t	m_refcnt;
	MediaType	m_type;
	void* 		m_pData;
	u_int32_t 	m_dataLength;
	Timestamp	m_timestamp;
	Duration 	m_duration;
	u_int32_t	m_durationScale;
};

#endif /* __MEDIA_FRAME_H__ */

