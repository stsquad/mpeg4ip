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
/*
 * player_sdp.h - provide sdp translation routines we need
 */
#ifndef __MPEG4_SDP_H__
#define __MPEG4_SDP_H__ 1

typedef struct fmtp_parse_t {
  int stream_type;
  int profile_level_id;
  uint8_t *config_binary;
  char *config_ascii;
  uint32_t config_binary_len;
  int constant_size;
  int size_length;
  int index_length;
  int index_delta_length;
  int CTS_delta_length;
  int DTS_delta_length;
  int auxiliary_data_size_length;
  int bitrate;
  int profile;
  int ISMACrypIVDeltaLength;
} fmtp_parse_t;
  

#ifdef __cplusplus
extern "C" {
#endif
  fmtp_parse_t *parse_fmtp_for_mpeg4(const char *bptr, lib_message_func_t);
  void free_fmtp_parse(fmtp_parse_t *ptr);
#ifdef __cplusplus
}
#endif

#endif // #ifndef __MPEG4_SDP_H__
