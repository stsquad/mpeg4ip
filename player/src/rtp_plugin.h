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
 *              Bill May        wmay@cisco.com
 */
#ifndef __RTP_PLUGIN_H__
#define __RTP_PLUGIN_H__

#include <systems.h>
#include <sdp/sdp.h>

struct rtp_packet;

typedef uint64_t (*rtp_ts_to_msec_f)(void *ifptr, 
				     uint32_t rtp_ts, 
				     int just_checking);

typedef rtp_packet *(*get_next_pak_f)(void *ifptr, 
				     rtp_packet *current, 
				     int rm_from_list);

typedef void (*remove_from_list_f)(void *ifptr, 
				   rtp_packet *pak);

typedef void (*free_pak_f)(rtp_packet *pak);

typedef struct rtp_vft_t {
  lib_message_func_t log_msg;
  rtp_ts_to_msec_f   rtp_ts_to_msec;
  get_next_pak_f     get_next_pak;
  remove_from_list_f remove_from_list;
  free_pak_f         free_pak;
} rtp_vft_t;

typedef enum {
  RTP_PLUGIN_NO_MATCH,
  RTP_PLUGIN_MATCH,
  RTP_PLUGIN_MATCH_USE_VIDEO_DEFAULT,
  RTP_PLUGIN_MATCH_USE_AUDIO_DEFAULT,
} rtp_check_return_t;

typedef struct rtp_plugin_data_t {
  void *ifptr;
  rtp_vft_t *vft;
} rtp_plugin_data_t;

typedef rtp_check_return_t (*rtp_plugin_check_f)(lib_message_func_t msg,
						 format_list_t *fptr,
						 uint8_t rtp_payload_type);
typedef rtp_plugin_data_t *(*rtp_plugin_create_f)(format_list_t *fptr, 
						  uint8_t rtp_payload_type,
						  rtp_vft_t *rtp_vft,
						  void *ifptr);

typedef void (*rtp_plugin_destroy_f)(rtp_plugin_data_t *your_data);

typedef uint64_t (*rtp_plugin_start_next_frame_f)(rtp_plugin_data_t *your_data,
						  uint8_t **buffer,
						  uint32_t *buflen, 
						  void **user_data);

typedef void (*rtp_plugin_used_bytes_for_frame_f)(rtp_plugin_data_t *your_data,
						  uint32_t bytes);

typedef void (*rtp_plugin_reset_f)(rtp_plugin_data_t *your_data);

typedef void (*rtp_plugin_flush_f)(rtp_plugin_data_t *your_data);

typedef int (*rtp_plugin_have_no_data_f)(rtp_plugin_data_t *your_data);

typedef struct rtp_plugin_t {
  const char *name;
  const char *version;
  rtp_plugin_check_f                rtp_plugin_check;
  rtp_plugin_create_f               rtp_plugin_create;
  rtp_plugin_destroy_f              rtp_plugin_destroy;
  rtp_plugin_start_next_frame_f     rtp_plugin_start_next_frame;
  rtp_plugin_used_bytes_for_frame_f rtp_plugin_used_bytes_for_frame;
  rtp_plugin_reset_f                rtp_plugin_reset;
  rtp_plugin_flush_f                rtp_plugin_flush;
  rtp_plugin_have_no_data_f         rtp_plugin_have_no_data;
} rtp_plugin_t;
  
#define RTP_PLUGIN_VERSION "0.1"
#ifndef DLL_EXPORT
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif
#endif

#define RTP_PLUGIN_EXPORT_NAME mpeg4ip_rtp_plugin
#define RTP_PLUGIN_EXPORT_NAME_STR "mpeg4ip_rtp_plugin"

#define RTP_PLUGIN(name, \
                   check, \
                   create, \
                   destroy,\
                   snf, \
                   ubff, \
                   reset, \
                   flush, \
                   hnd) \
extern "C" { rtp_plugin_t DLL_EXPORT RTP_PLUGIN_EXPORT_NAME = { \
   name, \
   RTP_PLUGIN_VERSION, \
   check, create, destroy, snf, ubff, reset, flush, hnd, \
}; }
                   
#endif
