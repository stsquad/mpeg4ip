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

#ifndef __OUR_CONFIG_FILE_H__
#define __OUR_CONFIG_FILE_H__ 1

#include <config_file/config_file.h>
#define CONFIG_USE_MPEG4_ISO_ONLY 0
#define CONFIG_PREV_FILE_0 1
#define CONFIG_PREV_FILE_1 2
#define CONFIG_PREV_FILE_2 3
#define CONFIG_PREV_FILE_3 4
#define CONFIG_RTP_BUFFER_TIME 5
#define CONFIG_LOOPED 6
#define CONFIG_VOLUME 7
#define CONFIG_MUTED 8
#define CONFIG_HTTP_DEBUG 9
#define CONFIG_RTSP_DEBUG 10
#define CONFIG_SDP_DEBUG 11
#define CONFIG_MAX 12

extern CConfig config;

#endif
