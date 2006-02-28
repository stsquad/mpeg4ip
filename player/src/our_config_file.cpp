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
 *              Peter Maersk-Moller peter @maersk-moller.net
 */

/*
 * config_file.cpp - a fairly simple config file reader.  It will read
 * either strings or ints out of .gmp4player_rc (or a specified file)
 */
#define DECLARE_CONFIG_VARIABLES
#include "our_config_file.h"
#include "player_util.h"

// Okay - this is an ugly global...

// Basic list of available config variables.  Future work could make this
// dynamic, but I'm not going to bother right now...
static const SConfigVariable MyConfigVariables[] = {
  CONFIG_STRING(CONFIG_PREV_FILE_0, "File0", NULL),
  CONFIG_STRING(CONFIG_PREV_FILE_1, "File1", NULL),
  CONFIG_STRING(CONFIG_PREV_FILE_2, "File2", NULL),
  CONFIG_STRING(CONFIG_PREV_FILE_3, "File3", NULL),
  CONFIG_BOOL_HELP(CONFIG_LOOPED, "Looped", false, "Replay video"),
  CONFIG_INT_HELP(CONFIG_VOLUME, "Volume", 75, "Set Volume (0 - 100)"),
  CONFIG_BOOL_HELP(CONFIG_MUTED, "AudioMuted", false, "Mute Volume"),
  CONFIG_INT(CONFIG_HTTP_DEBUG, "HttpDebug", LOG_ALERT),
  CONFIG_INT(CONFIG_RTSP_DEBUG, "RtspDebug", LOG_ALERT),
  CONFIG_INT(CONFIG_SDP_DEBUG, "SdpDebug", LOG_ALERT),
  CONFIG_INT_HELP(CONFIG_IPPORT_MIN, "RtpIpPortMin", UINT32_MAX, "Set minimum IP port to receive on"),
  CONFIG_INT_HELP(CONFIG_IPPORT_MAX, "RtpIpPortMax", UINT32_MAX, "Set maximum IP port to receive on"),
  CONFIG_INT(CONFIG_USE_OLD_MP4_LIB, "UseOldMp4Lib", 0),
  CONFIG_INT_HELP(CONFIG_USE_RTP_OVER_RTSP, "UseRtpOverRtsp", 0, "Use RTP in TCP session (for firewalls)"),
  CONFIG_INT(CONFIG_SEND_RTCP_IN_RTP_OVER_RTSP, "SendRtcpInRtpOverRtsp", 0), 
  CONFIG_STRING(CONFIG_PREV_DIRECTORY, "PrevDirectory", NULL),
  CONFIG_INT(CONFIG_RTP_DEBUG, "RtpDebug", LOG_ALERT),
  CONFIG_INT(CONFIG_PLAY_AUDIO, "PlayAudio", 1),
  CONFIG_INT(CONFIG_PLAY_VIDEO, "PlayVideo", 1),
  CONFIG_INT(CONFIG_PLAY_TEXT, "PlayText", 1),
  CONFIG_INT(CONFIG_RTP_BUFFER_TIME_MSEC, "RtpBufferTimeMsec", 2000),
  CONFIG_INT_HELP(CONFIG_MPEG2T_PAM_WAIT_SECS, "Mpeg2tPamWaitSecs", 30, "Time to wait for Program Map (seconds)"),
  CONFIG_INT(CONFIG_LIMIT_AUDIO_SDL_BUFFER, "LimitAudioSdlBuffer", 0),
  CONFIG_INT(CONFIG_MPEG2T_DEBUG, "Mpeg2tDebug", LOG_ALERT),
  CONFIG_INT(CONFIG_ASPECT_RATIO, "AspectRatio", 0),
  CONFIG_BOOL(CONFIG_FULL_SCREEN, "FullScreen", false),
  CONFIG_STRING_HELP(CONFIG_MULTICAST_RX_IF, "MulticastIf", NULL, "Physical interface to receive multicast"),
  CONFIG_INT_HELP(CONFIG_RX_SOCKET_SIZE, "RxSocketSize", 0, "Default receive buffer socket size"),
  CONFIG_STRING(CONFIG_MULTICAST_SRC, "MulticastSrc", NULL),
  CONFIG_STRING(CONFIG_LOG_FILE, "LogFile", NULL),
  CONFIG_BOOL_HELP(CONFIG_DISPLAY_DEBUG, "DisplayDebug", false, "In gmp4player, display status information every second"),
  CONFIG_INT(CONFIG_MPEG2PS_DEBUG, "Mpeg2psDebug", LOG_ALERT),
  CONFIG_STRING_HELP(CONFIG_URL_EXEC, "UrlExec", NULL, 
		     "Enter full path to browser launch - default /usr/bin/firefox/firefox"),
  CONFIG_BOOL(CONFIG_SHORT_VIDEO_RENDER, "ShortVideoRender", false),
  CONFIG_STRING_HELP(CONFIG_RTSP_PROXY_ADDR, "RtspProxyAddr", NULL, 
		     "RTSP proxy address"),
  CONFIG_INT(CONFIG_RTSP_PROXY_PORT, "RtspProxyPort", 0),
  CONFIG_STRING(CONFIG_OPENIPMPDRM_XMLFILE, "drmXML", NULL),
  CONFIG_STRING(CONFIG_OPENIPMPDRM_SENSITIVE, "drmInfo", NULL),
};

CConfigSet config(MyConfigVariables, 
		  sizeof(MyConfigVariables) / sizeof(*MyConfigVariables), 
		  ".gmp4player");
/* end file config_file.cpp */
