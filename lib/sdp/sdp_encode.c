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
 * sdp_encode.c
 *
 * Encodes session_desc_t structure into an SDP file.
 *
 * October, 2000
 * Bill May (wmay@cisco.com)
 * Cisco Systems, Inc.
 */
#include "sdp.h"
#include "sdp_decode_private.h"

typedef struct sdp_encode_t {
  char *buffer;
  uint32_t used;
  uint32_t buflen;
} sdp_encode_t;

static int prepare_sdp_encode (sdp_encode_t *se)
{
  se->buffer = malloc(2048);
  if (se->buffer == NULL) return ENOMEM;
  se->buffer[0] = '\0';
  se->used = 0;
  se->buflen = 2048;
  return (0);
}

static int add_string_to_encode (sdp_encode_t *sptr, const char *buf,
				 int line)
{
  uint32_t len;
  char *temp;
  
  if (buf == NULL) {
    sdp_debug(LOG_CRIT, "Can't add NULL string to SDP - line %d", line);
    return (EINVAL);
  }
  len = strlen(buf);
  if (len == 0) return (0);
  
  if (len + 1 + sptr->used > sptr->buflen) {
    temp = realloc(sptr->buffer, sptr->buflen + 1024);
    if (temp == NULL)
      return (ENOMEM);
    sptr->buffer = temp;
    sptr->buflen += 1024;
  }
  strcpy(sptr->buffer + sptr->used, buf);
  sptr->used += len;
  return (0);
}

#define ADD_STR_TO_ENCODE_WITH_RETURN(se, string) \
 { int ret; ret = add_string_to_encode(se, string, __LINE__); if (ret != 0) return(ret);}

#define CHECK_RETURN(a) {int ret; ret = (a); if (ret != 0) return (ret); }
static int encode_a_ints (int recvonly,
			   int sendrecv,
			   int sendonly,
			   sdp_encode_t *se)
{
  if (recvonly)
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=recvonly\n");
  if (sendrecv)
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=sendrecv\n");
  if (sendonly)
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=sendonly\n");

  return(0);
}

static int encode_string_list (string_list_t *sptr,
			       sdp_encode_t *se,
			       char* code,
			       int *cnt)
{
  int val;
  val = 0;
  while (sptr != NULL) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, code);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->string_val);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
    val++;
    sptr = sptr->next;
  }
  if (cnt != NULL) *cnt = val;
  return (0);
}

static int encode_bandwidth (bandwidth_t *bptr, sdp_encode_t *se)
{
  char buffer[20];
  while (bptr != NULL) {
    if (bptr->modifier == BANDWIDTH_MODIFIER_NONE)
      return (0);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "b=");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, 
				  bptr->modifier == BANDWIDTH_MODIFIER_USER ? 
				  bptr->user_band :
				  bptr->modifier == BANDWIDTH_MODIFIER_CT ?
				     "CT" : "AS");
    snprintf(buffer, sizeof(buffer), ":%ld\n", bptr->bandwidth);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
    bptr = bptr->next;
  }
  return (0);
}

static int encode_category (category_list_t *cptr, sdp_encode_t *se)
{
  int first;
  char buffer[40];
  
  first = FALSE;
  while (cptr != NULL) {
    if (first == FALSE) {
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=cat:");
      snprintf(buffer, sizeof(buffer), U64, cptr->category);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      first = TRUE;
    } else {
      snprintf(buffer, sizeof(buffer), "."U64, cptr->category);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
    }
    cptr = cptr->next;
  }
  if (first == TRUE)
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return(0);
}

static int encode_conf_type (session_desc_t *sptr, sdp_encode_t *se)
{
  const char *temp;
  switch (sptr->conf_type) {
  case CONFERENCE_TYPE_NONE:
  default:
    return (0);
  case CONFERENCE_TYPE_OTHER:
    temp = sptr->conf_type_user;
    break;
  case CONFERENCE_TYPE_BROADCAST:
    temp = "broadcast";
    break;
  case CONFERENCE_TYPE_MEETING:
    temp = "meeting";
    break;
  case CONFERENCE_TYPE_MODERATED:
    temp = "moderated";
    break;
  case CONFERENCE_TYPE_TEST:
    temp = "test";
    break;
  case CONFERENCE_TYPE_H332:
    temp = "H322";
    break;
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=type:");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, temp);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return (0);
}

static int encode_connect (connect_desc_t *cptr, sdp_encode_t *se)
{
  char buffer[20];
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "c=IN ");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, cptr->conn_type);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, cptr->conn_addr);
  if (cptr->ttl) {
    snprintf(buffer, sizeof(buffer), "/%d", cptr->ttl);
    ADD_STR_TO_ENCODE_WITH_RETURN(se,buffer);
    if (cptr->num_addr) {
      snprintf(buffer, sizeof(buffer), "/%d", cptr->num_addr);
      ADD_STR_TO_ENCODE_WITH_RETURN(se,buffer);
    }
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return (0);
}

