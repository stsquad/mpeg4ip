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
#include "plaintext.h"
#include <mp4v2/mp4.h>
#define LOGIT pt->m_vft->log_msg
/*
 * Create raw audio structure
 */
static codec_data_t *pt_codec_create (const char *stream_type,
					const char *compressor, 
					format_list_t *media_fmt,
					const uint8_t *userdata,
					uint32_t userdata_size,
					text_vft_t *vft,
					void *ifptr)

{
  pt_codec_t *pt;

  pt = (pt_codec_t *)malloc(sizeof(pt_codec_t));
  memset(pt, 0, sizeof(pt_codec_t));

  pt->m_vft = vft;
  pt->m_ifptr = ifptr;

  (pt->m_vft->text_configure)(ifptr, TEXT_DISPLAY_TYPE_PLAIN, NULL);
  return (codec_data_t *)pt;
}

static void pt_close (codec_data_t *ptr)
{
  pt_codec_t *pt = (pt_codec_t *)ptr;

  free(pt);
}

/*
 * Handle pause - basically re-init the codec
 */
static void pt_do_pause (codec_data_t *ifptr)
{
  //LOGIT(LOG_DEBUG, "pt", "do pause");
}

/*
 * Decode task call for FAAC
 */
static int pt_decode (codec_data_t *ptr,
			frame_timestamp_t *pts,
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer,
			uint32_t buflen,
			void *ud)
{
  pt_codec_t *pt = (pt_codec_t *)ptr;
  uint32_t orig_buflen = buflen;
  uint64_t ts = pts->msec_timestamp;

  // use pt_display_structure_t to pass the 

  plain_display_structure_t display;
  display.display_string = (const char *)buffer;
  // do rest of parsing.
  (pt->m_vft->text_have_frame)(pt->m_ifptr, 
				 ts, 
				 TEXT_DISPLAY_TYPE_PLAIN, 
				 &display);

  return (orig_buflen);
}


static int pt_codec_check (lib_message_func_t message,
			     const char *stream_type,
			     const char *compressor,
			     int type,
			     int profile,
			     format_list_t *fptr, 
			     const uint8_t *userdata,
			     uint32_t userdata_size,
			     CConfigSet *pConfig)
{
#if 0
  // not supported
  bool have_mp4_file = strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0;
  if (have_mp4_file) {
    if (strcasecmp(compressor, "pt") == 0) {
      return 1;
    }
  }
#endif
						    
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 &&
      fptr != NULL) {
    if (fptr->rtpmap_name != NULL) {
      if (strcasecmp(fptr->rtpmap_name, "x-plain-text") == 0) {
	return 1;
      }
    }
  }
  return -1;
}

TEXT_CODEC_PLUGIN("plaintext",
		  pt_codec_create,
		  pt_do_pause,
		  pt_decode,
		  NULL,
		  pt_close,
		  pt_codec_check,
		  NULL, 
		  0);
/* end file pt.cpp */


