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
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sdp.h"

bool GenerateSdpFile(CLiveConfig* pConfig)
{
	session_desc_t sdp;

	memset(&sdp, 0, sizeof(sdp));
	// o=
	sdp.session_id = GetTimestamp();
	sdp.session_version = GetTimestamp();
	sdp.create_addr_type = "IP4";
	
	char sHostName[256];
	char sIpAddress[16];

	if (gethostname(sHostName, sizeof(sHostName)) < 0) {
		debug_message("GenerateSdpFile: Couldn't gethostname"); 
		strcpy(sIpAddress, "0.0.0.0");
	} else {
		struct hostent* h = gethostbyname(sHostName);
		if (h == NULL) {
			debug_message("GenerateSdpFile: Couldn't gethostbyname"); 
			strcpy(sIpAddress, "0.0.0.0");
		} else {
			strcpy(sIpAddress, 
				inet_ntoa(*(struct in_addr*)(h->h_addr_list[0])));
		}
	}
	sdp.create_addr = sIpAddress;

	// s=
	sdp.session_name = pConfig->m_sdpFileName;

	bool destIsMcast = false;
	bool destIsSSMcast = false;
	struct in_addr in;

	if (inet_aton(pConfig->m_rtpDestAddress, &in)) {
		destIsMcast = IN_MULTICAST(ntohl(in.s_addr));
		if ((ntohl(in.s_addr) >> 24) == 232) {
			destIsSSMcast = true;
		}
	}

	// c=
	sdp.session_connect.conn_type = "IP4";
	sdp.session_connect.conn_addr = pConfig->m_rtpDestAddress;
	if (destIsMcast) {
		sdp.session_connect.ttl = pConfig->m_rtpMulticastTtl;
	}
	sdp.session_connect.used = 1;

	// if SSM, add source filter attribute
	struct string_list_t sdpSourceFilter;
	char sIncl[64];

	if (destIsSSMcast) {
		sdp.unparsed_a_lines = &sdpSourceFilter;
		memset(&sdpSourceFilter, 0, sizeof(sdpSourceFilter));
		sdpSourceFilter.string_val = sIncl;
		sprintf(sIncl, "a=incl:IN IP4 %s %s",
			pConfig->m_rtpDestAddress,
			sIpAddress);
	}

	// m=	
	media_desc_t sdpMediaVideo;
	format_list_t sdpMediaVideoFormat;
	rtpmap_desc_t sdpVideoRtpMap;
	char videoFmtpBuf[512];

	if (pConfig->m_videoEnable) {
		memset(&sdpMediaVideo, 0, sizeof(sdpMediaVideo));

		sdp.media = &sdpMediaVideo;
		sdpMediaVideo.parent = &sdp;

		sdpMediaVideo.media = "video";
		sdpMediaVideo.port = pConfig->m_rtpVideoDestPort;
		sdpMediaVideo.proto = "RTP/AVP";

		sdpMediaVideo.fmt = &sdpMediaVideoFormat;
		memset(&sdpMediaVideoFormat, 0, sizeof(sdpMediaVideoFormat));
		sdpMediaVideoFormat.media = &sdpMediaVideo;
		sdpMediaVideoFormat.fmt = "96";
	
		memset(&sdpVideoRtpMap, 0, sizeof(sdpVideoRtpMap));
		sdpVideoRtpMap.encode_name = "MP4V-ES";
		sdpVideoRtpMap.clock_rate = 90000;

		sdpMediaVideoFormat.rtpmap = &sdpVideoRtpMap;

		// TBD GenerateMpeg4VideoConfig(pConfig);
		char* sConfig = BinaryToAscii(pConfig->m_videoMpeg4Config, 
			pConfig->m_videoMpeg4ConfigLength); 
		sprintf(videoFmtpBuf, "profile-level-id=%u; config=%s;",
			pConfig->m_videoProfileLevelId, sConfig); 
		free(sConfig);
		sdpMediaVideoFormat.fmt_param = videoFmtpBuf;
	}

	media_desc_t sdpMediaAudio;
	format_list_t sdpMediaAudioFormat;
	rtpmap_desc_t sdpAudioRtpMap;

	if (pConfig->m_audioEnable) {
		if (sdp.media) {
			sdp.media->next = &sdpMediaAudio;
		} else {
			sdp.media = &sdpMediaAudio;
		}
		memset(&sdpMediaAudio, 0, sizeof(sdpMediaAudio));
		sdpMediaAudio.parent = &sdp;

		sdpMediaAudio.media = "audio";
		sdpMediaAudio.port = pConfig->m_rtpAudioDestPort;
		sdpMediaAudio.proto = "RTP/AVP";

		sdpMediaAudio.fmt = &sdpMediaAudioFormat;
		memset(&sdpMediaAudioFormat, 0, sizeof(sdpMediaAudioFormat));
		sdpMediaAudioFormat.media = &sdpMediaAudio;
		sdpMediaAudioFormat.fmt = "14";
	
		memset(&sdpAudioRtpMap, 0, sizeof(sdpAudioRtpMap));
		sdpAudioRtpMap.encode_name = "MPA";
		sdpAudioRtpMap.clock_rate = pConfig->m_audioSamplingRate;

		sdpMediaAudioFormat.rtpmap = &sdpAudioRtpMap;
	}

	return (sdp_encode_one_to_file(&sdp, pConfig->m_sdpFileName, 0) == 0);
}
