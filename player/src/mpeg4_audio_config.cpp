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
#include "systems.h"
#include "mpeg4_audio_config.h"
#include "bitstream/bitstream.h"
#include "player_util.h"

static long freq_index_to_freq[] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 
  12000, 11025, 8000, 7350 };


int audio_object_type_is_aac (mpeg4_audio_config_t *mptr)
{
  unsigned int audio_object;
  audio_object = mptr->audio_object_type;
  if (audio_object == 1 || 
      audio_object == 2 ||
      audio_object == 3 || 
      audio_object == 4 ||
      audio_object == 6 ||
      audio_object == 7) 
    return 1;
  return 0;
}

void decode_mpeg4_audio_config (const unsigned char *buffer, 
			       uint32_t buf_len,
			       mpeg4_audio_config_t *mptr)
{
  CBitstream bit;
  uint32_t ret;

  bit.init(buffer, buf_len * 8);

  if (bit.getbits(5, &ret) < 0)
    return;

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
  }
}
