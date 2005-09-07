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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May 		wmay@cisco.com
 */
#define DECLARE_CONFIG_VARIABLES 1
#include "profile_video.h"
#undef DECLARE_CONFIG_VARIABLES
#include "video_encoder.h"

void CVideoProfile::LoadConfigVariables (void)
{
  AddConfigVariables(VideoProfileConfigVariables, 
		     NUM_ELEMENTS_IN_ARRAY(VideoProfileConfigVariables));
  // eventually will add interface to read each encoder's variables
  AddVideoProfileEncoderVariables(this);
}

void CVideoProfile::Update (void) 
{
  VideoProfileCheck(this);

  u_int16_t frameHeight;
  float aspectRatio = GetFloatValue(CFG_VIDEO_CROP_ASPECT_RATIO);

  // crop video to appropriate aspect ratio modulo 16 pixels
  if ((aspectRatio - VIDEO_STD_ASPECT_RATIO) < 0.1) {
    frameHeight = GetIntegerValue(CFG_VIDEO_HEIGHT);
  } else {
    frameHeight = 
      (u_int16_t)(
		  (float)GetIntegerValue(CFG_VIDEO_WIDTH) 
		  / aspectRatio);
    
    if ((frameHeight % 16) != 0) {
      frameHeight += 16 - (frameHeight % 16);
    }
    
    if (frameHeight > GetIntegerValue(CFG_VIDEO_HEIGHT)) {
      // OPTION might be better to insert black lines 
      // to pad image but for now we crop down
      frameHeight = GetIntegerValue(CFG_VIDEO_HEIGHT);
      if ((frameHeight % 16) != 0) {
	frameHeight -= (frameHeight % 16);
      }
    }
  }

  m_videoWidth = GetIntegerValue(CFG_VIDEO_WIDTH);
  m_videoHeight = frameHeight;

  m_ySize = m_videoWidth * m_videoHeight;
  m_uvSize = m_ySize / 4;
  m_yuvSize = (m_ySize * 3) / 2;
  
  m_videoMaxVopSize = 128 * 1024;
  // the below updates the mpeg4 video profile
  uint32_t widthMB, heightMB;
  uint32_t bitrate;
  if (GetBoolValue(CFG_VIDEO_FORCE_PROFILE_ID)) {
    m_videoMpeg4ProfileId = GetIntegerValue(CFG_VIDEO_PROFILE_ID);
  } else {
    bitrate = GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000;

    widthMB = (m_videoWidth + 15) / 16;
    heightMB = (m_videoHeight + 15) / 16;
       
    float frame_rate = GetFloatValue(CFG_VIDEO_FRAME_RATE);

    frame_rate *= widthMB;
    frame_rate *= heightMB;
	
    uint32_t mbpSec = (uint32_t)ceil(frame_rate);
    uint16_t profile;
    if (bitrate < 131072 && mbpSec < 2970) {
      // profile is ASP 0/1 - subsumed3 by SP2
      profile = MPEG4_ASP_L0;
    } else if (bitrate < 393216 && mbpSec < 5940) {
      // profile is ASP 2 - subsumed by SP3 
      profile = MPEG4_ASP_L2;
    } else if (bitrate < 786432 && mbpSec < 11880) {
      // profile is ASP 3
      profile = MPEG4_ASP_L3;
    } else if (bitrate < 1536000 && mbpSec < 11880) {
      // profile is ASP3b
      profile = MPEG4_ASP_L3B;
    } else if (bitrate < 3000 * 1024 && mbpSec < 23760) {
      // profile is ASP4
      profile = MPEG4_ASP_L4;
    } else {
      // profile is ASP5 - but may be higher
      if (bitrate > 8000 * 1024 || mbpSec > 16384) {
	error_message("Video statistics surpass ASP Level 5 - bit rate %u MpbSec %u", 
		      bitrate, mbpSec);
      }
      profile = MPEG4_ASP_L5;
    }
    if (GetBoolValue(CFG_VIDEO_USE_B_FRAMES) == false) {
      if (bitrate <= 65536 && mbpSec < 1485) {
	// profile is SP0, or SP1  SP0 is more accurate
	profile = MPEG4_SP_L0;
      } else if (bitrate <= 131072 && mbpSec < 5940) {
	// profile is SP2
	profile = MPEG4_SP_L2;
      } else if (bitrate < 393216 && mbpSec < 11880) {
	// profile is SP3
	profile = MPEG4_SP_L3;
      }
    }
    m_videoMpeg4ProfileId = profile;
  }
  
  GenerateMpeg4VideoConfig(this);
}
