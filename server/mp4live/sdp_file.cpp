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
#include "mp4.h"
#include "mp4av.h"

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
	sdp.session_name = pConfig->GetStringValue(CONFIG_SDP_FILE_NAME);

	bool destIsMcast = false;		// Multicast
	bool destIsSSMcast = false;		// Single Source Multicast
	struct in_addr in;
	struct in6_addr in6;

	char* sDestAddr = 
		pConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS);

	if (inet_aton(sDestAddr, &in)) {
		sdp.session_connect.conn_type = "IP4";
		destIsMcast = IN_MULTICAST(ntohl(in.s_addr));
		if ((ntohl(in.s_addr) >> 24) == 232) {
			destIsSSMcast = true;
		}
	} else if (inet_pton(AF_INET6, sDestAddr, &in6)) {
		sdp.session_connect.conn_type = "IP6";
		destIsMcast = IN6_IS_ADDR_MULTICAST(&in6);
	}

	// c=
	sdp.session_connect.conn_addr = sDestAddr;
	if (destIsMcast) {
		sdp.session_connect.ttl = 
			pConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL);
	}
	sdp.session_connect.used = 1;

	// Since we currently don't do anything with RTCP RR's
	// and they create unnecessary state in the routers
	// tell clients not to generate them
	struct bandwidth_t bandwidth;
	memset(&bandwidth, 0, sizeof(bandwidth));
	sdp.session_bandwidth = &bandwidth;
	bandwidth.modifier = BANDWIDTH_MODIFIER_USER; 
	bandwidth.bandwidth = 0;
	bandwidth.user_band = "RR";

	// if SSM, add source filter attribute
	if (destIsSSMcast) {
		char sIncl[64];

		sprintf(sIncl, "a=incl:IN IP4 %s %s",
			pConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS),
			sIpAddress);

		sdp_add_string_to_list(&sdp.unparsed_a_lines, sIncl);
	}

	bool audioIsAac =
		!strcasecmp(pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
			AUDIO_ENCODING_AAC);

	// if ISMA compliant (no audio or AAC audio), add that info
	if (!pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE) || audioIsAac) {
		sdp_add_string_to_list(&sdp.unparsed_a_lines,
			"a=isma-compliance:1,1.0,1");
	}

	u_int8_t videoProfile = 0xFF;

	if (pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		videoProfile = 
			pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_ID);
	}

	u_int8_t audioProfile = 0xFF;

	if (pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		if (audioIsAac) {
			audioProfile = 0x0F;
		} else {
			audioProfile = 0xFE;
		}
	}

	u_int8_t* pAudioConfig = NULL;
	u_int32_t audioConfigLength = 0;

	if (audioIsAac) {
		MP4AV_AacGetConfiguration(
			&pAudioConfig,
			&audioConfigLength,
			MP4AV_AAC_LC_PROFILE,
			pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE),
			pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS));
	}

	char* iod =
		MP4MakeIsmaSdpIod(
			videoProfile,
			pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE),
			pConfig->m_videoMpeg4Config,
			pConfig->m_videoMpeg4ConfigLength,
			audioProfile,
			pConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE),
			pAudioConfig,
			audioConfigLength,
			0 /* DEBUG MP4_DETAILS_ISMA */);

	if (iod) {
		sdp_add_string_to_list(&sdp.unparsed_a_lines, iod);
		free(iod);
	}

	// m=	
	media_desc_t sdpMediaVideo;
	format_list_t sdpMediaVideoFormat;
	rtpmap_desc_t sdpVideoRtpMap;
	char videoFmtpBuf[512];
	bandwidth_t videoBandwidth;

	if (pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		memset(&sdpMediaVideo, 0, sizeof(sdpMediaVideo));

		sdp.media = &sdpMediaVideo;
		sdpMediaVideo.parent = &sdp;

		sdpMediaVideo.media = "video";
		sdpMediaVideo.port = 
			pConfig->GetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT);
		sdpMediaVideo.proto = "RTP/AVP";

		sdpMediaVideo.fmt = &sdpMediaVideoFormat;
		memset(&sdpMediaVideoFormat, 0, sizeof(sdpMediaVideoFormat));
		sdpMediaVideoFormat.media = &sdpMediaVideo;
		sdpMediaVideoFormat.fmt = "96";
	
		memset(&sdpVideoRtpMap, 0, sizeof(sdpVideoRtpMap));
		sdpVideoRtpMap.encode_name = "MP4V-ES";
		sdpVideoRtpMap.clock_rate = 90000;

		sdpMediaVideoFormat.rtpmap = &sdpVideoRtpMap;

		char* sConfig = MP4BinaryToBase16(
			pConfig->m_videoMpeg4Config, 
			pConfig->m_videoMpeg4ConfigLength); 

		sprintf(videoFmtpBuf, 
			"profile-level-id=%u; config=%s;",
			pConfig->GetIntegerValue(CONFIG_VIDEO_PROFILE_LEVEL_ID),
			sConfig); 
		free(sConfig);

		sdpMediaVideoFormat.fmt_param = videoFmtpBuf;

		memset(&videoBandwidth, 0, sizeof(videoBandwidth));
		sdpMediaVideo.media_bandwidth = &videoBandwidth;
		videoBandwidth.modifier = BANDWIDTH_MODIFIER_AS; 
		videoBandwidth.bandwidth =
			pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE);
	}

	media_desc_t sdpMediaAudio;
	format_list_t sdpMediaAudioFormat;
	rtpmap_desc_t sdpAudioRtpMap;
	bandwidth_t audioBandwidth;

	char audioFmtpBuf[512];

	if (pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		if (sdp.media) {
			sdp.media->next = &sdpMediaAudio;
		} else {
			sdp.media = &sdpMediaAudio;
		}
		memset(&sdpMediaAudio, 0, sizeof(sdpMediaAudio));
		sdpMediaAudio.parent = &sdp;

		sdpMediaAudio.media = "audio";
		sdpMediaAudio.port = 
			pConfig->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT);
		sdpMediaAudio.proto = "RTP/AVP";

		sdpMediaAudio.fmt = &sdpMediaAudioFormat;
		memset(&sdpMediaAudioFormat, 0, sizeof(sdpMediaAudioFormat));
		sdpMediaAudioFormat.media = &sdpMediaAudio;
		sdpMediaAudioFormat.fmt = "97";
	
		memset(&sdpAudioRtpMap, 0, sizeof(sdpAudioRtpMap));
		if (!strcasecmp(pConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
		  AUDIO_ENCODING_MP3)) {
			sdpAudioRtpMap.encode_name = "MPA";

		} else if (audioIsAac) {
			sdpAudioRtpMap.encode_name = "mpeg4-generic";

			char* sConfig = 
				MP4BinaryToBase16(pAudioConfig, audioConfigLength);

			sprintf(audioFmtpBuf,
				"streamtype=5; profile-level-id=15; mode=AAC-hbr; config=%s; "
				"SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;",
					sConfig); 
			free(sConfig);

			sdpMediaAudioFormat.fmt_param = audioFmtpBuf;
		}
		sdpAudioRtpMap.clock_rate = 
			pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE);

		sdpMediaAudioFormat.rtpmap = &sdpAudioRtpMap;

		memset(&audioBandwidth, 0, sizeof(audioBandwidth));
		sdpMediaAudio.media_bandwidth = &audioBandwidth;
		audioBandwidth.modifier = BANDWIDTH_MODIFIER_AS; 
		audioBandwidth.bandwidth =
			pConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE);
	}

	free(pAudioConfig);

	// finally call sdp library 
	// to write the entire sdp description
	// to the given sdp file name
	bool rc = (sdp_encode_one_to_file(&sdp, 
		pConfig->GetStringValue(CONFIG_SDP_FILE_NAME), 0) == 0);

	// free malloced memory inside sdp structure
	sdp_free_string_list(&sdp.unparsed_a_lines);

	return rc;
}

