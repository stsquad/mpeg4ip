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
#include "audio_encoder.h"
#include "video_encoder.h"
#include "text_encoder.h"
#include "media_stream.h"

//#define DEBUG_IOD
static void create_conn_for_media (media_desc_t *sdpMedia,
				   const char *sDestAddr,
				   uint32_t ttl,
				   session_desc_t *sdp)
{
  bool destIsMcast  = false;
  bool destIsSSMcast = false;
  struct in_addr in;
  struct in6_addr in6;

  if (inet_aton(sDestAddr, &in)) {
    sdpMedia->media_connect.conn_type = strdup("IP4");
    destIsMcast = IN_MULTICAST(ntohl(in.s_addr));
    if ((ntohl(in.s_addr) >> 24) == 232) {
      destIsSSMcast = true;
    }
  } else if (inet_pton(AF_INET6, sDestAddr, &in6)) {
    sdpMedia->media_connect.conn_type = strdup("IP6");
    destIsMcast = IN6_IS_ADDR_MULTICAST(&in6);
  } else {
    // this is bad - but will work for now
    // we have a domain name, or something like it.
    sdpMedia->media_connect.conn_type = strdup("IP4");
    destIsMcast = false;
  }
  // c=
  sdpMedia->media_connect.conn_addr = strdup(sDestAddr);
  if (destIsMcast) {
    sdpMedia->media_connect.ttl = ttl;
  }
  sdpMedia->media_connect.used = 1;
  if (destIsSSMcast) {
    char sIncl[64];
	  
    sprintf(sIncl, "a=incl:IN IP4 %s %s",
	    sDestAddr,
	    sdp->create_addr);
	  
    sdp_add_string_to_list(&sdpMedia->unparsed_a_lines, sIncl);
  }
}