static int encode_rtcp (media_desc_t *mptr, sdp_encode_t *se)
{
  char buffer[20];
  connect_desc_t *cptr = &mptr->rtcp_connect;
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=rtcp:");
  snprintf(buffer, sizeof(buffer), "%u", mptr->rtcp_port);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  if (cptr->conn_type != NULL) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, cptr->conn_type);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, cptr->conn_addr);
    if (cptr->ttl) {
      snprintf(buffer, sizeof(buffer), "/%d", cptr->ttl);
      ADD_STR_TO_ENCODE_WITH_RETURN(se,buffer);
      if (cptr->num_addr) {
	snprintf(buffer, sizeof(buffer), "/%d", cptr->num_addr);
	ADD_STR_TO_ENCODE_WITH_RETURN(se,buffer);
      }
    }
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return (0);
}

static int encode_key (key_desc_t *kptr, sdp_encode_t *se)
{
  const char *temp;
  
  switch (kptr->key_type) {
  case KEY_TYPE_NONE:
  default:
    return (0);
  case KEY_TYPE_PROMPT:
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "k=prompt\n");
    return(0);
  case KEY_TYPE_CLEAR:
    temp ="clear";
    break;
  case KEY_TYPE_BASE64:
    temp = "base64";
    break;
  case KEY_TYPE_URI:
    temp = "uri";
    break;
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "k=");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, temp);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, ":");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, kptr->key);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return (0);
}
static int encode_range (range_desc_t *rptr, sdp_encode_t *se)
{
  char buffer[80];
  if (rptr->have_range) {
    if (rptr->range_is_npt) {
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=npt:");
      snprintf(buffer, sizeof(buffer), "%g-", rptr->range_start);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      if (rptr->range_end_infinite == FALSE) {
	snprintf(buffer, sizeof(buffer), "%g", rptr->range_end);
	ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      }
    } else {
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=smpte");
      if (rptr->range_smpte_fps != 0) {
	snprintf(buffer, sizeof(buffer), "-%d", rptr->range_smpte_fps);
	ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      }
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "=");
      sdp_smpte_to_str(rptr->range_start, rptr->range_smpte_fps, buffer, 
		       sizeof(buffer));
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "-");
      if (rptr->range_end_infinite == FALSE) {
	sdp_smpte_to_str(rptr->range_end, rptr->range_smpte_fps, buffer,
			 sizeof(buffer));
	ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      }
    }
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  return (0);
}

static int encode_time (session_time_desc_t *tptr, sdp_encode_t *se)
{
  uint64_t start, end;
  time_repeat_desc_t *rptr;
  uint32_t ix;
  char buffer[80];
  
  while (tptr != NULL) {
    start = tptr->start_time;
    if (start != 0) start += NTP_TO_UNIX_TIME;
    end = tptr->end_time;
    if (end != 0) end += NTP_TO_UNIX_TIME;

    snprintf(buffer, sizeof(buffer), "t="U64" "U64"\n", start, end);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);

    rptr = tptr->repeat;
    while (rptr != NULL) {
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "r=");
      sdp_time_offset_to_str(rptr->repeat_interval, buffer, sizeof(buffer));
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
      sdp_time_offset_to_str(rptr->active_duration, buffer, sizeof(buffer));
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      for (ix = 0; ix < rptr->offset_cnt; ix++) {
	snprintf(buffer, sizeof(buffer), " %d", rptr->offsets[ix]);
	ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      }
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
      rptr = rptr->next;
    }
    tptr = tptr->next;
  }
  return (0);
}

static int encode_time_adj (time_adj_desc_t *aptr, sdp_encode_t *se)
{
  uint64_t time_val;
  uint32_t offset;
  uint32_t len;
  char buffer[40], *buff;
  int dohead;

  if (aptr == NULL) return (0);
  
  dohead = TRUE;
  while (aptr != NULL) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, dohead ? "z=" : " ");
	time_val = aptr->adj_time + NTP_TO_UNIX_TIME;
    snprintf(buffer, sizeof(buffer), U64" ", time_val);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
    if (aptr->offset < 0) {
      offset = (0 - aptr->offset);
      buffer[0] = '-';
      buff = &buffer[1];
      len = sizeof(buffer) - 1;
    } else {
      offset = aptr->offset;
      buff = buffer;
      len = sizeof(buffer);
    }
    sdp_time_offset_to_str(offset, buff, len);
    
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
    if (dohead == FALSE)
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
    dohead = !dohead;
    aptr = aptr->next;
  }
  if (dohead == FALSE)
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return (0);
}

