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
 *              Bill May        wmay@cisco.com
 */

#ifndef __LIVE_APIS_H__
#define __LIVE_APIS_H__ 1
#include <stdint.h>
#include <stdlib.h>
#include "file_write.h"

const char *get_capture_device(void);
const char **get_capture_devices(size_t &max, size_t &current_index);
size_t get_capture_device_index(void);
void set_capture_device(size_t index);

const char *get_video_capture_input(void);
const char **get_video_capture_inputs(size_t capture_device,
				      size_t &max,
				      size_t &current_index);
size_t get_video_capture_input_index();

void set_video_capture_inputs(size_t index);

size_t get_video_height(void);
size_t get_video_width(void);
size_t get_video_frames_per_second(void);
int get_video_crop_enabled(void);
size_t get_video_crop_height(void);
size_t get_video_crop_width(void);

const char *get_video_encoder_type(void);
const char **get_video_encoder_types(size_t &max, size_t &current_index);
size_t get_video_encoder_index(void);
size_t get_video_encoder_kbps(void);

enum {
  VIDEO_ERR_BAD_HEIGHT,
  VIDEO_ERR_BAD_WIDTH,
  VIDEO_ERR_BAD_HEIGHT_WIDTH,
  VIDEO_ERR_BAD_FPS,
  VIDEO_ERR_BAD_CROP_HEIGHT,
  VIDEO_ERR_BAD_CROP_WIDTH,
  VIDEO_ERR_BAD_CROP_HEIGHT_WIDTH,
};

int check_video_parameters(size_t height,
			   size_t width,
			   int crop_enabled,
			   size_t crop_height,
			   size_t crop_width,
			   size_t fps,
			   size_t video_encoder,
			   size_t video_encode_kbps,
			   const char **errmsg);

int set_video_parameters(size_t height,
			 size_t width,
			 size_t crop_enabled,
			 size_t crop_height,
			 size_t crop_width,
			 size_t fps,
			 size_t video_encoder,
			 size_t video_encode_kbps);

void init_audio(void);
const char *get_audio_frequency(void);
const char **get_audio_frequencies(size_t &max, size_t &current_index);
const char *get_audio_codec(void);
const char **get_audio_codecs(size_t &max, size_t &current_index);
size_t get_audio_kbitrate(void);

int set_audio_parameters(size_t audio_codec_index,
			 size_t audio_freq_index,
			 size_t audio_bitrate,
			 const char **errmsg);

int start_audio_recording(CFileWriteBase *);
int stop_audio_recording(void);
CFileWriteBase *audio_record_file(const char *record_file);

const char *get_broadcast_address(void);
uint16_t get_broadcast_video_port(void);
uint16_t get_broadcast_audio_port(void);

int set_broadcast_parameters(const char *addr,
			     uint16_t vport,
			     uint16_t aport,
			     const char **ermsg);

const char *get_record_file(void);
int get_record_video(void);
int get_record_audio(void);
int set_record_parameters(int record_v,
			  int record_a,
			  const char *record_f,
			  const char **errmsg);
CFileWriteBase *start_record(int audio_enabled, int video_enabled);
#endif
