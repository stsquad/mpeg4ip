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
 * Copyright (C) Cisco Systems Inc. 2002-2003.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
#ifndef __RTP_PLUGIN_H__
#define __RTP_PLUGIN_H__

#include <mpeg4ip.h>
#include <sdp.h>
#include <mpeg4ip_config_set.h>
#include "codec_plugin.h"

struct rtp_packet;

/*
 * API routines for plugin.  These allow access to RTP data structures
 * you might need
 */

/*
 * rtp_ts_to_msec - used to calculate the msec timestamp to be used from an
 * rtp timestamp.
 * ifptr - ifptr from rtp_plugin_data_t
 * rtp_ts - RTP timestamp to convert
 * ts - timestamp packet received (rtp_pak->pd.rtp_pd_timestamp)
 * just_checking - call this with 0 when displaying information, or checking
 *    on future.  Call this with a 1 when passing back values
 */
typedef uint64_t (*rtp_ts_to_msec_f)(void *ifptr, 
				     uint32_t rtp_ts, 
				     uint64_t ts, 
				     int just_checking);

/*
 * get_next_pak - gets rtp_pak from input queue.  Input queue is sorted by
 * sequence number
 * ifptr - same as above
 * current - pointer to list (allows a look further into list without removing)
 * rm_from_list - 1 to remove, 0 to keep on list.
 */
typedef rtp_packet *(*get_next_pak_f)(void *ifptr, 
				      rtp_packet *current, 
				      int rm_from_list);

/*
 * remove_from_list - removes packet from list
 * ifptr - same as above
 * pak - rtp_packet to remove
 */
typedef void (*remove_from_list_f)(void *ifptr, 
				   rtp_packet *pak);

/*
 * free_pak - free rtp packet
 */
typedef void (*free_pak_f)(rtp_packet *pak);

/*
 * find_mbit - see if m-bit is set in the packet queue
 */
typedef bool (*find_mbit_f)(void *ifptr);

/*
 * get_head_and_check - should be used if you want to check sequence
 * numbers and rtp timestamps.  fail_if_not should be true to return
 * NULL if the sequence number and rtp_ts doesn't match what is expected
 * (sequence number has the next value, and the packet rtp timestamp matches
 * rtp_ts.
 * If it returns a value, it will remove it from the rtp queue.
 * This should be used with fail_if_not true for the first packet read in a 
 * frame, and true for subsequent calls with the same frame.
 */
typedef rtp_packet *(*get_head_and_check_f)(void *ifptr, 
					    bool fail_if_not,
					    uint32_t rtp_ts);
/*
 * VFT (virtual function table) that allows callbacks from plugins 
 */
typedef struct rtp_vft_t {
  lib_message_func_t log_msg;          // display output on console
  rtp_ts_to_msec_f   rtp_ts_to_msec;
  get_next_pak_f     get_next_pak;
  remove_from_list_f remove_from_list;
  free_pak_f         free_pak;
  CConfigSet         *pConfig;
  find_mbit_f        find_mbit;
  get_head_and_check_f get_head_and_check;
} rtp_vft_t;

/*
 * This needs to be at the beginning of your data structure, which usually
 * looks like:
 * typedef struct my_rtp_plugin_data_t {
 *    rtp_plugin_data_t rtp_data;
 *    int my_data;
 *    < rest of private data>
 * } my_rtp_plugin_data_t;
 *
 * store your data this way, instead of static variables.
 */
typedef struct rtp_plugin_data_t {
  void *ifptr;
  rtp_vft_t *vft;
} rtp_plugin_data_t;

/*
 * APIs that need to be filled in by plugin
 */

typedef enum {
  RTP_PLUGIN_NO_MATCH,
  RTP_PLUGIN_MATCH,
  RTP_PLUGIN_MATCH_USE_VIDEO_DEFAULT,
  RTP_PLUGIN_MATCH_USE_AUDIO_DEFAULT,
} rtp_check_return_t;

/*
 * rtp_plugin_check - see if this is the plugin
 * Inputs:
 *   msg - use to output any console messages
 *   fptr - pointer to SDP data (see sdp.h)
 *   rtp_payload_type - payload type received.
 *   pConfig - configuration set values
 * Outputs:
 *   see rtp_check_return_t enum.  AUDIO_DEFAULT indicates no 
 *   rtp payload header.  VIDEO_DEFAULT indicates video frames assembled
 *   with M=0 followed by M=1 as last packet with no rtp payload processing
 */