static int encode_orient (media_desc_t *mptr, sdp_encode_t *se)
{
  const char *temp;
  switch (mptr->orient_type) {
  case ORIENT_TYPE_NONE:
  default:
    return (0);
    
  case ORIENT_TYPE_PORTRAIT:
    temp = "portrait";
    break;
  case ORIENT_TYPE_LANDSCAPE:
    temp = "landscape";
    break;
  case ORIENT_TYPE_SEASCAPE:
    temp = "seascape";
    break;
  case ORIENT_TYPE_USER:
    temp = mptr->orient_user_type;
    break;
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=orient:");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, temp);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  return(0);
}

static int encode_media (media_desc_t *mptr, sdp_encode_t *se)
{
  format_list_t *fptr;
  char buffer[80];
  
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "m=");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, mptr->media);
  
  snprintf(buffer, sizeof(buffer), " %u", mptr->port);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  if (mptr->num_ports) {
    snprintf(buffer,sizeof(buffer), "/%u", mptr->num_ports);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, mptr->proto);

  fptr = mptr->fmt_list;
  while (fptr != NULL) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, fptr->fmt);
    fptr = fptr->next;
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  if (mptr->media_desc) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "i=");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, mptr->media_desc);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (mptr->media_connect.used) {
    CHECK_RETURN(encode_connect(&mptr->media_connect, se));
  }
  if (mptr->rtcp_connect.used) {
    CHECK_RETURN(encode_rtcp(mptr, se));
  }
  CHECK_RETURN(encode_bandwidth(mptr->media_bandwidth, se));
  CHECK_RETURN(encode_key(&mptr->key, se));
  
  if (mptr->ptime_present) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=ptime:");
    snprintf(buffer, sizeof(buffer), "%u\n", mptr->ptime);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  }
  if (mptr->quality_present) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=quality:");
    snprintf(buffer, sizeof(buffer), "%u\n", mptr->quality);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  }
  if (mptr->framerate_present) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=framerate:");
    snprintf(buffer, sizeof(buffer), "%g\n", mptr->framerate);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  }
  
  if (mptr->control_string) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=control:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, mptr->control_string);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (mptr->sdplang) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=sdplang:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, mptr->sdplang);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (mptr->lang) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=lang:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, mptr->lang);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }

  fptr = mptr->fmt_list;
  while (fptr != NULL) {
    if (fptr->rtpmap_name != NULL) {
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=rtpmap:");
      ADD_STR_TO_ENCODE_WITH_RETURN(se, fptr->fmt);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
      ADD_STR_TO_ENCODE_WITH_RETURN(se, fptr->rtpmap_name);
      snprintf(buffer, sizeof(buffer), "/%u", fptr->rtpmap_clock_rate);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      if (fptr->rtpmap_encode_param != 0) {
	snprintf(buffer, sizeof(buffer), "/%u", fptr->rtpmap_encode_param);
	ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
      }
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
    }
    if (fptr->fmt_param != NULL) {
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=fmtp:");
      ADD_STR_TO_ENCODE_WITH_RETURN(se, fptr->fmt);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
      ADD_STR_TO_ENCODE_WITH_RETURN(se, fptr->fmt_param);
      ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
    }
    fptr = fptr->next;
  }
  CHECK_RETURN(encode_a_ints(mptr->recvonly,
			      mptr->sendrecv,
			      mptr->sendonly,
			      se));
  CHECK_RETURN(encode_orient(mptr, se));
  CHECK_RETURN(encode_range(&mptr->media_range, se));
  CHECK_RETURN(encode_string_list(mptr->unparsed_a_lines, se, "", NULL));
  return (0);
} 
  
