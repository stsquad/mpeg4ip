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
 * player_sdp.h - provide sdp translation routines we need
 */
#ifndef __PLAYER_SPD_H__
#define __PLAYER_SPD_H__ 1

#ifdef __cplusplus
extern "C" {
#endif
void convert_relative_urls_to_absolute(session_desc_t *sdp,
				       const char *base_url);

void create_rtsp_transport_from_sdp(session_desc_t *sdp,
				    media_desc_t *media,
				    in_port_t port,
				    char *buffer,
				    uint32_t buflen);
connect_desc_t *get_connect_desc_from_media(media_desc_t *media);
range_desc_t *get_range_from_media(media_desc_t *media);
range_desc_t *get_range_from_sdp(session_desc_t *sptr);

  bandwidth_t *find_bandwidth_from_media(media_desc_t *media,
					 bandwidth_modifier_t bw_type,
					 const char *user_type);
  int find_rtcp_bandwidth_from_media(media_desc_t *media, double *bw);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __PLAYER_SDP_H__
