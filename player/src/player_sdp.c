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
 * player_sdp.c - utilities for handling SDP structures
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sdp/sdp.h>
#include "player_sdp.h"
#include "player_util.h"

/*
 * do_relative_url_to_absolute - does the actual work to convert a
 * relative url to an absolute url.
 */
static void do_relative_url_to_absolute (const char **control_string,
					 const char *base_url)
{
  char *str;
  const char *cpystr;
  uint32_t cblen, malloclen;

  malloclen = cblen = strlen(base_url);

  if (base_url[cblen - 1] != '/') malloclen++;
  /*
   * If the control string is just a *, use the base url only
   */
  cpystr = *control_string;
  if (strcmp(cpystr, "*") != 0) {
    if (*cpystr == '/') cpystr++;

    // duh - add 1 for \0...
    str = (char *)malloc(strlen(cpystr) + malloclen + 1);
    if (str == NULL)
      return;
    strcpy(str, base_url);
    if (base_url[cblen - 1] != '/') {
      strcat(str, "/");
    }
    if (*cpystr == '/') cpystr++;
    strcat(str, cpystr);
  } else {
    str = strdup(base_url);
  }
  free((void *)*control_string);
  *control_string = str;
}

/*
 * convert_relative_urls_to_absolute - for every url inside the session
 * description, convert relative to absolute.
 */
void convert_relative_urls_to_absolute (session_desc_t *sdp,
					const char *base_url)
{
  media_desc_t *media;
  
  if (base_url == NULL)
    return;

  if ((sdp->control_string != NULL) &&
      (strncmp(sdp->control_string, "rtsp://", strlen("rtsp://"))) != 0) {
    do_relative_url_to_absolute(&sdp->control_string, base_url);
  }
  
  for (media = sdp->media; media != NULL; media = media->next) {
    if ((media->control_string != NULL) &&
	(strncmp(media->control_string, "rtsp://", strlen("rtsp://")) != 0)) {
      do_relative_url_to_absolute(&media->control_string, base_url);
    }
  }
}

/*
 * create_rtsp_transport_from_sdp - from a sdp media description, create
 * the RTSP transport string needed.
 */
void create_rtsp_transport_from_sdp (session_desc_t *sdp,
				     media_desc_t *media,
				     in_port_t port,
				     char *buffer,
				     uint32_t buflen)
{

  uint32_t ret;

  ret = snprintf(buffer, buflen, "%s;unicast;client_port=%d-%d",
		 media->proto, port, port + 1);
  
}

/*
 * get_connect_desc_from_media.  If the media doesn't have one, the
 * session_desc_t might.
 */
connect_desc_t *get_connect_desc_from_media (media_desc_t *media)
{
  session_desc_t *sptr;
  
  if (media->media_connect.used)
    return (&media->media_connect);

  sptr = media->parent;
  if (sptr == NULL) return (NULL);
  if (sptr->session_connect.used == FALSE)
    return (NULL);
  return (&sptr->session_connect);
}

/*
 * get_range_from_media.  If the media doesn't have one, the
 * session_desc_t might.
 */
range_desc_t *get_range_from_media (media_desc_t *media)
{
  session_desc_t *sptr;
  
  if (media->media_range.have_range) {
    return (&media->media_range);
  }

  sptr = media->parent;
  if (sptr == NULL || sptr->session_range.have_range == FALSE)
    return (NULL);
  return (&sptr->session_range);
}

range_desc_t *get_range_from_sdp (session_desc_t *sptr)
{
  media_desc_t *media;
  
  if (sptr == NULL)
    return (NULL);
  
  if (sptr->session_range.have_range != FALSE)
    return (&sptr->session_range);

  media = sptr->media;
  while (media != NULL) {
    if (media->media_range.have_range) {
      return (&media->media_range);
    }
    media = media->next;
  }
  return (NULL);
}

static bandwidth_t *find_bandwidth_from_bw_list (bandwidth_t *bptr,
						 bandwidth_modifier_t bw_type,
						 const char *user_type)
{
  int user_type_len = 0;

  if (bw_type == BANDWIDTH_MODIFIER_USER) {
    user_type_len = strlen(user_type);
  }
  while (bptr != NULL) {
    if (bptr->modifier == bw_type) {
      if (bptr->modifier != BANDWIDTH_MODIFIER_USER)
	return (bptr);
      if (strncasecmp(user_type, bptr->user_band, user_type_len) == 0)
	return (bptr);
    }
    bptr = bptr->next;
  }
  return (NULL);
}

bandwidth_t *find_bandwidth_from_media (media_desc_t *media,
					bandwidth_modifier_t bw_type,
					const char *user_type)
{
  bandwidth_t *bptr;
  session_desc_t *sptr;

  if (media == NULL) return NULL;

  bptr = find_bandwidth_from_bw_list(media->media_bandwidth,
				     bw_type,
				     user_type);
  if (bptr != NULL)
    return bptr;

  sptr = media->parent;
  if (sptr == NULL)
    return (NULL);
  return (find_bandwidth_from_bw_list(sptr->session_bandwidth,
				      bw_type,
				      user_type));
}

int find_rtcp_bandwidth_from_media (media_desc_t *media,
				    double *bw)
{
  bandwidth_t *bptr;
  
  *bw = 0.0;
  bptr = find_bandwidth_from_media(media, BANDWIDTH_MODIFIER_USER, "rr");
  if (bptr == NULL) {
    return -1;
  }
  *bw = (double)bptr->bandwidth;
  return 0;
}

/* end file player_sdp.c */
