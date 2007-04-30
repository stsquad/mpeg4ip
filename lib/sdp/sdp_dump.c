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
 * sdp_dump.c
 *
 * dump SDP structure to stdout
 *
 * October, 2000
 * Bill May (wmay@cisco.com)
 * Cisco Systems, Inc.
 */
#include "sdp.h"
#include <time.h>
#include "sdp_decode_private.h"

static void time_repeat_dump (time_repeat_desc_t *trptr)
{
  int morethan1;
  char buffer[80], *start;
  int cnt;
  uint32_t ix;

  morethan1 = trptr != NULL && trptr->next != NULL;
  cnt = 0;
  
  while (trptr != NULL) {
    if (morethan1) {
      printf("Repeat %d:\n", cnt + 1);
      start = "\t";
    } else {
      start = "";
    }
    cnt++;
    sdp_time_offset_to_str(trptr->repeat_interval, buffer, sizeof(buffer));
    printf("%sRepeat Interval: %s\n", start, buffer);
    sdp_time_offset_to_str(trptr->active_duration, buffer, sizeof(buffer));
    printf("%sDuration of session: %s\n", start, buffer);
    printf("%sOffsets: ", start);
    for (ix = 0; ix < trptr->offset_cnt; ix++) {
      sdp_time_offset_to_str(trptr->offsets[ix], buffer, sizeof(buffer));
      printf("%s ", buffer);
      if ((ix % 8) == 0) {
	printf("\n%s", start);
      }
    }
    trptr = trptr->next;
  }
}
  
static void time_dump (session_time_desc_t *tptr) {
  while (tptr != NULL) {
    if (tptr->end_time == 0 &&
	tptr->start_time == 0) {
      printf("Start/End time is 0\n");
    } else {
      if (tptr->start_time != 0) {
	printf("Start Time: %s", ctime(&tptr->start_time));
      } else {
	printf("Start Time is 0\n");
      }
      if (tptr->end_time != 0) {
	printf("End Time: %s", ctime(&tptr->end_time));
      } else {
	printf("End Time is 0\n");
      }
    }
    time_repeat_dump(tptr->repeat);
    tptr = tptr->next;
   
  }
}

static void time_adj_dump (time_adj_desc_t *taptr)
{
  while (taptr != NULL) {
    printf("Adjustment of %d on %s", taptr->offset, ctime(&taptr->adj_time));
    taptr = taptr->next;
  }
}

static void unparsed_dump (string_list_t *uptr,
			   char *start)
{
  printf("%sUnparsed lines:\n", start);
  while (uptr != NULL) {
    printf("\t%s%s\n", start, uptr->string_val);
    uptr = uptr->next;
  }
}

static void bandwidth_dump (bandwidth_t *bptr, char *start)
{
  while (bptr != NULL) {
    printf("%s Bandwidth: %ld ", start, bptr->bandwidth);
    if (bptr->modifier == BANDWIDTH_MODIFIER_USER) {
      printf("(%s)", bptr->user_band);
    } else {
      printf("(%s)",
	     bptr->modifier == BANDWIDTH_MODIFIER_AS ? "AS type" : "CT type");
    }
    printf("\n");
    bptr = bptr->next;
  }
}

static void category_dump (category_list_t *cptr)
{
  int ix = 0;
  if (cptr == NULL)
    return;
  printf("Category: ");
  while (cptr != NULL) {
    printf(D64" ", cptr->category);
    ix++;
    if (ix >= 8) {
      printf("\n");
      ix = 0;
    }
    cptr = cptr->next;
  }
  if (ix != 0) printf("\n");
}
static void key_dump (key_desc_t *sptr)
{
  switch (sptr->key_type) {
  case KEY_TYPE_PROMPT:
    printf("Key: prompt\n");
    break;
  case KEY_TYPE_CLEAR:
    printf("Key: clear : %s\n", sptr->key);
    break;
  case KEY_TYPE_BASE64:
    printf("Key: base64\n");
    break;
  case KEY_TYPE_URI:
    printf("Key: uri : %s", sptr->key);
    break;
  case KEY_TYPE_NONE:
  default:
    break;
  }
}

