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


#ifndef __CODEC_PLUGIN_PRIVATE_H__
#define __CODEC_PLUGIN_PRIVATE_H__

#include "codec_plugin.h"

void initialize_plugins(void);

void close_plugins(void);

codec_plugin_t *check_for_audio_codec(const char *compressor,
				      format_list_t *fptr,
				      int audio_type,
				      int profile,
				      const unsigned char *userdata,
				      uint32_t userdata_size);

codec_plugin_t *check_for_video_codec(const char *compressor,
				      format_list_t *fptr,
				      int type,
				      int profile, 
				      const unsigned char *userdata,
				      uint32_t userdata_size);
class CPlayerSession;

int audio_codec_check_for_raw_file (CPlayerSession *psptr,
				    const char *name);
int video_codec_check_for_raw_file (CPlayerSession *psptr,
				    const char *name);
#endif
