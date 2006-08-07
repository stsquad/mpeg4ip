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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
#include "href.h"
#include <mp4v2/mp4.h>
//#define DEBUG_HREF
#define LOGIT href->m_vft->log_msg
/*
 * Create raw audio structure
 */
static codec_data_t *href_codec_create (const char *stream_type,
					const char *compressor, 
					format_list_t *media_fmt,
					const uint8_t *userdata,
					uint32_t userdata_size,
					text_vft_t *vft,
					void *ifptr)

{
  href_codec_t *href;

  href = (href_codec_t *)malloc(sizeof(href_codec_t));
  memset(href, 0, sizeof(href_codec_t));

  href->m_vft = vft;
  href->m_ifptr = ifptr;

  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    const char *base_url = strstr(media_fmt->fmt_param, "base_url=");
    if (base_url != NULL) {
      href->m_base_url = strdup(base_url + strlen("base_url="));
    }
  }
  (href->m_vft->text_configure)(ifptr, TEXT_DISPLAY_TYPE_HREF, NULL);
  return (codec_data_t *)href;
}

static void href_close (codec_data_t *ptr)
{
  href_codec_t *href = (href_codec_t *)ptr;
  CHECK_AND_FREE(href->m_base_url);
  CHECK_AND_FREE(href->m_url);
  free(href);
}

/*
 * Handle pause - basically re-init the codec
 */
static void href_do_pause (codec_data_t *ifptr)
{
  //LOGIT(LOG_DEBUG, "href", "do pause");
}

/*
 * Decode task call for FAAC
 */
static int href_decode (codec_data_t *cptr,
			frame_timestamp_t *pts,
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer,
			uint32_t buflen,
			void *ud)
{
  href_codec_t *href = (href_codec_t *)cptr;
  uint32_t orig_buflen = buflen;
  uint64_t ts = pts->msec_timestamp;

  if (ud != NULL) {
    const char *base_url = (const char *)ud;
    if (href->m_base_url == NULL ||
	strcmp(href->m_base_url, base_url) != 0) {
      href->m_base_url = strdup(base_url);
    }
    free(ud);
  }
  //LOGIT(LOG_DEBUG, "hrefd", "buffer %s", buffer);
  // use href_display_structure_t to pass the 
#ifdef DEBUG_HREF
  LOGIT(LOG_DEBUG, "href_plug", "href %s at "U64,
	buffer, ts);
#endif
  href_display_structure_t display;
  memset(&display, 0, sizeof(display));
  if (buflen + 1 > href->m_buffer_len) {
    href->m_buffer = (char *)realloc(href->m_buffer, buflen + 1);
  }
  memcpy(href->m_buffer, buffer, buflen);
  href->m_buffer[buflen] = '\0'; // at end, so we can do string things.

  char *ptr = href->m_buffer;
  if (*ptr == 'A') {
      display.auto_dispatch = true;
      ptr++;
  }
  
  ADV_SPACE(ptr);
  if (*ptr != '<') {
    LOGIT(LOG_INFO, "href", "Illegal first element in \"%s\"", href->m_buffer);
    return orig_buflen;
  }
  ptr++;
  display.url = ptr;
  while (*ptr != '>' && *ptr != '\0') ptr++;
  if (*ptr != '>') {
    LOGIT(LOG_INFO, "href", "Can't find end of element in \"%s\"", 
	  href->m_buffer);
    return orig_buflen;
  }
  *ptr = '\0';
  if (href->m_base_url != NULL) {
    const char *slash = strchr(display.url, '/');
    const char *colon = strchr(display.url, ':');
    if (slash == NULL || colon == NULL || colon > slash) {
      // need to add base url
      CHECK_AND_FREE(href->m_url);
      href->m_url = (char *)malloc(strlen(href->m_base_url) + strlen(display.url) + 1);
      strcpy(href->m_url, href->m_base_url);
      strcat(href->m_url, display.url);
      display.url = href->m_url;
    }
  }
  ptr++;
  while (*ptr != '\0') {
    char *val = ptr, *start;
    ptr++;
    ADV_SPACE(ptr);
    if (*ptr != '<') {
      LOGIT(LOG_INFO, "href", "Can't find start of element \"%s\"", 
	    val);
      return orig_buflen;
    }
    ptr++;
    start = ptr;
    while (*ptr != '>' && *ptr != '\0') ptr++;
    if (*ptr != '>') {
      LOGIT(LOG_INFO, "href", "Can't find end of element in \"%s\"", val);
      return orig_buflen;
    }
    *ptr = '\0';
    ptr++;
    switch (tolower(*val)) {
    case 't':
      if (display.target_element != NULL) {
	LOGIT(LOG_INFO, "href", "duplicate target element in href");
	return orig_buflen;
      }
      display.target_element = start;
      break;
    case 'e':
      if (display.embed_element != NULL) {
	LOGIT(LOG_INFO, "href", "duplicate embed element in href");
	return orig_buflen;
      }
      display.embed_element = start;
      break;
	
    case 'm':
      display.send_click_location = true;
      break;
    }
  }
  // do rest of parsing.
  (href->m_vft->text_have_frame)(href->m_ifptr, 
				 ts, 
				 TEXT_DISPLAY_TYPE_HREF, 
				 &display);

  return (orig_buflen);
}


static int href_codec_check (lib_message_func_t message,
			     const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr, 
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			     CConfigSet *pConfig)
{
  bool have_mp4_file = strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0;
  if (have_mp4_file) {
    if (strcasecmp(compressor, "href") == 0) {
      return 1;
    }
  }
						    
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "X-HREF") == 0) {
	return 1;
      }
    }
  }
  return -1;
}

TEXT_CODEC_PLUGIN("href",
		  href_codec_create,
		  href_do_pause,
		  href_decode,
		  NULL,
		  href_close,
		  href_codec_check,
		  NULL, 
		  0);
/* end file href.cpp */


