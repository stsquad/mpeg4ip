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
#ifndef __MEDIA_UTILS_H__
#define __MEDIA_UTILS_H__ 1

#include "video.h"
#include "our_bytestream.h"
#include "codec/codec.h"
extern CIpPort *global_invalid_ports;

int parse_name_for_session(CPlayerSession *psptr,
			   const char *name,
			   const char **errmsg);

int lookup_audio_codec_by_name(const char *name);
int lookup_video_codec_by_name(const char *name);

CCodecBase *start_audio_codec(const char *codec_name,
			      CAudioSync *audio_sync,
			      CInByteStreamBase *pbytestream,
			      format_list_t *media_fmt,
			      audio_info_t *aud,
			      const unsigned char *userdata,
			      uint32_t userdata_size);

CCodecBase *start_video_codec(const char *codec_name,
			      CVideoSync *video_sync,
			      CInByteStreamBase *pbytestream,
			      format_list_t *media_fmt,
			      video_info_t *vid,
			      const unsigned char *userdata,
			      uint32_t userdata_size);

CRtpByteStreamBase *create_rtp_byte_stream_for_format(format_list_t *fmt,
						      unsigned int rtp_proto,
						      int ondemand,
						      uint64_t tps,
						      rtp_packet **head, 
						      rtp_packet **tail,
						      int rtpinfo_received,
						      uint32_t rtp_rtptime,
						      int rtcp_received,
						      uint32_t ntp_frac,
						      uint32_t ntp_sec,
						      uint32_t rtp_ts);
#endif