static int sdp_encode (session_desc_t *sptr, sdp_encode_t *se)
{
  int temp, temp1;
  media_desc_t *mptr;
  char buffer[80];

  if (sptr->create_addr_type == NULL ||
      sptr->create_addr == NULL) {
    return (ESDP_ORIGIN);
  }
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "v=0\no=");
  ADD_STR_TO_ENCODE_WITH_RETURN(se,
				sptr->orig_username == NULL ?
				"-" : sptr->orig_username);
  snprintf(buffer, sizeof(buffer), " "D64" "D64" IN ",
	  sptr->session_id,
	  sptr->session_version);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, buffer);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->create_addr_type);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, " ");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->create_addr);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\ns=");
  ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->session_name);
  ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");

  if (sptr->session_desc != NULL) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "i=");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->session_desc);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (sptr->uri != NULL) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "u=");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->uri);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  CHECK_RETURN(encode_string_list(sptr->admin_email, se, "e=", &temp));
  CHECK_RETURN(encode_string_list(sptr->admin_phone, se, "p=", &temp1));
  if (temp + temp1 == 0 && sptr->no_admin_or_phone == FALSE) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "e=NONE\n");
  }
  if (sptr->session_connect.used) {
    CHECK_RETURN(encode_connect(&sptr->session_connect, se));
  }
  CHECK_RETURN(encode_bandwidth(sptr->session_bandwidth, se));
  CHECK_RETURN(encode_time(sptr->time_desc, se));
  CHECK_RETURN(encode_time_adj(sptr->time_adj_desc, se));
  CHECK_RETURN(encode_key(&sptr->key, se));
  if (sptr->control_string) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=control:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->control_string);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  CHECK_RETURN(encode_category(sptr->category_list, se));
  if (sptr->keywds) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "u=");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->uri);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  CHECK_RETURN(encode_conf_type(sptr, se));
  if (sptr->tool) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=tool:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->tool);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (sptr->charset) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=charset:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->charset);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (sptr->sdplang) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=sdplang:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->sdplang);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  if (sptr->lang) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=lang:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->lang);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }

  if (sptr->etag) {
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "a=etag:");
    ADD_STR_TO_ENCODE_WITH_RETURN(se, sptr->etag);
    ADD_STR_TO_ENCODE_WITH_RETURN(se, "\n");
  }
  
  CHECK_RETURN(encode_range(&sptr->session_range, se));
  CHECK_RETURN(encode_a_ints(sptr->recvonly,
			      sptr->sendrecv,
			      sptr->sendonly,
			      se));
  CHECK_RETURN(encode_string_list(sptr->unparsed_a_lines, se, "", NULL));
  mptr = sptr->media;
  while (mptr != NULL) {
    CHECK_RETURN(encode_media(mptr, se));
    mptr = mptr->next;
  }
  return (0);
}  
	  
int sdp_encode_one_to_file (session_desc_t *sptr,
			    const char *filename,
			    int append)
{
  FILE *ofile;
  sdp_encode_t sdp;
  CHECK_RETURN(prepare_sdp_encode(&sdp));
  CHECK_RETURN(sdp_encode(sptr, &sdp));
  ofile = fopen(filename, append ? "a" : "w");
  if (ofile == NULL) {
    sdp_debug(LOG_EMERG, "Cannot open file %s", filename);
    free(sdp.buffer);
    return (-1);
  }
  fputs(sdp.buffer, ofile);
  fclose(ofile);
  free(sdp.buffer);
  return (0);
}

int sdp_encode_list_to_file (session_desc_t *sptr,
			      const char *filename,
			      int append)
{
  FILE *ofile;
  sdp_encode_t sdp;
  int retval;
  
  CHECK_RETURN(prepare_sdp_encode(&sdp));
  ofile = fopen(filename, append ? "a" : "w");
  if (ofile == NULL) {
    free(sdp.buffer);
	return (-1);
  }
  while (sptr != NULL) {
    sdp.used = 0;
    retval = sdp_encode(sptr, &sdp);
    if (retval != 0) {
      break;
    }
    fputs(sdp.buffer, ofile);
    sptr = sptr->next;
  }
  fclose(ofile);
  free(sdp.buffer);
  return (0);
}

int sdp_encode_one_to_memory (session_desc_t *sptr, char **mem)
{
  sdp_encode_t sdp;
  int retval;
  
  *mem = NULL;
  CHECK_RETURN(prepare_sdp_encode(&sdp));
  retval = sdp_encode(sptr, &sdp);
  if (retval != 0) {
    free(sdp.buffer);
    return (retval);
  }
  *mem = sdp.buffer;
  return (0);
}

int sdp_encode_list_to_memory (session_desc_t *sptr, char **mem, int *count)
{
  sdp_encode_t sdp;
  int retval;
  int cnt;
  
  *mem = NULL;
  CHECK_RETURN(prepare_sdp_encode(&sdp));
  cnt = 0;
  retval = 0;
  while (sptr != NULL && retval == 0) {
    retval = sdp_encode(sptr, &sdp);
    if (retval == 0)
      cnt++;
    sptr = sptr->next;
  }
  *mem = sdp.buffer;
  if (count != NULL)
    *count = cnt;
  return (retval);
}





