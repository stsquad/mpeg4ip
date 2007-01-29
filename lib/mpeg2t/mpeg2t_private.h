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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */
#ifndef __MPEG2T_PRIVATE_H__
#define __MPEG2T_PRIVATE_H__ 1

#include "mpeg2_transport.h"

int process_mpeg2t_ac3_audio(mpeg2t_es_t *es_pid, const uint8_t *esptr,
			      uint32_t buflen);
int mpeg2t_ac3_audio_info(mpeg2t_es_t *es_pid, char *buffer, size_t buflen);

// mpeg2t_mp3.c
int process_mpeg2t_mpeg_audio(mpeg2t_es_t *es_pid, const uint8_t *esptr,
			      uint32_t buflen);
int mpeg2t_mpeg_audio_info(mpeg2t_es_t *es_pid, char *buffer, size_t buflen);
// mpeg2_video.c
int process_mpeg2t_mpeg_video(mpeg2t_es_t *es_pid, 
			      const uint8_t *esptr, 
			      uint32_t buflen);
int mpeg2t_mpeg_video_info(mpeg2t_es_t *es_pid, char *buffer, size_t buflen);
// mpeg2_video.c
int process_mpeg2t_h264_video(mpeg2t_es_t *es_pid, 
			      const uint8_t *esptr, 
			      uint32_t buflen);
int mpeg2t_h264_video_info(mpeg2t_es_t *es_pid, char *buffer, size_t buflen);
int process_mpeg2t_mpeg4_video(mpeg2t_es_t *es_pid, 
			       const uint8_t *esptr, 
			       uint32_t buflen);
int mpeg2t_mpeg4_video_info(mpeg2t_es_t *es_pid, char *buffer, size_t buflen);


void mpeg2t_malloc_es_work(mpeg2t_es_t *es_pid, uint32_t frame_len);
void mpeg2t_finished_es_work(mpeg2t_es_t *es_pid, uint32_t frame_len);

void mpeg2t_message(int loglevel, const char *fmt, ...)
#ifndef _WIN32
     __attribute__((format(__printf__, 2, 3)));
#else
     ;
#endif

#endif
