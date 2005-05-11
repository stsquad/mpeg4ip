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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *              Bill May                wmay@cisco.com
 */

#include "mp4live.h"
#include "media_source.h"
#include "audio_encoder.h"
#include "video_encoder.h"
#include "video_util_rgb.h"
#include <mp4av.h>
#include "mpeg4ip_byteswap.h"
#include "video_util_filter.h"
//#define DEBUG_AUDIO_RESAMPLER 1
//#define DEBUG_SYNC 1
//#define DEBUG_AUDIO_SYNC 1
//#define DEBUG_VIDEO_SYNC 1

CMediaSource::CMediaSource() 
{

  m_source = false;

  m_videoSource = this;
}

CMediaSource::~CMediaSource() 
{
}


bool CMediaSource::InitVideo (bool realTime)
{
  m_sourceRealTime = realTime;

  m_videoWantKeyFrame = true;

  return true;
}

void CMediaSource::SetVideoSrcSize(
				   u_int16_t srcWidth,
				   u_int16_t srcHeight,
				   u_int16_t srcStride)
{
  // N.B. InitVideo() must be called first

  m_videoSrcWidth = srcWidth;
  m_videoSrcHeight = srcHeight;
  m_videoSrcAspectRatio = (float)srcWidth / (float)srcHeight;

  // N.B. SetVideoSrcSize() should be called once before 

  m_videoSrcYStride = srcStride;
  m_videoSrcUVStride = srcStride / 2;

  // these next three may change below
  m_videoSrcAdjustedHeight = m_videoSrcHeight;

  m_videoSrcYSize = m_videoSrcYStride 
    * MAX(m_videoSrcHeight, m_videoSrcAdjustedHeight);
  m_videoSrcUVSize = m_videoSrcYSize / 4;
  m_videoSrcYUVSize = (m_videoSrcYSize * 3) / 2;

  // resizing

}



bool CMediaSource::InitAudio(
			     bool realTime)
{
  m_sourceRealTime = realTime;
  m_audioSrcSampleNumber = 0;
  m_audioSrcFrameNumber = 0;

  return true;
}

bool CMediaSource::SetAudioSrc(
			       MediaType srcType,
			       u_int8_t srcChannels,
			       u_int32_t srcSampleRate)
{
  // audio source info 
  m_audioSrcChannels = srcChannels;
  m_audioSrcSampleRate = srcSampleRate;

  return true;
}