typedef rtp_check_return_t (*rtp_plugin_check_f)(lib_message_func_t msg,
						 format_list_t *fptr,
						 uint8_t rtp_payload_type,
						 CConfigSet *pConfig);
/*
 * rtp_plugin_create - create private data needed by RTP plugin
 * Inputs:
 *   fptr - pointer to SDP for this media
 *   rtp_payload_type - value of RTP payload received
 *   rtp_vft - VFT to be used for callbacks
 *   ifptr - ifptr to pass to API routines.
 * Outputs: 
 *   returns pointer to private data structure (see rtp_plugin_data_t above)
 */
typedef rtp_plugin_data_t *(*rtp_plugin_create_f)(format_list_t *fptr, 
						  uint8_t rtp_payload_type,
						  rtp_vft_t *rtp_vft,
						  void *ifptr);

/*
 * rtp_plugin_destroy - clean up routine. Clean up any data allocated in
 * private data
 */
typedef void (*rtp_plugin_destroy_f)(rtp_plugin_data_t *your_data);

/*
 * rtp_plugin_start_next_frame - called when RTP plugin should return the
 * next frame to be decoded.
 * Inputs:
 *  your_data - rtp_plugin data
 *  buffer - return point for buffer pointer.  Note: memory does not get freed.
 *  buflen - return point for frame buffer length 
 *  ts - frame timestamp structure
 *  user_data - informational data to pass to a knowledgeable decode plugin
 *              an example might be the RTP payload header for mpeg2, passed
 *              to a plugin that could decode part of a frame.
 * Outputs:
 *  true - frame is valid, false for no frame.
 */
typedef bool (*rtp_plugin_start_next_frame_f)(rtp_plugin_data_t *your_data,
					      uint8_t **buffer,
					      uint32_t *buflen, 
					      frame_timestamp_t *ts,
					      void **user_data);

/*
 * rtp_plugin_used_bytes_for_frame - indicates how many bytes out of frame
 * were used by the decoder.  Can mostly be ignored by frame based codecs.
 * Inputs:
 *  your_data
 *  bytes - number of bytes processed
 */
typedef void (*rtp_plugin_used_bytes_for_frame_f)(rtp_plugin_data_t *your_data,
						  uint32_t bytes);

/*
 * rtp_plugin_reset - called during reset events like pause or seek.
 */
typedef void (*rtp_plugin_reset_f)(rtp_plugin_data_t *your_data);

/*
 * rtp_plugin_flush - called to indicate to flush any local data
 */
typedef void (*rtp_plugin_flush_f)(rtp_plugin_data_t *your_data);

/*
 * rtp_plugin_have_frame - return true if there is a frame to use.
 */
typedef bool (*rtp_plugin_have_frame_f)(rtp_plugin_data_t *your_data);

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
  rtp_plugin_have_frame_f           rtp_plugin_have_frame;
  SConfigVariable                  *rtp_variable_list;
  uint32_t                          rtp_variable_list_count;
} rtp_plugin_t;
  
#define RTP_PLUGIN_VERSION "0.3"
#define HAVE_RTP_PLUGIN_VERSION_0_3 1
#define HAVE_RTP_PLUGIN_VERSION_0_2 1
#ifndef DLL_EXPORT
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif
#endif

#define RTP_PLUGIN_EXPORT_NAME mpeg4ip_rtp_plugin
#define RTP_PLUGIN_EXPORT_NAME_STR "mpeg4ip_rtp_plugin"

/*
 * RTP_PLUGIN - this is the macro to include with your plugin
 */
#define RTP_PLUGIN(name, \
                   check, \
                   create, \
                   destroy,\
                   snf, \
                   ubff, \
                   reset, \
                   flush, \
                   hf, \
                   var, \
                   varlen) \
extern "C" { rtp_plugin_t DLL_EXPORT RTP_PLUGIN_EXPORT_NAME = { \
   name, \
   RTP_PLUGIN_VERSION, \
   check, create, destroy, snf, ubff, reset, flush, hf, var, varlen,\
}; }
                   
#endif
