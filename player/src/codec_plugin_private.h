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
 * Copyright (C) Cisco Systems Inc. 2002-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */


#ifndef __CODEC_PLUGIN_PRIVATE_H__
#define __CODEC_PLUGIN_PRIVATE_H__

#include "codec_plugin.h"
#include "rtp_plugin.h"

  void initialize_plugins(CConfigSet *pConfig);

  void close_plugins(void);

  codec_plugin_t *check_for_audio_codec(const char *stream_type,
					const char *compressor,
					format_list_t *fptr,
					int audio_type,
					int profile,
					const uint8_t *userdata,
					uint32_t userdata_size,
					CConfigSet *pConfig);

codec_plugin_t *check_for_video_codec(const char *stream_type,
				      const char *compressor,
				      format_list_t *fptr,
				      int type,
				      int profile, 
				      const uint8_t *userdata,
				      uint32_t userdata_size,
				      CConfigSet *pConfig);
codec_plugin_t *check_for_text_codec(const char *stream_type, 
				     const char *compressor, 
				     format_list_t *fptr, 
				     const uint8_t *userdata,
				     uint32_t userdata_size,
				     CConfigSet *pConfig);

codec_data_t *audio_codec_check_for_raw_file(const char *name,
					     codec_plugin_t **codec,
					     double *maxtime,
					     char **desc,
					     CConfigSet *pConfig);

codec_data_t *video_codec_check_for_raw_file(const char *name,
					     codec_plugin_t **codec,
					     double *maxtime,
					     char **desc,
					     CConfigSet *pConfig);


// RTP plugins
rtp_check_return_t check_for_rtp_plugins (format_list_t *fptr,
					  uint8_t rtp_payload_type,
					  rtp_plugin_t **rtp_plugin,
					  CConfigSet *pConfig);
#endif
