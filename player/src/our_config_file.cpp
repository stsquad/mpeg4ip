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
 * config_file.cpp - a fairly simple config file reader.  It will read
 * either strings or ints out of .gmp4player_rc (or a specified file)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "our_config_file.h"
#include "player_util.h"

// Okay - this is an ugly global...

// Basic list of available config variables.  Future work could make this
// dynamic, but I'm not going to bother right now...

static config_variable_t configs[] = {
  { CONFIG_USE_MPEG4_ISO_ONLY, "Mpeg4IsoOnly", CONFIG_INT, 0, NULL },
  { CONFIG_PREV_FILE_0, "File0", CONFIG_STRING, 0, NULL },
  { CONFIG_PREV_FILE_1, "File1", CONFIG_STRING, 0, NULL },
  { CONFIG_PREV_FILE_2, "File2", CONFIG_STRING, 0, NULL },
  { CONFIG_PREV_FILE_3, "File3", CONFIG_STRING, 0, NULL },
  { CONFIG_RTP_BUFFER_TIME, "RtpBufferTime", CONFIG_INT, 2, NULL },
  { CONFIG_LOOPED, "Looped", CONFIG_INT, 0, NULL },
  { CONFIG_VOLUME, "Volume", CONFIG_INT, 75, NULL },
  { CONFIG_MUTED, "AudioMuted", CONFIG_INT, 0, NULL},
  { CONFIG_HTTP_DEBUG, "HttpDebug", CONFIG_INT, LOG_ALERT, NULL},
  { CONFIG_RTSP_DEBUG, "RtspDebug", CONFIG_INT, LOG_ALERT, NULL},
  { CONFIG_SDP_DEBUG, "SdpDebug", CONFIG_INT, LOG_ALERT, NULL},
  { CONFIG_IPPORT_MIN, "RtpIpPortMin", CONFIG_INT, -1, NULL},
  { CONFIG_IPPORT_MAX, "RtpIpPortMax", CONFIG_INT, -1, NULL},
  { CONFIG_USE_OLD_MP4_LIB, "UseOldMp4Lib", CONFIG_INT, 0, NULL},
  { CONFIG_USE_RTP_OVER_RTSP, "UseRtpOverRtsp", CONFIG_INT, 0, NULL},
  { CONFIG_SEND_RTCP_IN_RTP_OVER_RTSP, "SendRtcpInRtpOverRtsp", CONFIG_INT, 0, NULL},
  { CONFIG_PREV_DIRECTORY, "PrevDirectory", CONFIG_STRING, 0, NULL},
};

CConfig config(configs, CONFIG_MAX);
/* end file config_file.cpp */
