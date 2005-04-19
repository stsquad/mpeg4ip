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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */

#include "h264_sdp.h"

/*
 * h264_sdp_parse_sprop_param_sets - look for the sprop_param sets in the
 * a=fmtp message passed, create nal units from base64 code.
 * Inputs - fmt_param - a=fmtp: string
 *   nal_len - pointer to return length
 *   message - pointer to output routine
 * Outputs - binary nal string (must be freed by caller)
 */
uint8_t *h264_sdp_parse_sprop_param_sets (const char *fmt_param, 
					  uint32_t *nal_len,
					  lib_message_func_t message)
{
  const char *sprop;
  const char *end;
  uint8_t *bin, *ret;
  uint32_t binsize;

  sprop = strcasestr(fmt_param, "sprop-parameter-sets");

  if (sprop == NULL) {
    if (message != NULL) 
      message(LOG_ERR, "h264sdp", "no sprop-parameter-sets in sdp");
    return NULL;
  }
  sprop += strlen("sprop-parameter-sets");
  ADV_SPACE(sprop);
  if (*sprop != '=') {
    if (message != NULL)
      message(LOG_DEBUG, "h264sdp", "no equals in sprop-parameter-sets");
    return NULL;
  }
  sprop++;
  ADV_SPACE(sprop);
  ret = NULL;
  *nal_len = 0;
  do {
    end = sprop;
    while (*end != ',' && *end != ';' && *end != '\0') end++;
    if (sprop != end) {
      bin = Base64ToBinary(sprop, end - sprop, &binsize);
      if (bin != NULL) {
	ret = (uint8_t *)realloc(ret, *nal_len + binsize + 4);
	if (nal_len == 0) {
	  ret[*nal_len] = 0;
	  *nal_len += 1;
	}
	ret[*nal_len] = 0;
	ret[*nal_len + 1] = 0;
	ret[*nal_len + 2] = 1;
	memcpy(ret + *nal_len + 3, 
	       bin,
	       binsize);
	*nal_len += binsize + 3;
      } else {
	if (message != NULL)
	  message(LOG_ERR, "h264sdp", 
		  "failed to convert %u \"%s\"", 
		  end - sprop, sprop);
      }
    }
	  
    sprop = end;
    if (*sprop == ',') sprop++;
  } while (*sprop != ';' && *sprop != '\0');
  return ret;
}
