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

/* mpeg2_transport.h - API for transport stream decoding */

#ifndef __MPEG2_TRANSPORT_H__
#define __MPEG2_TRANSPORT_H__

#include "mpeg4ip_sdl_includes.h"
#include "mpeg2t_defines.h"

/*
 * mpeg2t_frame_t - how we'll pass frames back when we've stored them
 * These are malloced in 1 chunk - the frame pointer points right
 * to the end of the structure.
 */
typedef struct mpeg2t_frame_t {
  struct mpeg2t_frame_t *next_frame;
  int have_ps_ts; // 0 if we don't have presentation timestamp, 1 if we do
  int have_dts;
  uint64_t ps_ts; 
  uint64_t dts;
  int frame_type; // for video
  uint8_t *frame; // frame data
  uint32_t frame_len; // length of frame
  uint32_t pict_header_offset;
  uint32_t seq_header_offset;
  uint32_t nal_pic_param_offset; // h264
  uint32_t flags;
} mpeg2t_frame_t;

#define HAVE_SEQ_HEADER 0x1
#define HAVE_PICT_HEADER 0x2
#define HAVE_PIC_PARAM_HEADER 0x4
/*
 * PID list contains basic structure (mpeg2t_pid_t), with a type
 * field indicating which type to cast to
 */
typedef enum mpeg2t_pak_type {
  MPEG2T_PAS_PAK,
  MPEG2T_PROG_MAP_PAK,
  MPEG2T_ES_PAK,
} mpeg2t_pak_type;

struct mpeg2t_t;

typedef struct mpeg2t_unk_pid_t {
  struct mpeg2t_unk_pid_t *next_unk;
  uint16_t pid;
  uint32_t count;
} mpeg2t_unk_pid_t;

typedef struct mpeg2t_pid_t {
  struct mpeg2t_pid_t *next_pid;
  struct mpeg2t_t *main;
  uint32_t lastcc;
  uint32_t data_len;
  uint16_t pid;
  uint8_t *data;
  uint32_t data_len_loaded;
  uint32_t data_len_max;
  mpeg2t_pak_type pak_type;
  void *userdata;
} mpeg2t_pid_t;

  
typedef struct mpeg2t_pas_t {
  mpeg2t_pid_t pid;
  uint16_t transport_stream_id;
  uint8_t version_number;
  int current_next_indicator;
  uint programs; // count of programs
  uint programs_added;
} mpeg2t_pas_t;

typedef struct mpeg2t_pmap_t {
  mpeg2t_pid_t pid;
  uint16_t program_number;
  bool received;
  uint8_t version_number;
  uint8_t *prog_info;
  uint32_t prog_info_len;
} mpeg2t_pmap_t;

/*
 * Elementary stream type PID.  Contains enough data for 
 * audio and video frame-izing (breaking transport stream up
 * into decodable frames)
 */
typedef struct mpeg2t_es_t {
  mpeg2t_pid_t pid;
  uint8_t stream_type;
  uint32_t es_info_len;
  uint8_t *es_data;
  uint16_t prog_num;
  uint8_t stream_id;
  mpeg2t_frame_t *work;
  uint32_t work_max_size;
  int work_state;
  uint32_t work_loaded;
  mpeg2t_frame_t *list;
  uint8_t left_buff[12];
  int peshdr_loaded;
  int left;
  int have_ps_ts;
  int have_dts;
  uint64_t ps_ts;
  uint64_t dts;
  uint32_t header;
  int have_seq_header;
  uint32_t seq_header_offset;
  uint32_t pict_header_offset;
  SDL_mutex *list_mutex;
  int info_loaded;           // 1 if video info or audio info is valid
  int is_video;              // 1 if video, 0 if audio
  int h, w;                  // video info
  double frame_rate;         // video info
  uint32_t tick_per_frame;   // video info
  int mpeg_layer;
  double bitrate;
  uint16_t sample_freq;      // audio info
  uint16_t sample_per_frame; // audio info
  int audio_chans;           // audio info
  int save_frames;           // set this to save frames
  int report_psts;
  int frames_in_list;
} mpeg2t_es_t;

