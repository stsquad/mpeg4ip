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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#ifndef __H264_SDP_H__
#define __H264_SDP_H__ 1

#include "mpeg4ip.h"
#include "mp4.h"
#ifdef __cplusplus
extern "C" {
#endif
uint8_t *h264_sdp_parse_sprop_param_sets(const char *fmt_param,
					 uint32_t *nal_len, 
					 lib_message_func_t message);
#ifdef __cplusplus
}
#endif
#endif