static void range_dump (range_desc_t *rptr, const char *start)
{
  char buffer[80];
  if (rptr->have_range == FALSE) return;
  printf("%sRange is ", start);
  if (rptr->range_is_npt) {
    printf("npt - start %g, end ", rptr->range_start);
    if (rptr->range_end_infinite) {
      printf("infinite\n");
    } else {
      printf("%g\n", rptr->range_end);
    }
  } else {
    printf("smtpe - start ");
    sdp_smpte_to_str(rptr->range_start, rptr->range_smpte_fps, buffer, 
		     sizeof(buffer));
    printf("%s, end ", buffer);
    if (rptr->range_end_infinite) {
      printf("infinite\n");
    } else {
      sdp_smpte_to_str(rptr->range_end, rptr->range_smpte_fps, buffer,
		       sizeof(buffer));
      printf("%s\n", buffer);
    }
  }

}

static void media_dump (media_desc_t *mptr)
{
  format_list_t *fptr;
  int ix;
  
  printf("\tMedia type: %s\n", mptr->media);
  printf("\tMedia proto: %s\n", mptr->proto);
  if (mptr->media_desc)
    printf("\tMedia description: %s\n", mptr->media_desc);
  if (mptr->media_connect.used) {
    printf("\tMedia Address: %s", mptr->media_connect.conn_addr);
    if (strcasecmp(mptr->media_connect.conn_type, "IP4") != 0) {
      printf("(address type %s)", mptr->media_connect.conn_type);
    }
    if (mptr->media_connect.num_addr > 0) {
      printf("(%u addresses)", mptr->media_connect.num_addr);
    }
    printf("\n\tMedia TTL: %u\n", mptr->media_connect.ttl);
  }
  printf("\tMedia port number: %d", mptr->port);
  if (mptr->num_ports > 1) {
    printf("/%d", mptr->num_ports);
  }
  printf("\n");

  if (mptr->rtcp_connect.used) {
    if (mptr->rtcp_connect.conn_addr != NULL) {
      printf("\tRTCP Address: %s:%u", mptr->rtcp_connect.conn_addr, mptr->rtcp_port);
    } else {
      printf("\tRTCP port: %u", mptr->rtcp_port);
    }
    if (mptr->rtcp_connect.conn_type != NULL && 
	strcasecmp(mptr->rtcp_connect.conn_type, "IP4") != 0) {
      printf("(address type %s)", mptr->rtcp_connect.conn_type);
    }
    if (mptr->media_connect.num_addr > 0) {
      printf("(%u addresses)", mptr->rtcp_connect.num_addr);
    }
    if (mptr->rtcp_connect.ttl > 0) 
      printf("\n\tRTCP TTL: %u\n", mptr->rtcp_connect.ttl);
  }

  bandwidth_dump(mptr->media_bandwidth, "\n\tMedia");
  if (mptr->recvonly) {
    printf("\tReceive Only Set\n");
  }
  if (mptr->sendrecv) {
    printf("\tSend/Receive Set\n");
  }
  if (mptr->sendonly) {
    printf("\tSend Only Set\n");
  }
  if (mptr->ptime_present) {
    printf("\tPacket Time: %d\n", mptr->ptime);
  }
  if (mptr->quality_present) {
    printf("\tQuality: %d\n", mptr->quality);
  }
  if (mptr->framerate_present) {
    printf("\tFramerate: %f\n", mptr->framerate);
  }
  if (mptr->control_string) {
    printf("\tControl: %s\n", mptr->control_string);
  }
  range_dump(&mptr->media_range, "\t");
  printf("\tMedia formats: ");
  fptr = mptr->fmt_list;
  ix = 0;
  while (fptr != NULL) {
    if (ix >= 6) {
      printf("\n\t\t");
      ix = 0;
    }

    printf("%s", fptr->fmt);
    if (fptr->rtpmap_name != NULL) {
      printf("(%s %u", fptr->rtpmap_name, fptr->rtpmap_clock_rate);
      if (fptr->rtpmap_encode_param != 0){
	printf(" %u", fptr->rtpmap_encode_param);
      }
      printf(")");
      ix += 2;
    }
    if (fptr->fmt_param != NULL) {
      printf("(%s)", fptr->fmt_param);
      ix += 4;
    }
    printf(" ");
    ix++;
    fptr = fptr->next;
  }
  printf("\n");
  unparsed_dump(mptr->unparsed_a_lines, "\t");
}
  