void createStreamSdp (CLiveConfig *pGlobal,
		      CMediaStream *pStream)
{
  session_desc_t *sdp;
  bool allow_rtcp = pGlobal->GetBoolValue(CONFIG_RTP_NO_B_RR_0);

  sdp = MALLOC_STRUCTURE(session_desc_t);
  memset(sdp, 0, sizeof(*sdp));

  // o=
  sdp->session_id = GetTimestamp();
  sdp->session_version = GetTimestamp();
  sdp->create_addr_type = strdup("IP4");
  sdp->create_addr = get_host_ip_address();

	// s=
  sdp->session_name = strdup(pStream->GetStringValue(STREAM_CAPTION));
  const char *desc = pStream->GetStringValue(STREAM_DESCRIPTION);
  if (desc != NULL) {
    sdp->session_desc = strdup(desc);
  }
  bool destIsMcast = false;		// Multicast
  bool destIsSSMcast = false;		// Single Source Multicast
  struct in_addr in;
  struct in6_addr in6;
  bool do_seperate_conn;
  const char *sAudioDestAddr = pStream->GetStringValue(STREAM_AUDIO_DEST_ADDR);
  const char *sVideoDestAddr = pStream->GetStringValue(STREAM_VIDEO_DEST_ADDR);
  const char *sTextDestAddr = pStream->GetStringValue(STREAM_TEXT_DEST_ADDR);
  in_port_t audio_port = pStream->GetIntegerValue(STREAM_AUDIO_DEST_PORT);
  in_port_t video_port = pStream->GetIntegerValue(STREAM_VIDEO_DEST_PORT);
  in_port_t text_port = pStream->GetIntegerValue(STREAM_TEXT_DEST_PORT);
  uint32_t ttl = pGlobal->GetIntegerValue(CONFIG_RTP_MCAST_TTL);
  if (strcmp(sAudioDestAddr, sVideoDestAddr) == 0 &&
      strcmp(sAudioDestAddr, sTextDestAddr) == 0) {
    do_seperate_conn = false;
    if (inet_aton(sAudioDestAddr, &in)) {
      sdp->session_connect.conn_type = strdup("IP4");
      destIsMcast = IN_MULTICAST(ntohl(in.s_addr));
      if ((ntohl(in.s_addr) >> 24) == 232) {
	destIsSSMcast = true;
      }
    } else if (inet_pton(AF_INET6, sAudioDestAddr, &in6)) {
      sdp->session_connect.conn_type = strdup("IP6");
      destIsMcast = IN6_IS_ADDR_MULTICAST(&in6);
    } else {
      // this is bad - but will work for now
      // we have a domain name, or something like it.
      sdp->session_connect.conn_type = strdup("IP4");
      destIsMcast = false;
      destIsSSMcast = false;
    }
    
    // c=
    sdp->session_connect.conn_addr = strdup(sAudioDestAddr);
    if (destIsMcast) {
      sdp->session_connect.ttl = ttl;
    }
    sdp->session_connect.used = 1;
    if (destIsSSMcast) {
      char sIncl[64];
      
      sprintf(sIncl, "a=incl:IN IP4 %s %s",
	      sAudioDestAddr,
	      sdp->create_addr);
      
      sdp_add_string_to_list(&sdp->unparsed_a_lines, sIncl);
    }
  } else
    do_seperate_conn = true;

  // Since we currently don't do anything with RTCP RR's
  // and they create unnecessary state in the routers
  // tell clients not to generate them
  if (allow_rtcp == false) {
    bandwidth_t *bandwidth = MALLOC_STRUCTURE(bandwidth_t);
    memset(bandwidth, 0, sizeof(*bandwidth));
    sdp->session_bandwidth = bandwidth;
    bandwidth->modifier = BANDWIDTH_MODIFIER_USER; 
    bandwidth->bandwidth = 0;
    bandwidth->user_band = strdup("RR");
  }

  // if SSM, add source filter attribute
  
  bool audioIsIsma, videoIsIsma;
  bool audioIs3gp = false, videoIs3gp = false;
  bool createIod = true;
  u_int8_t audioProfile = 0xFF;
  u_int8_t videoProfile = 0xFF;
  u_int8_t* pAudioConfig = NULL;
  u_int32_t audioConfigLength = 0;
  u_int8_t *pVideoConfig = NULL;
  u_int32_t videoConfigLength = 0;
  
  if (pStream->GetBoolValue(STREAM_AUDIO_ENABLED)) {
    audioIsIsma = false;
    audioIs3gp = false;
    media_desc_t *sdpMediaAudio;
    bool audioCreateIod = false;
    bandwidth_t *audioBandwidth;
    
    sdpMediaAudio = create_audio_sdp(pStream->GetAudioProfile(),
				     &audioCreateIod,
				     &audioIsIsma,
				     &audioIs3gp,
				     &audioProfile,
				     &pAudioConfig,
				     &audioConfigLength);
    if (sdpMediaAudio != NULL) {
      if (audioCreateIod == false) createIod = false;
      if (sdp->media) {
	sdp->media->next = sdpMediaAudio;
      } else {
	sdp->media = sdpMediaAudio;
      }
      if (do_seperate_conn) {
	create_conn_for_media(sdpMediaAudio, sAudioDestAddr,
			      ttl, sdp);
      }
      sdpMediaAudio->parent = sdp;
      
      sdpMediaAudio->media = strdup("audio");
      sdpMediaAudio->port = audio_port;
      sdpMediaAudio->proto = strdup("RTP/AVP");
      
      audioBandwidth = MALLOC_STRUCTURE(bandwidth_t);
      memset(audioBandwidth, 0, sizeof(*audioBandwidth));
      sdpMediaAudio->media_bandwidth = audioBandwidth;
      audioBandwidth->modifier = BANDWIDTH_MODIFIER_AS; 
      audioBandwidth->bandwidth =
	(pStream->GetAudioProfile()->GetIntegerValue(CFG_AUDIO_BIT_RATE) + 999)/ 1000;
      
    }
  } else {
    audioIsIsma = true;
  }
  
  if (pStream->GetBoolValue(STREAM_VIDEO_ENABLED)) {
    media_desc_t *sdpMediaVideo;
    bandwidth_t *videoBandwidth;
    bool videoCreateIod = false;
    
    sdpMediaVideo = create_video_sdp(pStream->GetVideoProfile(),
				     &videoCreateIod,
				     &videoIsIsma,
				     &videoIs3gp,
				     &videoProfile,
				     &pVideoConfig,
				     &videoConfigLength);
    if (sdpMediaVideo != NULL) {
      if (videoCreateIod == false) createIod = false;
      
      if (do_seperate_conn) {
	create_conn_for_media(sdpMediaVideo, sVideoDestAddr,
			      ttl, sdp);
      }

      sdpMediaVideo->next = sdp->media;
      sdp->media = sdpMediaVideo;
      sdpMediaVideo->parent = sdp;
      
      sdpMediaVideo->media = strdup("video");
      sdpMediaVideo->port = video_port;
      sdpMediaVideo->proto = strdup("RTP/AVP");
      
      videoBandwidth = MALLOC_STRUCTURE(bandwidth_t);
      memset(videoBandwidth, 0, sizeof(*videoBandwidth));
      sdpMediaVideo->media_bandwidth = videoBandwidth;
      videoBandwidth->modifier = BANDWIDTH_MODIFIER_AS; 
      videoBandwidth->bandwidth =
	pStream->GetVideoProfile()->GetIntegerValue(CFG_VIDEO_BIT_RATE);
    }
  } else {
    videoIsIsma = true;
  }

  if (videoIs3gp && audioIs3gp) {
    sdp_add_string_to_list(&sdp->unparsed_a_lines, 
			   "a=range:npt=0-");
  }
  if (pStream->GetBoolValue(STREAM_TEXT_ENABLED)) {
    media_desc_t *sdpMediaText;
   
    sdpMediaText = create_text_sdp(pStream->GetTextProfile());
    if (sdp->media == NULL) {
      sdp->media = sdpMediaText;
    } else {
      media_desc_t *next = sdp->media;
      while (next->next != NULL) next = next->next;
      next->next = sdpMediaText;
    }
      
    if (sdpMediaText != NULL) {
      if (do_seperate_conn) {
	create_conn_for_media(sdpMediaText, sTextDestAddr,
			      ttl, sdp);
      }
      sdpMediaText->parent = sdp;
      sdpMediaText->port = text_port;
      sdpMediaText->proto = strdup("RTP/AVP");
    }
  }
  session_time_desc_t *sdpTime;
  sdpTime = MALLOC_STRUCTURE(session_time_desc_t);
  sdpTime->start_time = 0;
  sdpTime->end_time = 0;
  sdpTime->next = NULL;
  sdpTime->repeat = NULL;
  sdp->time_desc = sdpTime;
  
  bool added_iod = false;
  if (createIod) {
    char* iod =
      MP4MakeIsmaSdpIod(
			videoProfile,
			pStream->GetVideoProfile() == NULL ? 0 : 
			pStream->GetVideoProfile()->GetIntegerValue(CFG_VIDEO_BIT_RATE) * 1000,
			pVideoConfig,
			videoConfigLength,
			audioProfile,
			pStream->GetAudioProfile() == NULL ? 0 :
			pStream->GetAudioProfile()->GetIntegerValue(CFG_AUDIO_BIT_RATE),
			pAudioConfig,
			audioConfigLength,
#ifdef DEBUG_IOD
			pGlobal->GetBoolValue(CONFIG_APP_DEBUG) ?
			MP4_DETAILS_ISMA : 
#endif
			0
			);

    if (iod) {
      added_iod = true;
      sdp_add_string_to_list(&sdp->unparsed_a_lines, iod);
      free(iod);
    }
  }
  
  if (pAudioConfig) {
    free(pAudioConfig);
  }
  
  if (audioIsIsma && videoIsIsma && added_iod) {
    sdp_add_string_to_list(&sdp->unparsed_a_lines,
			   "a=isma-compliance:1,1.0,1");
  }
  
  sdp_encode_one_to_file(sdp, 
			 pStream->GetStringValue(STREAM_SDP_FILE_NAME), 
			 0);
  sdp_free_session_desc(sdp);

}