#define MPEG2T_STREAM_11172_VIDEO 0x01
#define MPEG2T_STREAM_13818_VIDEO 0x02
#define MPEG2T_STREAM_MPEG_VIDEO 0x03
#define MPEG2T_STREAM_H264 0x1b

typedef struct mpeg2t_t {
  mpeg2t_pas_t pas;
  int program_count;
  int program_maps_recvd;
  SDL_mutex *pid_mutex;
  int save_frames_at_start;
  int have_initial_psts;
  uint64_t initial_psts;
  mpeg2t_unk_pid_t *unk_pids;
} mpeg2t_t;

 #ifdef __cplusplus 
extern "C" {
#endif
/*
 * These functions will give transport packet header information
 */
uint32_t mpeg2t_find_sync_byte(const uint8_t *buffer, uint32_t buflen);
uint32_t mpeg2t_transport_error_indicator(const uint8_t *pHdr);
uint32_t mpeg2t_payload_unit_start_indicator(const uint8_t *pHdr);
uint16_t mpeg2t_pid(const uint8_t *pHdr);
uint32_t mpeg2t_adaptation_control(const uint8_t *pHdr);
uint32_t mpeg2t_continuity_counter(const uint8_t *pHdr);

const uint8_t *mpeg2t_transport_payload_start(const uint8_t *pHdr, 
					      uint32_t *payload_len);

/*
 * create_mpeg2_transport - create the frame-izing client structure
 * This structure stores the pid list
 */
mpeg2t_t *create_mpeg2_transport(void);

void delete_mpeg2t_transport(mpeg2t_t *ptr);
/*
 * mpeg2t_process_buffer - process a received buffer of len buflen
 * It will return a pid that we've completed processing on a packet
 * or frame.  For instance, when we've read a PMAP, or read an audio
 * frame.  Buflen_used might be less than buflen - need to loop
 * accordingly.
 */
mpeg2t_pid_t *mpeg2t_process_buffer(mpeg2t_t *ptr, 
				    const uint8_t *buffer, 
				    uint32_t buflen,
				    uint32_t *buflen_used);

/*
 * mpeg2t_get_es_list_head - read the first frame off the given
 * es pid pointer
 */
mpeg2t_frame_t *mpeg2t_get_es_list_head(mpeg2t_es_t *es_pid);
  void mpeg2t_free_frame(mpeg2t_frame_t *fptr);

/*
 * mpeg2t_set_loglevel, mpeg2t_set_error_func - set the log level
 * and the error function
 */
void mpeg2t_set_loglevel(int loglevel);
void mpeg2t_set_error_func(error_msg_func_t func);

/*
 * mpeg2t_lookup_pid - get the pid pointer for the given PID
 */
mpeg2t_pid_t *mpeg2t_lookup_pid(mpeg2t_t *ptr,uint16_t pid);
/*
 * mpeg2t_es_[set | get]_userdata - set/get a userdata value for 
 * the elementary stream.
 */
void mpeg2t_set_userdata(mpeg2t_pid_t *es_pid, void *data);
void *mpeg2t_get_userdata(mpeg2t_pid_t *es_pid);

/*
 * mpeg2t_[start | stop]_saving_frames - start or stop saving
 * frames for an elementary stream.  Most streams will still be
 * processed and the data for each frame saved; however, only
 * streams that indicate that they are to be saved will save the
 * data so it can be retrieved using mpeg2t_get_es_list_head().
 */
#define MPEG2T_PID_NOTHING 0
#define MPEG2T_PID_REPORT_PSTS 1
#define MPEG2T_PID_SAVE_FRAME 2
#define MPEG2T_PID_EVERYTHING (MPEG2T_PID_REPORT_PSTS | MPEG2T_PID_SAVE_FRAME)

void mpeg2t_set_frame_status(mpeg2t_es_t *es_pid, uint32_t status);

  void mpeg2t_clear_frames (mpeg2t_es_t *es_pid);


int mpeg2t_write_stream_info(mpeg2t_es_t *es_pid, 
			     char *buffer,
			     size_t buflen);
#ifdef __cplusplus
}
#endif
#endif
/* end file mpeg2_transport.h */
