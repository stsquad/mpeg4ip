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
 *              Bill May        wmay@cisco.com
 */
#include "mpeg4ip.h"
#include "mpeg4_audio_config.h"
#include "mpeg4ip_bitstream.h"

static long freq_index_to_freq[] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 
  12000, 11025, 8000, 7350 };

extern "C" int audio_object_type_is_aac (mpeg4_audio_config_t *mptr)
{
  unsigned int audio_object;
  audio_object = mptr->audio_object_type;
  if (audio_object == 1 || 
      audio_object == 2 ||
      audio_object == 3 || 
      audio_object == 4 ||
      audio_object == 5 ||
      audio_object == 6 ||
      audio_object == 7 ||
      audio_object == 17) 
    return 1;
  return 0;
}




extern "C" int audio_object_type_is_celp (mpeg4_audio_config_t *mptr)
{
  unsigned int audio_object;

  audio_object = mptr->audio_object_type;
  if (audio_object == 8) 
    return 1;

  return 0;
}


extern "C" void decode_mpeg4_audio_config (const uint8_t *buffer, 
					   uint32_t buf_len,
					   mpeg4_audio_config_t *mptr,
					   bool parse_streammux)
{
  CBitstream bit;
  uint32_t ret = 0;

    
  bit.init(buffer, buf_len * 8);

  if (parse_streammux) {
    // skip 15 bits
    if (bit.getbits(15, &ret) < 0) return;
  }
  if (bit.getbits(5, &ret) < 0)
    return;

  // note: there is a 6 bit extension if we hit 0x1f for ret.
  // add 32 + the six bits
  //
  if (ret == 0x1f) {
    if (bit.getbits(6, &ret) < 0) return;
    ret = 32 + ret;
  }
  mptr->audio_object_type = ret;

  if (bit.getbits(4, &ret) < 0)
    return;

  if (ret == 0xf) {
    if (bit.getbits(24, &ret) < 0) 
      return;
    mptr->frequency = ret;
  } else {
    mptr->frequency = freq_index_to_freq[ret];
  }

  if (bit.getbits(4, &ret) < 0)
    return;

  mptr->channels = ret;

  // rptr points to remaining bits - starting with 0x04, moving
  // down buffer_len.
  if (audio_object_type_is_aac(mptr)) {
    if (bit.getbits(1, &ret) < 0)
      return;

    if (ret == 0) {
      mptr->codec.aac.frame_len_1024 = 1;
    } else {
      mptr->codec.aac.frame_len_1024 = 0;
    }
  } else if (audio_object_type_is_celp(mptr)){
    try {
      mptr->codec.celp.isBaseLayer = bit.GetBits(1);
      if (mptr->codec.celp.isBaseLayer == 0) {
	mptr->codec.celp.isBWSLayer = bit.GetBits(1);
	if (mptr->codec.celp.isBWSLayer == 0) {
	  mptr->codec.celp.CELP_BRS_id = bit.GetBits(2);
	}
      }

      mptr->codec.celp.NumOfBitsInBuffer=bit.bits_remain();
      mptr->codec.celp.excitation_mode = bit.GetBits(1);
      mptr->codec.celp.sample_rate_mode = bit.GetBits(1);
      mptr->codec.celp.fine_rate_control = bit.GetBits(1);
      if (mptr->codec.celp.excitation_mode == CELP_EXCITATION_MODE_RPE) {
	mptr->codec.celp.rpe_config = bit.GetBits(3);
	// 16000 is 10 msec, all others are 15 msec
	if (mptr->codec.celp.rpe_config == 1) // 16000 bitrate
	  mptr->codec.celp.samples_per_frame = (16000 * 10) / 1000;
	else
	  mptr->codec.celp.samples_per_frame = (16000 * 15) / 1000;
      } else {
	mptr->codec.celp.mpe_config = bit.GetBits(5);
	mptr->codec.celp.num_enh_layers = bit.GetBits(2);
	if (mptr->codec.celp.sample_rate_mode == 1) {
	  // 16kHz sample rate
	  if (mptr->codec.celp.mpe_config < 16) {
	    mptr->codec.celp.samples_per_frame = (16000 * 20) / 1000;
	  } else {
	    mptr->codec.celp.samples_per_frame = (16000 * 10) / 1000;
	  }
	} else {
	  if (bit.bits_remain() > 0) {
	    mptr->codec.celp.bwsm = bit.GetBits(1);
	  } else {
	    mptr->codec.celp.bwsm = 0;
	  }
	  if (mptr->codec.celp.mpe_config < 3) {
	    //40
	    mptr->codec.celp.samples_per_frame = (8000 * 40) / 1000;
	  } else if (mptr->codec.celp.mpe_config < 6) {
	    // 30
	    mptr->codec.celp.samples_per_frame = (8000 * 30) / 1000;
	  } else if (mptr->codec.celp.mpe_config < 22) {
	    // 20
	    mptr->codec.celp.samples_per_frame = (8000 * 20) / 1000;
	  } else if (mptr->codec.celp.mpe_config < 27) {
	    // 10
	    mptr->codec.celp.samples_per_frame = (8000 * 10) / 1000;
	  } else {
	    // 30
	    mptr->codec.celp.samples_per_frame = (8000 * 30) / 1000;
	  }
	  if (mptr->codec.celp.bwsm != 0) {
	    mptr->codec.celp.samples_per_frame *= 2;
	  }
	}      
      }
    } catch (...) {}
  }
}
