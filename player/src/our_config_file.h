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
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter @maersk-moller.net
 */

#ifndef __OUR_CONFIG_FILE_H__
#define __OUR_CONFIG_FILE_H__ 1

#include <mpeg4ip_config_set.h>
DECLARE_CONFIG(CONFIG_PREV_FILE_0);
DECLARE_CONFIG(CONFIG_PREV_FILE_1);
DECLARE_CONFIG(CONFIG_PREV_FILE_2);
DECLARE_CONFIG(CONFIG_PREV_FILE_3);
DECLARE_CONFIG(CONFIG_LOOPED);
DECLARE_CONFIG(CONFIG_VOLUME);
DECLARE_CONFIG(CONFIG_MUTED);
DECLARE_CONFIG(CONFIG_HTTP_DEBUG);
DECLARE_CONFIG(CONFIG_RTSP_DEBUG);
DECLARE_CONFIG(CONFIG_SDP_DEBUG);
DECLARE_CONFIG(CONFIG_IPPORT_MIN);
DECLARE_CONFIG(CONFIG_IPPORT_MAX);
DECLARE_CONFIG(CONFIG_USE_OLD_MP4_LIB);
DECLARE_CONFIG(CONFIG_USE_RTP_OVER_RTSP);
DECLARE_CONFIG(CONFIG_SEND_RTCP_IN_RTP_OVER_RTSP);
DECLARE_CONFIG(CONFIG_PREV_DIRECTORY);
DECLARE_CONFIG(CONFIG_RTP_DEBUG);
DECLARE_CONFIG(CONFIG_PLAY_AUDIO);
DECLARE_CONFIG(CONFIG_PLAY_VIDEO);
DECLARE_CONFIG(CONFIG_PLAY_TEXT);
DECLARE_CONFIG(CONFIG_RTP_BUFFER_TIME_MSEC);
DECLARE_CONFIG(CONFIG_MPEG2T_PAM_WAIT_SECS);
DECLARE_CONFIG(CONFIG_MPEG2T_DEBUG);
DECLARE_CONFIG(CONFIG_LIMIT_AUDIO_SDL_BUFFER);
DECLARE_CONFIG(CONFIG_ASPECT_RATIO);
DECLARE_CONFIG(CONFIG_FULL_SCREEN);
DECLARE_CONFIG(CONFIG_MULTICAST_RX_IF);
DECLARE_CONFIG(CONFIG_RX_SOCKET_SIZE);
DECLARE_CONFIG(CONFIG_MULTICAST_SRC);
DECLARE_CONFIG(CONFIG_LOG_FILE);
DECLARE_CONFIG(CONFIG_DISPLAY_DEBUG);
DECLARE_CONFIG(CONFIG_MPEG2PS_DEBUG);
DECLARE_CONFIG(CONFIG_URL_EXEC);
DECLARE_CONFIG(CONFIG_SHORT_VIDEO_RENDER);
DECLARE_CONFIG(CONFIG_RTSP_PROXY_ADDR);
DECLARE_CONFIG(CONFIG_RTSP_PROXY_PORT);
DECLARE_CONFIG(CONFIG_OPENIPMPDRM_XMLFILE);
DECLARE_CONFIG(CONFIG_OPENIPMPDRM_SENSITIVE);

extern CConfigSet config;

#define get_config_string GetStringValue
#define get_config_value  GetIntegerValue
#define set_config_value  SetIntegerValue
#define set_config_string SetStringValue
#endif