void session_dump_one (session_desc_t *sptr)
{
  int ix;
  media_desc_t *media;
  string_list_t *strptr;
  
  printf("Session name: %s\n", sptr->session_name);
  if (sptr->session_desc != NULL) {
    printf("Description: %s\n", sptr->session_desc);
  }
  if (sptr->uri != NULL) {
    printf("URI: %s\n", sptr->uri);
  }
  if (sptr->orig_username) {
    printf("Origin username: %s\n", sptr->orig_username);
  }
  printf("Session id: "D64"\nSession version: "D64"\n",
	 sptr->session_id,
	 sptr->session_version);
  if (sptr->create_addr)
    printf("Session created by: %s", sptr->create_addr);
  if (sptr->create_addr_type &&
      strcasecmp(sptr->create_addr_type, "IP4") != 0) {
    printf("address type %s", sptr->create_addr_type);
  }
  printf("\n");
  if (sptr->conf_type != CONFERENCE_TYPE_NONE) {
    printf("Conference type: %s\n",
	   sptr->conf_type < CONFERENCE_TYPE_OTHER ?
	   type_values[sptr->conf_type - 1] : sptr->conf_type_user);
  }
    
  if (sptr->keywds) printf("Keywords: %s\n", sptr->keywds);
  if (sptr->tool) printf("Tool: %s\n", sptr->tool);
  category_dump(sptr->category_list);

  strptr = sptr->admin_email;
  while (strptr != NULL) {
    printf("Admin email: %s\n", strptr->string_val);
    strptr = strptr->next;
  }
  strptr = sptr->admin_phone;
  while (strptr != NULL) {
    printf("Admin phone: %s\n", strptr->string_val);
    strptr = strptr->next;
  }
  key_dump(&sptr->key);
  time_dump(sptr->time_desc);
  time_adj_dump(sptr->time_adj_desc);
  if (sptr->session_connect.used) {
    printf("Session Address: %s", sptr->session_connect.conn_addr);
    if (strcasecmp(sptr->session_connect.conn_type, "IP4") != 0) {
      printf("(address type %s)", sptr->session_connect.conn_type);
    }
    if (sptr->session_connect.num_addr > 0) {
      printf("(%u addresses)", sptr->session_connect.num_addr);
    }
    printf("\ntSession TTL: %u\n", sptr->session_connect.ttl);
  }
  range_dump(&sptr->session_range, "");
  bandwidth_dump(sptr->session_bandwidth, "Session");
  if (sptr->control_string) {
    printf("Control: %s\n", sptr->control_string);
  }
  unparsed_dump(sptr->unparsed_a_lines, "");
  ix = 0;
  media = sptr->media;
  while (media != NULL) {

    printf("Media description %d:\n", ix + 1);
    media_dump(media);
    media = media->next;
    ix++;
  }
  if (ix == 0) {
    printf("No media descriptions for session\n");
  }
}

void session_dump_list (session_desc_t *sptr)
{
  while (sptr != NULL) {
    session_dump_one(sptr);
    printf("\n");
    sptr = sptr->next;
    if (sptr) {
      printf("------------------------------------------------------\n");
    }
  }
}
