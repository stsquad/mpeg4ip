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
void do_relative_url_to_absolute (char **control_string,
				  const char *base_url,
				  int dontfree)
{
  char *str;
  uint32_t cblen;

  cblen = strlen(base_url);
  if (base_url[cblen - 1] != '/') cblen++;
  /*
   * If the control string is just a *, use the base url only
   */
  if (strcmp(*control_string, "*") != 0) {
    if (**control_string == '/') cblen--;
    // duh - add 1 for \0...
    str = (char *)malloc(strlen(*control_string) + cblen + 1);
    if (str == NULL)
      return;
    strcpy(str, base_url);
    if (base_url[cblen - 1] != '/') {
      strcat(str, "/");
    }
    strcat(str, **control_string == '/' ?
	   *control_string + 1 : *control_string);
  } else {
    str = strdup(base_url);
  }
  if (dontfree == 0) 
    free(*control_string);
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
      (strncmp(sdp->control_string, "rtsp:://", strlen("rtsp://"))) != 0) {
    do_relative_url_to_absolute(&sdp->control_string, base_url, 0);
  }
  
  for (media = sdp->media; media != NULL; media = media->next) {
    if ((media->control_string != NULL) &&
	(strncmp(media->control_string, "rtsp://", strlen("rtsp://")) != 0)) {
      do_relative_url_to_absolute(&media->control_string, base_url, 0);
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

#define ADV_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}

#define TTYPE(a,b) {a, sizeof(a), b}

static char *fmtp_advance_to_next (char *ptr)
{
  while (*ptr != '\0' && *ptr != ';') ptr++;
  if (*ptr == ';') ptr++;
  return (ptr);
}

static char *fmtp_parse_number (char *ptr, int *ret_value)
{
  long int value;
  char *ret_ptr;
  
  value = strtol(ptr, &ret_ptr, 0);
  if ((ret_ptr != NULL) &&
      (*ret_ptr == ';' || *ret_ptr == '\0')) {
    if (*ret_ptr == ';') ret_ptr++;
    if (value > INT_MAX) value = INT_MAX;
    if (value < INT_MIN) value = INT_MIN;
    *ret_value = value;
    return (ret_ptr);
  }
  return (NULL);
}
static char *fmtp_streamtype (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->stream_type);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_profile_level_id (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->profile_level_id);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

static unsigned char to_hex (char *ptr)
{
  if (isdigit(*ptr)) {
    return (*ptr - '0');
  }
  return (tolower(*ptr) - 'a');
}

static char *fmtp_config (char *ptr, fmtp_parse_t *fptr)
{
  char *iptr;
  uint32_t len;
  unsigned char *bptr;
  
  iptr = ptr;
  while (isxdigit(*iptr)) iptr++;
  len = iptr - ptr;
  if (len == 0 || len & 0x1 || !(*iptr == ';' || *iptr == '\0')) {
    player_error_message("Error in fmtp config statement");
    return (fmtp_advance_to_next(ptr));
  }
  iptr = fptr->config_ascii = (char *)malloc(len + 1);
  len /= 2;
  bptr = fptr->config_binary = (unsigned char *)malloc(len);
  fptr->config_binary_len = len;
  
  while (len > 0) {
    *bptr++ = (to_hex(ptr) << 4) | (to_hex(ptr + 1));
    *iptr++ = *ptr++;
    *iptr++ = *ptr++;
    len--;
  }
  *iptr = '\0';
  if (*ptr == ';') ptr++;
  return (ptr);
}
static char *fmtp_constant_size (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->constant_size);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_size_length (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->size_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_index_length (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->index_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_index_delta_length (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->index_delta_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_CTS_delta_length (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->CTS_delta_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_DTS_delta_length (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->DTS_delta_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_auxiliary_data_size_length (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->auxiliary_data_size_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_bitrate (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->bitrate);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}
static char *fmtp_profile (char *ptr, fmtp_parse_t *fptr)
{
  char *ret;
  ret = fmtp_parse_number(ptr, &fptr->profile);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

struct {
  const char *name;
  uint32_t namelen;
  char *(*routine)(char *, fmtp_parse_t *);
} fmtp_types[] = 
{
  TTYPE("streamtype", fmtp_streamtype),
  TTYPE("profile-level-id", fmtp_profile_level_id),
  TTYPE("config", fmtp_config),
  TTYPE("constantsize", fmtp_constant_size),
  TTYPE("sizelength", fmtp_size_length),
  TTYPE("indexlength", fmtp_index_length),
  TTYPE("indexdeltalength", fmtp_index_delta_length),
  TTYPE("ctsdeltalength", fmtp_CTS_delta_length),
  TTYPE("dtsdeltalength", fmtp_DTS_delta_length),
  TTYPE("auxiliarydatasizelength", fmtp_auxiliary_data_size_length),
  TTYPE("bitrate", fmtp_bitrate),
  TTYPE("profile", fmtp_profile),
  {NULL, 0, NULL},
}; 

fmtp_parse_t *parse_fmtp_for_mpeg4 (char *optr)
{
  int ix;
  char *bptr;
  fmtp_parse_t *ptr;

  bptr = optr;
  if (bptr == NULL) 
    return (NULL);


  ptr = (fmtp_parse_t *)malloc(sizeof(fmtp_parse_t));
  if (ptr == NULL)
    return (NULL);
  
  ptr->config_binary = NULL;
  ptr->config_ascii = NULL;
  ptr->profile_level_id = -1;
  ptr->constant_size = 0;
  ptr->size_length = 0;
  ptr->index_length = 0;   // default value...
  ptr->index_delta_length = 0;
  ptr->CTS_delta_length = 0;
  ptr->DTS_delta_length = 0;
  ptr->auxiliary_data_size_length = 0;
  ptr->bitrate = -1;
  ptr->profile = -1;

  do {
    ADV_SPACE(bptr);
    for (ix = 0; fmtp_types[ix].name != NULL; ix++) {
      if (strncasecmp(bptr, 
		      fmtp_types[ix].name, 
		      fmtp_types[ix].namelen - 1) == 0) {
	bptr += fmtp_types[ix].namelen - 1;
	ADV_SPACE(bptr);
	if (*bptr != '=') {
	  player_error_message("No = in fmtp %s %s",
			       fmtp_types[ix].name,
			       optr);
	  bptr = fmtp_advance_to_next(bptr);
	  break;
	}
	bptr++;
	ADV_SPACE(bptr);
	bptr = (fmtp_types[ix].routine)(bptr, ptr);
	break;
      }
    }
    if (fmtp_types[ix].name == NULL) {
      player_error_message("Illegal name in bptr - skipping %s", 
			   bptr);
      bptr = fmtp_advance_to_next(bptr);
    }
  } while (bptr != NULL && *bptr != '\0');

  if (bptr == NULL) {
    free_fmtp_parse(ptr);
    return (NULL);
  }
  return (ptr);
}

void free_fmtp_parse (fmtp_parse_t *ptr)
{
  if (ptr->config_binary != NULL) {
    free(ptr->config_binary);
    ptr->config_binary = NULL;
  }

  if (ptr->config_ascii != NULL) {
    free(ptr->config_ascii);
    ptr->config_ascii = NULL;
  }

  free(ptr);
}

/* end file player_sdp.c */
