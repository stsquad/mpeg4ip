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
 * sdp_util.c
 *
 * Utility routines for sdp decode/encode.
 *
 * October, 2000
 * Bill May (wmay@cisco.com)
 * Cisco Systems, Inc.
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sdp.h"
#include "sdp_decode_private.h"

/*
 * add_format_to_list()
 * Adds a format to a media_desc_t format list.  Note: will only add unique
 * values.
 *
 * Inputs:
 *   **list - ptr to ptr to list to store in (usually &<media_desc_t>->fmt)
 *   *val - value to save.
 * Outputs:
 *   pointer to added value.  This could be a new one, or one previously
 *   added
 */
format_list_t *add_format_to_list (media_desc_t *mptr, char *val)
{
  format_list_t *new, *p;
  
  new = malloc(sizeof(format_list_t));
  if (new == NULL) {
    return (NULL);
  }

  new->next = NULL;
  new->fmt = strdup(val);
  new->rtpmap = NULL;
  new->fmt_param = NULL;
  new->media = mptr;
  
  if (new->fmt == NULL) {
    free(new);
    return (NULL);
  }
  
  if (mptr->fmt == NULL) {
    mptr->fmt = new;
  } else {
    p = mptr->fmt;
    if (strcmp(p->fmt, new->fmt) == 0) {
      free(new);
      return (p);
    }
    while (p->next != NULL) {
      p = p->next;
      if (strcmp(p->fmt, new->fmt) == 0) {
	free(new);
	return (p);
      }
    }
    p->next = new;
  }
  return (new);
}

/*
 * add_string_to_list()
 * Adds string to string_list_t list.  Duplicates string.
 * Inputs:
 *   list - pointer to pointer of list head
 *   val - value to add
 * Outputs:
 *   TRUE - succeeded, FALSE - failed due to no memory
 */
int add_string_to_list (string_list_t **list, char *val)
{
  string_list_t *new, *p;
  
  new = malloc(sizeof(string_list_t));
  if (new == NULL) {
    return (FALSE);
  }

  new->next = NULL;
  new->string_val = strdup(val);
  if (new->string_val == NULL) {
    free(new);
    return (FALSE);
  }
  
  if (*list == NULL) {
    *list = new;
  } else {
    p = *list;
    while (p->next != NULL) p = p->next;
    p->next = new;
  }
  return (TRUE);
}

/*
 * time_offset_to_str()
 * Converts a time offset in seconds to dhms format. (ie: 3600 -> 1h)
 * Inputs:
 *   val - value in seconds to convert
 *   buff - buffer to store into
 *   buflen - length of buffer
 */
void time_offset_to_str (uint32_t val, char *buff, size_t buflen)
{
  uint32_t div;

  div = val % SEC_PER_DAY;
  if (div == 0) {
    div = val / SEC_PER_DAY;
    snprintf(buff, buflen, "%dd", div);
    return;
  }
  div = val % SEC_PER_HOUR;
  if (div == 0) {
    div = val / SEC_PER_HOUR;
    snprintf(buff, buflen, "%dh", div);
    return;
  }
  div = val % SEC_PER_MINUTE;
  if (div == 0) {
    div = val / SEC_PER_MINUTE;
    snprintf(buff, buflen, "%dm", div);
    return;
  }

  snprintf(buff, buflen, "%d", val);
}

/*
 * find_format_in_line()
 * Looks for a format value in the list specified.  Must be an exact match.
 * Inputs:
 *   head - pointer to head of format list
 *   lptr - pointer to string to compare with.  Must have a space or \0
 *      seperating format from rest of line
 * Outputs:
 *   ptr of matching format or NULL if nothing matched.
 */
format_list_t *find_format_in_line (format_list_t *head, char *lptr)
{
  size_t len;
  
  while (head != NULL) {
    len = strlen(head->fmt);
    if ((strncasecmp(lptr, head->fmt, len) == 0) &&
	((isspace(*(lptr + len)) ||
	 (*(lptr + len) == '\0')))) {
      return (head);
    } else 
      head = head->next;
  }
  return (NULL);
}

void smpte_to_str (double value, uint16_t fps, char *buffer)
{
  double div;
  unsigned int temp;
  size_t index;
  if (fps == 0) fps = 30;

  temp = 0;
  div = 3600.0 * fps;
  while (value >= div) {
    temp++;
    value -= div;
  }
  index = sprintf(buffer, "%02d:", temp);
  temp = 0;
  div = 60.0 * fps;
  while (value >= div) {
    temp++;
    value -= div;
  }
  index += sprintf(buffer + index, "%02d:", temp);

  temp = 0;
  div = fps;
  while (value >= div) {
    temp++;
    value -= div;
  }
  index += sprintf(buffer + index, "%02d", temp);
  if (value > 0.0) sprintf(buffer + index, ":%02g", value);
}

static int sdp_debug_level = LOG_ERR;

static error_msg_func_t error_msg_func = NULL;

void sdp_set_loglevel (int loglevel)
{
  sdp_debug_level = loglevel;
}

void sdp_set_error_func (error_msg_func_t func)
{
  error_msg_func = func;
}
void sdp_debug (int loglevel, const char *fmt, ...)
{
  if (loglevel <= sdp_debug_level) {
    if (error_msg_func != NULL) {
      va_list ap;
      va_start(ap, fmt);
      (error_msg_func)(loglevel, "libsdp", fmt, ap);
      va_end(ap);
    }
  }
}
/* end file sdp_util.c */

