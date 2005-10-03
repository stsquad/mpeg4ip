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
 * mpeg4_sdp.c - utilities for handling SDP structures
 */
#include "mpeg4ip.h"
#include <sdp/sdp.h>
#include "mpeg4_sdp.h"

#define TTYPE(a,b) {a, sizeof(a), b}

#define FMTP_PARSE_FUNC(a) static const char *(a) (const char *ptr, \
					     fmtp_parse_t *fptr, \
					     lib_message_func_t message)
static const char *fmtp_advance_to_next (const char *ptr)
{
  while (*ptr != '\0' && *ptr != ';') ptr++;
  if (*ptr == ';') ptr++;
  return (ptr);
}

static char *fmtp_parse_number (const char *ptr, int *ret_value)
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

FMTP_PARSE_FUNC(fmtp_streamtype)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->stream_type);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_profile_level_id)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->profile_level_id);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

static unsigned char to_hex (const char *ptr)
{
  if (isdigit(*ptr)) {
    return (*ptr - '0');
  }
  return (tolower(*ptr) - 'a' + 10);
}

FMTP_PARSE_FUNC(fmtp_config)
{
  char *iptr;
  const char *lptr;
  uint32_t len;
  unsigned char *bptr;
  
  lptr = ptr;
  while (isxdigit(*lptr)) lptr++;
  len = lptr - ptr;
  if (len == 0 || len & 0x1 || !(*lptr == ';' || *lptr == '\0')) {
    message(LOG_ERR, "mp4util", "Error in fmtp config statement");
    return (fmtp_advance_to_next(ptr));
  }
  iptr = fptr->config_ascii = (char *)malloc(len + 1);
  len /= 2;
  bptr = fptr->config_binary = (uint8_t *)malloc(len);
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

FMTP_PARSE_FUNC(fmtp_constant_size)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->constant_size);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_size_length)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->size_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_index_length)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->index_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_index_delta_length)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->index_delta_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_CTS_delta_length)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->CTS_delta_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_DTS_delta_length)

{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->DTS_delta_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_auxiliary_data_size_length)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->auxiliary_data_size_length);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_bitrate)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->bitrate);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_profile)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->profile);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_mode)
{
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_cryptosuite)
{
  // just ignore it
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_ivlength)
{
  // just ignore it
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_ivdeltalength)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->ISMACrypIVDeltaLength);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}

FMTP_PARSE_FUNC(fmtp_selectiveencryption)
{
  // just ignore it
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_keyindicatorlength)
{
  // just ignore it
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_keyindicatorperau)
{
  // just ignore it
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_key)
{
  // just ignore it
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_object)
{
  return (fmtp_advance_to_next(ptr));
}

FMTP_PARSE_FUNC(fmtp_cpresent)
{
  const char *ret;
  ret = fmtp_parse_number(ptr, &fptr->cpresent);
  if (ret == NULL) {
    ret = fmtp_advance_to_next(ptr);
  }
  return (ret);
}


struct fmtp_parse_type_ {
  const char *name;
  uint32_t namelen;
  const char *(*routine)(const char *, fmtp_parse_t *, lib_message_func_t);
} fmtp_isma_types[] = 
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
  TTYPE("mode", fmtp_mode),
  TTYPE("ISMACrypCryptoSuite", fmtp_cryptosuite),
  TTYPE("ISMACrypIVLength", fmtp_ivlength),
  TTYPE("ISMACrypIVDeltaLength", fmtp_ivdeltalength),
  TTYPE("ISMACrypSelectiveEncryption", fmtp_selectiveencryption),
  TTYPE("ISMACrypKeyIndicatorLength", fmtp_keyindicatorlength),
  TTYPE("ISMACrypKeyIndicatorPerAU", fmtp_keyindicatorperau),
  TTYPE("ISMACrypKey", fmtp_key),
  {NULL, 0, NULL},
}, fmtp_3016_types[] = {
  TTYPE("profile-level-id", fmtp_profile_level_id),
  TTYPE("config", fmtp_config),
  TTYPE("object", fmtp_object),
  TTYPE("cpresent", fmtp_cpresent),
  { NULL, 0, NULL},
};

static fmtp_parse_t *parse_fmtp_for_table (struct fmtp_parse_type_ *fmtp_types,
					   const char *optr, 
					   lib_message_func_t message)
{
  int ix;
  const char *bptr;
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
  ptr->cpresent = 1;
  do {
    ADV_SPACE(bptr);
    for (ix = 0; fmtp_types[ix].name != NULL; ix++) {
      if (strncasecmp(bptr, 
		      fmtp_types[ix].name, 
		      fmtp_types[ix].namelen - 1) == 0) {
	bptr += fmtp_types[ix].namelen - 1;
	ADV_SPACE(bptr);
	if (*bptr != '=') {
	  message(LOG_ERR, "mp4util", "No = in fmtp %s %s",
		  fmtp_types[ix].name,
		  optr);
	  bptr = fmtp_advance_to_next(bptr);
	  break;
	}
	bptr++;
	ADV_SPACE(bptr);
	bptr = (fmtp_types[ix].routine)(bptr, ptr, message);
	break;
      }
    }
    if (fmtp_types[ix].name == NULL) {
      message(LOG_ERR, "mp4util", "Illegal name in bptr - skipping %s", 
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

fmtp_parse_t *parse_fmtp_for_mpeg4 (const char *optr, lib_message_func_t message)
{
  return parse_fmtp_for_table(fmtp_isma_types, optr, message);
}

fmtp_parse_t *parse_fmtp_for_rfc3016 (const char *optr, 
				      lib_message_func_t message)
{
  return parse_fmtp_for_table(fmtp_3016_types, optr, message);
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
