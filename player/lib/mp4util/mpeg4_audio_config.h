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

#ifndef __MPEG4_AUDIO_CONFIG_H__
#define __MPEG4_AUDIO_CONFIG_H__

typedef struct aac_audio_config_t {
  int frame_len_1024;
} aac_audio_config_t;


#define CELP_EXCITATION_MODE_RPE 1
#define CELP_EXCITATION_MODE_MPE 0

typedef struct celp_audio_config_t {
  int isBaseLayer;
  int isBWSLayer;
  int CELP_BRS_id;
  int NumOfBitsInBuffer;
  int excitation_mode;
  int sample_rate_mode;
  int fine_rate_control;
  int rpe_config;
  int mpe_config;
  int num_enh_layers;
  int bwsm;
  int samples_per_frame;
} celp_audio_config_t;


typedef struct mpeg4_audio_config_t {
  unsigned int audio_object_type;
  unsigned int frequency;
  unsigned int channels;
  union {
    aac_audio_config_t aac;
    celp_audio_config_t celp;
  } codec;
} mpeg4_audio_config_t;

extern "C" {
void decode_mpeg4_audio_config(const uint8_t *buffer,
			       uint32_t buf_len,
			       mpeg4_audio_config_t *mptr,
			       bool parse_streammux);

int audio_object_type_is_aac(mpeg4_audio_config_t *mptr);

int audio_object_type_is_celp(mpeg4_audio_config_t *mptr);
}
#endif
