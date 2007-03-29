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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */
/* mpeg2_transport.c - mpeg2 transport stream decoding */

#include "mpeg4ip.h"
#include "mpeg2_transport.h"
#include <assert.h>
#include "mpeg2t_private.h"

//#define DUMP_ADAPTION_CONTROL 1
#define DEBUG 1
#ifdef DEBUG
#define CHECK_MP2T_HEADER assert(*pHdr == MPEG2T_SYNC_BYTE)
#else
#define CHECK_MP2T_HEADER 
#endif
//#define DEBUG_MPEG2T 1
/*
 * mpeg2 transport layer header routines.
 */
uint32_t mpeg2t_find_sync_byte (const uint8_t *buffer, uint32_t buflen)
{
  uint32_t offset;

  offset = 0;

  while (offset < buflen) {
    if (buffer[offset] == MPEG2T_SYNC_BYTE) {
      return (offset);
    }
    offset++;
  }
  return (offset);
}

uint32_t mpeg2t_transport_error_indicator (const uint8_t *pHdr)
{

  CHECK_MP2T_HEADER;

  return ((pHdr[1] >> 7) & 0x1);
}

uint32_t mpeg2t_payload_unit_start_indicator (const uint8_t *pHdr)
{
  CHECK_MP2T_HEADER;

  return ((pHdr[1] >> 6) & 0x1);
}

uint16_t mpeg2t_pid (const uint8_t *pHdr)
{
  int pid;
  CHECK_MP2T_HEADER;

  pid = (pHdr[1] & 0x1f) << 8;
  pid |= pHdr[2];

  return (pid);
}

uint32_t mpeg2t_adaptation_control (const uint8_t *pHdr)
{
  CHECK_MP2T_HEADER;

  return ((pHdr[3] >> 4) & 0x3);
}

uint32_t mpeg2t_continuity_counter (const uint8_t *pHdr)
{
  CHECK_MP2T_HEADER;

  return (pHdr[3] & 0xf);
}

/*
 * mpeg2t_transport_payload_start
 * find the start of the payload data, skipping any adaptation data
 */
const uint8_t *mpeg2t_transport_payload_start (const uint8_t *pHdr,
					       uint32_t *payload_len)
{
  uint32_t adaption_control;
#if DUMP_ADAPTION_CONTROL
  int ix, pid;
#endif
  CHECK_MP2T_HEADER;

  if (mpeg2t_transport_error_indicator(pHdr) != 0) {
    *payload_len = 0;
    return NULL;
  }

  adaption_control = mpeg2t_adaptation_control(pHdr);

  if (adaption_control == 1) {
    *payload_len = 184;
    return pHdr + 4;
  }
  if (adaption_control == 3) {
    if (pHdr[4] > 183) {
      *payload_len = 0;
      return NULL;
    }
    *payload_len = 183 - pHdr[4];
#if DUMP_ADAPTION_CONTROL
    pid = ((pHdr[1] << 8) | pHdr[2]) & 0x1fff;
    mpeg2t_message(LOG_ERR, "pid %x adaptation control - len %d", 
		   pid, pHdr[4]);
    if ((pHdr[5] & 0x10) != 0) {
      uint64_t pcr;
      uint16_t ext;
      pcr = pHdr[6]; 
      pcr = (pcr << 8) | pHdr[7];
      pcr = (pcr << 8) | pHdr[8];
      pcr = (pcr << 8) | pHdr[9];
      pcr <<= 1;
      if ((pHdr[10] & 0x80) != 0) { 
	pcr |= 1;
      }
      ext = (pHdr[10] & 0x1) << 8;
      ext |= pHdr[11];
      mpeg2t_message(LOG_ERR, "pid %x pcr "U64" ext %u",
		     pid, pcr, ext);
    }
    for (ix = 0; ix < pHdr[4]; ix += 4) {
      mpeg2t_message(LOG_ERR, "pid %x %d - %02x %02x %02x %02x", 
		     pid, ix, 
		     pHdr[4 + ix],
		     pHdr[5 + ix],
		     pHdr[6 + ix],
		     pHdr[7 + ix]);
    }
#endif
    
    return pHdr + 5 + pHdr[4];
  }

  *payload_len = 0;
  return NULL;
}

/*
 * mpeg2t_start_join_pak - this handles PMAP or PAS that are more
 * than 1 transport stream packet. It should also handle any other
 * type packet.
 * Elementary streams use their own processing routines.
 */
static void mpeg2t_start_join_pak (mpeg2t_pid_t *pidptr,
				   const uint8_t *bufstart,
				   uint32_t buflen,
				   uint32_t seqlen, 
				   uint32_t cc)
{
  if (seqlen == 0) {
    if (pidptr->data_len_max < buflen) {
      pidptr->data = (uint8_t *)realloc(pidptr->data, buflen + 4096);
      if (pidptr->data == NULL) {
	pidptr->data_len_max = 0;
	return;
      }
      pidptr->data_len_max = buflen + 4096;
    }
  } else if (seqlen > pidptr->data_len_max) {
    pidptr->data = (uint8_t *)realloc(pidptr->data, seqlen);
    if (pidptr->data == NULL) {
      pidptr->data_len_max = 0;
      return;
    }
    pidptr->data_len_max = seqlen;
  }
  pidptr->data_len = seqlen;
  pidptr->data_len_loaded = buflen;
  memcpy(pidptr->data, bufstart, buflen);
  pidptr->lastcc = cc;
}

/*
 * mpeg2t_join_pak - add data to a started pak from above
 */
static int mpeg2t_join_pak (mpeg2t_pid_t *pidptr,
			    const uint8_t *bufstart,
			    uint32_t buflen,
			    uint32_t cc)
{
  uint32_t nextcc;
  uint32_t remaining;
  if (pidptr->data_len_loaded == 0) {
    mpeg2t_message(LOG_WARNING, 
		   "Trying to add to unstarted packet - PID %x", 
		   pidptr->pid);
    return -1;
  }
  nextcc = (pidptr->lastcc + 1) & 0xf;
  if (nextcc != cc) {
    mpeg2t_message(LOG_ERR, "Illegal cc value %d - should be %d - PID %x", 
		   cc, nextcc, pidptr->pid);
    pidptr->data_len_loaded = 0;
    return -1;
  }
  pidptr->lastcc = cc;
  if (pidptr->data_len == 0) {
    remaining = pidptr->data_len_max - pidptr->data_len_loaded;
    if (remaining < buflen) {
      pidptr->data = (uint8_t *)realloc(pidptr->data, 
					pidptr->data_len_max + 4096);
      if (pidptr->data == NULL) {
	pidptr->data_len_max = 0;
	return -1;
      }
      pidptr->data_len_max = pidptr->data_len_max + 4096;
    }
  } else {
    remaining = pidptr->data_len - pidptr->data_len_loaded;

    buflen = buflen > remaining ? remaining : buflen;
  }
  memcpy(pidptr->data + pidptr->data_len_loaded,
	 bufstart, 
	 buflen);
  pidptr->data_len_loaded += buflen;
  if (pidptr->data_len == 0 || pidptr->data_len_loaded < pidptr->data_len) 
    return 0;
  // Indicate that next one starts from beginning
  pidptr->data_len_loaded = 0;
  return 1;
}

/*
 * mpeg2t_lookup_pid - search through the list for the pid pointer 
 */
mpeg2t_pid_t *mpeg2t_lookup_pid (mpeg2t_t *ptr,
				 uint16_t pid)
{
  mpeg2t_pid_t *pidptr;

  SDL_LockMutex(ptr->pid_mutex);
  pidptr = &ptr->pas.pid;

  while (pidptr != NULL && pidptr->pid != pid) {
    pidptr = pidptr->next_pid;
  }
  SDL_UnlockMutex(ptr->pid_mutex);
  return pidptr;
}

/*
 * add_to_pidQ - add a PID to the queue
 */
static void add_to_pidQ (mpeg2t_t *ptr, mpeg2t_pid_t *pidptr)
{
  mpeg2t_pid_t *p = &ptr->pas.pid;

  pidptr->main = ptr;
  SDL_LockMutex(ptr->pid_mutex);
  while (p->next_pid != NULL) {
    p = p->next_pid;
  }
  p->next_pid = pidptr;
  SDL_UnlockMutex(ptr->pid_mutex);
}

/*
 * create_pmap - create a program map.  This is called while processing
 * PAS statements.  From here, we should get a PMAP packet for this
 * particular PID.  That will give us the elementary streams that are
 * part of the PMAP  (PMAP = program map)
 */
static void create_pmap (mpeg2t_t *ptr, uint16_t prog_num, uint16_t pid)
{
  mpeg2t_pmap_t *pmap;

  mpeg2t_message(LOG_NOTICE, "Adding pmap prog_num %x pid %x", prog_num, pid);
  pmap = MALLOC_STRUCTURE(mpeg2t_pmap_t);
  if (pmap == NULL) return;
  memset(pmap, 0, sizeof(*pmap));
  pmap->pid.pak_type = MPEG2T_PROG_MAP_PAK;
  pmap->pid.pid = pid;
  pmap->program_number = prog_num;
  add_to_pidQ(ptr, &pmap->pid);
  ptr->program_count++;
}

/*
 * create_es - create elementary stream pid.  This is called while
 * processing PMAPs.
 */
static void create_es (mpeg2t_t *ptr, 
		       uint16_t pid, 
		       uint16_t prog_num,
		       uint8_t stream_type,
		       const uint8_t *es_data,
		       uint32_t es_info_len)
{
  mpeg2t_es_t *es;

  mpeg2t_message(LOG_INFO, 
		 "Adding ES PID %x stream type %d", pid, stream_type);
  es = MALLOC_STRUCTURE(mpeg2t_es_t);
  if (es == NULL) return;
  memset(es, 0, sizeof(*es));
  es->list_mutex = SDL_CreateMutex();
  if (es->list_mutex == NULL) {
    mpeg2t_message(LOG_ERR, "Can't malloc mutex");
    free(es);
    return;
  }
  es->pid.pak_type = MPEG2T_ES_PAK;
  es->pid.pid = pid;
  es->prog_num = prog_num;
  es->stream_type = stream_type;
  switch (es->stream_type) {
  case 1:
  case 2:
    es->is_video = 1;
    break;
  case 0x10:
    es->is_video = 3; // mpeg4
    break;
  case 0x1b:
    es->is_video = 2;
    break;
  default:
    es->is_video = 0;
  }
  if (es_info_len != 0) {
    es->es_data = (uint8_t *)malloc(es_info_len);
    if (es->es_data != NULL) {
      memcpy(es->es_data, es_data, es_info_len);
      es->es_info_len = es_info_len;
    }
    mpeg2t_message(LOG_ERR, "pid %x - es len %d %p", pid, es_info_len,
		   es->es_data);
#if 0
 {
  uint32_t ix;
    for (ix = 0; ix < es_info_len; ix += 4) {
      mpeg2t_message(LOG_ERR, "%d - %02x %02x %02x %02x", 
		     ix, es_data[ix], 
		     es_data[ix + 1],
		     es_data[ix + 2],
		     es_data[ix + 3]);
    }
 }
#endif
  }
  es->work_max_size = 4096;
  es->save_frames = ptr->save_frames_at_start;
  add_to_pidQ(ptr, &es->pid);
}
  
/*
 * mpeg2t_process_pas - process the PAS.  The PAS is a list of PMAP
 * pids.
 */
static int mpeg2t_process_pas (mpeg2t_t *ptr, const uint8_t *buffer)
{
  uint32_t buflen;
  uint32_t section_len;
  uint32_t len;
  const uint8_t *mapptr;
  uint16_t prog_num, pid;
  const uint8_t *pasptr;
  int ret;

  buflen = 188;
  // process pas pointer
  pasptr = mpeg2t_transport_payload_start(buffer, &buflen);

  if (pasptr == NULL) return 0;

  if (mpeg2t_payload_unit_start_indicator(buffer) == 0) {
    ret = mpeg2t_join_pak(&ptr->pas.pid, 
			  pasptr,
			  buflen,
			  mpeg2t_continuity_counter(buffer));
    if (ret <= 0) return 0; // not done, or bad
    pasptr = ptr->pas.pid.data;
    section_len = ptr->pas.pid.data_len;
  } else {
    if (*pasptr >= buflen) 
      return 0;
    buflen -= *pasptr + 1;
    pasptr += *pasptr + 1; // go through the pointer field
    
    if (*pasptr != 0 || (pasptr[1] & 0xc0) != 0x80) {
      mpeg2t_message(LOG_ERR, "PAS field not 0");
      return 0;
    }
    section_len = ((pasptr[1] << 8) | pasptr[2]) & 0x3ff; 
    // remove table_id, section length fields
    pasptr += 3;
    buflen -= 3;
    if (buflen < section_len) {
      mpeg2t_start_join_pak(&ptr->pas.pid,
			    pasptr, // start after section len
			    buflen,
			    section_len,
			    mpeg2t_continuity_counter(buffer));
      return 0;
    }
    // At this point, pasptr points to transport_stream_id
  }
  
  ptr->pas.transport_stream_id = ((pasptr[0] << 8 | pasptr[1]));
  ptr->pas.version_number = (pasptr[2] >> 1) & 0x1f;
  mapptr = &pasptr[5];
  section_len -= 5 + 4; // remove CRC and stuff before map list
  /*
   * Now, we have a list of PMAPs - for each one, see if we've
   * already created a PIDPTR for it - if not, do so
   */
  for (len = 0; len < section_len; len += 4, mapptr += 4) {
    prog_num = (mapptr[0] << 8) | mapptr[1];
    if (prog_num != 0) {
      pid = ((mapptr[2] << 8) | mapptr[3]) & 0x1fff;
      if (mpeg2t_lookup_pid(ptr, pid) == NULL) {
	create_pmap(ptr, prog_num, pid);
	ptr->pas.programs++; // add to program count
      }
    } // else network information table
  }
  return 1;
}

/*
 * mpeg2t_process_pmap - process a PMAP, creating the elementary
 * stream PIDs
 */
static int mpeg2t_process_pmap (mpeg2t_t *ptr, 
				mpeg2t_pid_t *ifptr,
				const uint8_t *buffer)
{
  uint32_t buflen;
  uint32_t section_len;
  uint32_t len, es_len;
  uint16_t prog_num, pcr_pid;
  const uint8_t *pmapptr;
  mpeg2t_pmap_t *pmap_pid = (mpeg2t_pmap_t *)ifptr;
  int ret;
  uint8_t stream_type;
  uint16_t e_pid;
  uint32_t ix;

  buflen = 188;
  // process pas pointer
  pmapptr = mpeg2t_transport_payload_start(buffer, &buflen);
  if (pmapptr == NULL) return 0;

  if (mpeg2t_payload_unit_start_indicator(buffer) == 0) {
    ret = mpeg2t_join_pak(ifptr, 
			  pmapptr,
			  buflen,
			  mpeg2t_continuity_counter(buffer));
    if (ret <= 0) return 0;
    pmapptr = ifptr->data;
    section_len = ifptr->data_len;
  } else {
    if (*pmapptr >= buflen) 
      return 0;
    buflen -= *pmapptr + 1;
    pmapptr += *pmapptr + 1; // go through the pointer field
    if (*pmapptr != 2 || (pmapptr[1] & 0xc0) != 0x80) {
      mpeg2t_message(LOG_ERR, "PMAP start field not 2");
      return 0;
    }
    section_len = ((pmapptr[1] << 8) | pmapptr[2]) & 0x3ff; 
    pmapptr += 3;
    buflen -= 3;

    if (buflen < section_len) {
      mpeg2t_start_join_pak(ifptr,
			    pmapptr,
			    buflen,
			    section_len,
			    mpeg2t_continuity_counter(buffer));
      return 0;
    }
    // pmapptr points to program number
  }

			    
  prog_num = ((pmapptr[0] << 8) | pmapptr[1]);
#if 0
  if (prog_num != pmap_pid->program_number) {
    mpeg2t_message(LOG_ERR, 
		   "Prog Map error - program number doesn't match - pid %x orig %x from pak %x", 
		   pmap_pid->pid.pid, pmap_pid->program_number, prog_num);
    return 0;
  }
#endif
  pmap_pid->version_number = (pmapptr[2] >> 1) & 0x1f;

  pcr_pid = ((pmapptr[5] << 8) | pmapptr[6]) & 0x1fff;
  if (pcr_pid != 0x1fff) {
    mpeg2t_message(LOG_DEBUG, "Have PCR pid of %x", pcr_pid);
  }
  pmapptr += 7;
  section_len -= 7; // remove all the fixed fields to get the prog info len
  len = ((pmapptr[0] << 8) | pmapptr[1]) & 0xfff;
  pmapptr += 2;
  section_len -= 2;
  if (len != 0) {
    // This is the prog info - we don't do anything with it yet.
    if (len > section_len) return 0;
    mpeg2t_message(LOG_ERR, "have prog info len %d", len);
    for (ix = 0; ix < len; ix += 4) {
      mpeg2t_message(LOG_ERR, "%d - %02x %02x %02x %02x", 
		     ix, pmapptr[ix], 
		     pmapptr[ix + 1],
		     pmapptr[ix + 2],
		     pmapptr[ix + 3]);
    }
    if (len == pmap_pid->prog_info_len) {
      // same length - do a compare
    } else {
    }
    pmapptr += len;
    section_len -= len;
  }
  section_len -= 4; // remove CRC
  len = 0;
  if (section_len == 0) {
    return 0;
  }
  /*
   * Now, add all elementary streams for this PMAP
   */
  if (pmap_pid->received == 0) {
    pmap_pid->received = 1;
    ptr->program_maps_recvd++;
    ptr->pas.programs_added++;
  }
  while (len < section_len) {
    stream_type = pmapptr[0];
    e_pid = ((pmapptr[1] << 8) | pmapptr[2]) & 0x1fff;
    es_len = ((pmapptr[3] << 8) | pmapptr[4]) & 0xfff;
    if (es_len + len > section_len) return 0;
    if (mpeg2t_lookup_pid(ptr, e_pid) == NULL) {
      mpeg2t_message(LOG_NOTICE, "Creating es pid %x", e_pid);
      create_es(ptr, e_pid, pmap_pid->program_number, stream_type, &pmapptr[5], es_len);
    }
    // create_es
    len += 5 + es_len;
    pmapptr += 5 + es_len;
  }
  return 1;
}

/*
 * clean_es_data
 * clean all data needed for processing the stream.  This 
 * should really be broken out into other files, like the
 * frame processing.
 */
static void clean_es_data (mpeg2t_es_t *es_pid) 
{
  es_pid->have_ps_ts = 0;
  es_pid->have_dts = 0;
  if (es_pid->is_video > 0) {
    // mpeg1 or mpeg2 video
    es_pid->work_state = 0;
    es_pid->header = 0;
    es_pid->work_loaded = 0;
    es_pid->have_seq_header = 0;
    if (es_pid->work != NULL) {
      mpeg2t_malloc_es_work(es_pid, es_pid->work->frame_len);
    }
  } else {
    // mpeg1/mpeg2 audio (mp3 codec)
    if (es_pid->work != NULL ) {
      free(es_pid->work);
      es_pid->work = NULL;
    }
    es_pid->work_loaded = 0;
    es_pid->left = 0;
  }
}  

/*
 * mpeg2t_malloc_es_work - create or clean a mpeg2t_frame_t for an
 * elementary stream.
 */
void mpeg2t_malloc_es_work (mpeg2t_es_t *es_pid, uint32_t frame_len)
{
  uint8_t *frameptr;

#if 0
  mpeg2t_message(LOG_DEBUG, "es %x malloc %d", es_pid->pid.pid,
		 es_pid->have_ps_ts);
#endif

  if (es_pid->work == NULL || es_pid->work->frame_len < frame_len) {
    if (es_pid->work != NULL) {
      free(es_pid->work);
      es_pid->work = NULL;
    }
    frameptr = (uint8_t *)malloc(sizeof(mpeg2t_frame_t) + frame_len);
    if (frameptr == NULL) return;
    es_pid->work = (mpeg2t_frame_t *)frameptr;
    es_pid->work->frame = frameptr + sizeof(mpeg2t_frame_t);
    es_pid->work->frame_len = frame_len;
  }
  es_pid->work->next_frame = NULL;
  es_pid->work->have_ps_ts = es_pid->have_ps_ts;
  es_pid->work->have_dts = es_pid->have_dts;
  es_pid->work->ps_ts = es_pid->ps_ts;
  es_pid->work->dts = es_pid->dts;
  es_pid->have_ps_ts = 0;
  es_pid->have_dts = 0;
}

/*
 * mpeg2t_finished_es_work - when we have a frame, this is 
 * called to save the frame on the list (if so configured)
 */
void mpeg2t_finished_es_work (mpeg2t_es_t *es_pid,
			      uint32_t frame_len)
{
  mpeg2t_frame_t *p;
#if 1
  mpeg2t_message(LOG_WARNING, "pid %x pts %d "U64" listing %d", 
		 es_pid->pid.pid, es_pid->work->have_ps_ts, 
		 es_pid->work->ps_ts, es_pid->save_frames);
#endif
  SDL_LockMutex(es_pid->list_mutex);
  if (es_pid->save_frames == 0) {
    mpeg2t_malloc_es_work(es_pid, es_pid->work->frame_len);
  } else {
    es_pid->work->frame_len = frame_len;
    if (es_pid->list == NULL) {
      es_pid->list = es_pid->work;
    } else {
      p = es_pid->list;
      while (p->next_frame != NULL) p = p->next_frame;
      p->next_frame = es_pid->work;
    }
    es_pid->frames_in_list++;
    es_pid->work = NULL;
  }
  es_pid->work_loaded = 0;
  SDL_UnlockMutex(es_pid->list_mutex);
}

/*
 * mpeg2t_get_es_list_head - get the first frame on the list
 */
mpeg2t_frame_t *mpeg2t_get_es_list_head (mpeg2t_es_t *es_pid)
{
  mpeg2t_frame_t *p;
  SDL_LockMutex(es_pid->list_mutex);
  p = es_pid->list;
  if (p != NULL) {
    es_pid->list = p->next_frame;
    p->next_frame = NULL;
    es_pid->frames_in_list--;
  }
  SDL_UnlockMutex(es_pid->list_mutex);
  return p;
}

void mpeg2t_free_frame (mpeg2t_frame_t *fptr)
{
  free(fptr);
}
/*
 * mpeg2t_process_es - process a transport stream pak for an
 * elementary stream
 */
static int mpeg2t_process_es (mpeg2t_t *ptr, 
			      mpeg2t_pid_t *ifptr,
			      const uint8_t *buffer)
{
  uint32_t buflen;
  uint32_t pes_len;
  const uint8_t *esptr;
  mpeg2t_es_t *es_pid = (mpeg2t_es_t *)ifptr;
  int read_pes_options;
  uint8_t stream_id;
  uint32_t nextcc, pakcc;
  int ret;
  int ac;
  int have_psts;
  uint64_t pts;
  uint32_t offset;
#ifdef DEBUG_MPEG2T
  uint32_t ix;
#endif


  ac = mpeg2t_adaptation_control(buffer);
  // Note to self - if ac is 0x3, we may have to check
  // the discontinuity indicator in the adapation control - this
  // can cause a cc to be non-sequential, as well.
  // ASDF - not implemented yet
  nextcc = ifptr->lastcc;
  if (!(ac == 0 || ac == 2)) {
    // don't add to continuity counter if adaptation counter is 0 or 2
    nextcc = (nextcc + 1) & 0xf;
  }
  pakcc = mpeg2t_continuity_counter(buffer);
  if (nextcc != pakcc) {
    // Note - this message will occur for the first packet
    mpeg2t_message(LOG_ERR, "cc error in PID %x should be %d is %d %x", 
	   ifptr->pid, nextcc, pakcc, buffer[3]);
    clean_es_data(es_pid);
  }
  ifptr->lastcc = pakcc;

  if (es_pid->save_frames == 0 &&
      es_pid->report_psts == 0 &&
      es_pid->info_loaded > 0) {
    //    mpeg2t_message(LOG_INFO, "PID %x not processing", ifptr->pid);
    return 0;
  }

  buflen = 188;
  // process pas pointer
  esptr = mpeg2t_transport_payload_start(buffer, &buflen);
  if (esptr == NULL) return -1;
  
  have_psts = 0;
  if (mpeg2t_payload_unit_start_indicator(buffer) != 0) {
    // start of PES packet
    es_pid->peshdr_loaded = 1;
    if ((esptr[0] != 0) ||
	(esptr[1] != 0) ||
	(esptr[2] != 1)) {
      mpeg2t_message(LOG_ERR, 
		     "Illegal start to PES packet - pid %x - %02x %02x %02x",
		     ifptr->pid, esptr[0], esptr[1], esptr[2]);
      return -1;
    }
    stream_id = es_pid->stream_id = esptr[3];
    pes_len = (esptr[4] << 8) | esptr[5];
    esptr += 6;
    buflen -= 6;

    read_pes_options = 0;
    // do we have header extensions
    switch ((stream_id & 0x70) >> 4) {
    default:
      if ((stream_id == 0xbd) ||
	    (stream_id >= 0xf3 && stream_id <= 0xf7) ||
	    (stream_id >= 0xf9)) {
	read_pes_options = 1;
	break;
      }
      // fall through
    case 4:
    case 5:
    case 6:
      if (esptr[2] <= buflen - 3) {
	// multiple PES for header
	read_pes_options = 1;
      } else {
	// don't have enough to read the header
      }
      break;
    }

    es_pid->have_ps_ts = 0;
    es_pid->have_dts = 0;
    if (read_pes_options) {
      if (esptr[2] + 3U > buflen) {
	return 0;
      }
      mpeg2t_message(LOG_INFO, "pid %x pes bits %x %x", 
		     es_pid->pid.pid, esptr[0], esptr[1]);
#ifdef DEBUG_MPEG2T
      if ((esptr[1] & 0x20)) {
	mpeg2t_message(LOG_INFO, "pid %x has ESCR", es_pid->pid.pid);
      }
      if ((esptr[1] & 0x10)) {
	mpeg2t_message(LOG_INFO, "pid %x has ES rate", es_pid->pid.pid);
      }
#endif
      offset = 3;
      //mpeg2t_read_pes_options(es_pid, esptr);
      if (esptr[1] & 0x80) {
	// read presentation timestamp
#if 0
	mpeg2t_message(LOG_INFO, "Stream %x %02x %02x %02x", 
	       stream_id, esptr[0], esptr[1], esptr[2]);
	mpeg2t_message(LOG_INFO, "PTS %02x %02x %02x %02x %02x", 
	       esptr[3], esptr[4], esptr[5], esptr[6], esptr[7]);
#endif
	if (((esptr[1] >> 6) & 0x3) !=
	    ((esptr[3] >> 4) & 0xf)) {
	  mpeg2t_message(LOG_ERR, "PID %x Timestamp flag value not same %x %x",
			 es_pid->pid.pid, esptr[1], esptr[2]);
	  return -1;
	}
	pts = ((esptr[3] >> 1) & 0x7);
	pts <<= 8;
	pts |= esptr[4];
	pts <<= 7;
	pts |= ((esptr[5] >> 1) & 0x7f);
	pts <<= 8;
	pts |= esptr[6];
	pts <<= 7;
	pts |= ((esptr[7] >> 1) & 0x7f);
	es_pid->have_ps_ts = 1;
	es_pid->ps_ts = pts; // (pts * M_64) / (90 * M_64); // give msec
	have_psts = 1;
	mpeg2t_message(LOG_INFO, "pid %x psts "U64, 
		      es_pid->pid.pid, es_pid->ps_ts);
	offset = 8;
      }

      // this is how we would read the dts, if we wanted to use it
      if (esptr[1] & 0x40) {
	pts = ((esptr[offset] >> 1) & 0x7);
	pts <<= 8;
	pts |= esptr[offset + 1];
	pts <<= 7;
	pts |= ((esptr[offset + 2] >> 1) & 0x7f);
	pts <<= 8;
	pts |= esptr[offset + 3];
	pts <<= 7;
	pts |= ((esptr[offset + 4] >> 1) & 0x7f);
	mpeg2t_message(LOG_INFO, "pid %x  dts "U64,
		       es_pid->pid.pid, pts);
	have_psts = 1;
	es_pid->have_dts = 1;
	es_pid->dts = pts;
      }
#ifdef DEBUG_MPEG2T
      mpeg2t_message(LOG_INFO, "pid %x pes buflen %d\n", es_pid->pid.pid, esptr[2]);
      for (ix = 0; ix < esptr[2]; ix += 4) {
	mpeg2t_message(LOG_ERR, "pid %x %d - %02x %02x %02x %02x", 
		       es_pid->pid.pid, ix, 
		       esptr[3 + ix],
		       esptr[4 + ix],
		       esptr[5 + ix],
		       esptr[6 + ix]);
      }
#endif
      buflen -= esptr[2] + 3;
      esptr += esptr[2] + 3;
      if (pes_len > 0)
	pes_len -= esptr[2] + 3;
    }
  // process esptr, buflen
    if (buflen == 0) {
      es_pid->have_ps_ts = 0;
      es_pid->have_dts = 0;
      return 0;
    }
    mpeg2t_message(LOG_DEBUG, 
		   "%x PES start stream id %02x len %u (%u)", ifptr->pid, 
		   stream_id, pes_len, buflen);
  } else {
    // 0 in Payload start - process frame at start
    read_pes_options = 0;
  }
  // have start of data is at esptr, buflen data
  ret = 0;
  switch (es_pid->stream_type) {
  case 1:
  case 2:
    // mpeg1 or mpeg2 video
    ret = process_mpeg2t_mpeg_video(es_pid, esptr, buflen);
    break;
  case 3:
  case 4:
    // mpeg1/mpeg2 audio (mp3 codec)
    ret = process_mpeg2t_mpeg_audio(es_pid, esptr, buflen);
    break;
  case 0x10:
    ret = process_mpeg2t_mpeg4_video(es_pid, esptr, buflen);
    break;
  case 129:
    ret = process_mpeg2t_ac3_audio(es_pid, esptr, buflen);
    break;
  case 0x1b:
    ret = process_mpeg2t_h264_video(es_pid, esptr, buflen);
    break;
  case 0xf:
    // aac
    break;
  }
  //es_pid->have_ps_ts = 0;
  if (have_psts != 0 && 
      es_pid->report_psts != 0) ret = 1;
  return ret;
}

static void add_unknown_pid (mpeg2t_t *ptr, uint16_t rpid)
{
  mpeg2t_unk_pid_t *p = MALLOC_STRUCTURE(mpeg2t_unk_pid_t);
  p->next_unk = ptr->unk_pids;
  ptr->unk_pids = p;
  p->pid = rpid;
  p->count = 1;
}
  
static mpeg2t_unk_pid_t *look_up_unknown_pid (mpeg2t_t *ptr, uint16_t rpid)
{
  mpeg2t_unk_pid_t *p = ptr->unk_pids;
  while (p != NULL) {
    if (p->pid == rpid) return p;
    p = p->next_unk;
  }
  return NULL;
}

/*
 * mpeg2t_process_buffer - API routine that allows us to
 * process a buffer filled with transport streams
 */      
mpeg2t_pid_t *mpeg2t_process_buffer (mpeg2t_t *ptr, 
				     const uint8_t *buffer, 
				     uint32_t buflen,
				     uint32_t *buflen_used)
{
  uint32_t offset;
  uint32_t remaining;
  uint32_t used;
  uint16_t rpid;
  mpeg2t_pid_t *pidptr;
  int ret;

  used = 0;
  remaining = buflen;
  mpeg2t_message(LOG_DEBUG, "start processing buffer - len %d", buflen);
  if (buflen < 188) {
    *buflen_used = buflen;
    return NULL;
  }
  while (used < buflen) {
    offset = mpeg2t_find_sync_byte(buffer, remaining);
    //mpeg2t_message(LOG_DEBUG, "offset %d", offset);
    if (offset >= remaining) {
      mpeg2t_message(LOG_ERR, "sync not found in buffer");
      *buflen_used = buflen;
      return NULL;
    }
    remaining -= offset;
    buffer += offset;
    used += offset;

    if (remaining < 188) {
      *buflen_used = used;
      return NULL;
    }

    // we have a complete buffer
    rpid = mpeg2t_pid(buffer);
#if 1
    mpeg2t_message(LOG_DEBUG, "Buffer- PID %x start %d cc %d %x",
		   rpid, mpeg2t_payload_unit_start_indicator(buffer),
		   mpeg2t_continuity_counter(buffer),
		   buffer[3]);
#endif
    if (rpid == 0x1fff) {
      // just skip
    } else {
      // look up pid in table
      pidptr = mpeg2t_lookup_pid(ptr, rpid);
      if (pidptr != NULL) {
	// okay - we've got a valid pid ptr
	switch (pidptr->pak_type) {
	case MPEG2T_PAS_PAK:
	  ret = mpeg2t_process_pas(ptr, buffer);
	  if (ret > 0) {
	    *buflen_used = used + 188;
	    return pidptr;
	  }
	  break;
	case MPEG2T_PROG_MAP_PAK:
	  ret = mpeg2t_process_pmap(ptr, pidptr, buffer);
	  if (ret > 0) {
	    *buflen_used = used + 188;
	    return pidptr;
	  }
	  break;
	case MPEG2T_ES_PAK:
	  if (mpeg2t_process_es(ptr, pidptr, buffer) > 0) {
	    *buflen_used = used + 188;
	    return pidptr;
	  }
	  break;
	}
      } else {
	if (ptr->pas.programs_added >= ptr->pas.programs) {
	  mpeg2t_unk_pid_t *unk;
	  unk = look_up_unknown_pid(ptr, rpid);
	  if (unk != NULL) {
	    unk->count++;
	    if ((unk->count % 1000) == 0) {
	      mpeg2t_message(LOG_ERR, 
			     "unknown pid %x received %u packets", 
			     rpid, unk->count);
	    }
	  } else {
	    add_unknown_pid(ptr, rpid);
	  
	    mpeg2t_message(LOG_ALERT, 
			 "pid %x received - not in pas/program map table", rpid);
	  }
	}
      }
			 
    }

    used += 188;
    buffer += 188;
    remaining -= 188;
  }
  *buflen_used = buflen;
  return NULL;
}

/*
 * create_mpeg2_tranport - create the basic structure - we always
 * need a PAS, so we create one (PAS has a pid of 0).
 */  
mpeg2t_t *create_mpeg2_transport (void)
{
  mpeg2t_t *ptr;

  ptr = MALLOC_STRUCTURE(mpeg2t_t);
  memset(ptr, 0, sizeof(mpeg2t_t));
  ptr->pas.pid.pak_type = MPEG2T_PAS_PAK;
  ptr->pas.pid.next_pid = NULL;
  ptr->pas.pid.pid = 0;
  ptr->program_count = 0;
  ptr->program_maps_recvd = 0;
  ptr->pid_mutex = SDL_CreateMutex();
  return (ptr);
}

static void clean_pid (mpeg2t_pid_t *pidptr)
{
  CHECK_AND_FREE(pidptr->data);
}

static void clean_es_pid (mpeg2t_es_t *es_pid)
{
  mpeg2t_frame_t *p;
  clean_es_data(es_pid);
  
  do {
    p = mpeg2t_get_es_list_head(es_pid);
    if (p != NULL)
      free(p);
  } while (p != NULL);

  CHECK_AND_FREE(es_pid->work);
  CHECK_AND_FREE(es_pid->es_data);
  SDL_DestroyMutex(es_pid->list_mutex);
}


  
void delete_mpeg2t_transport (mpeg2t_t *ptr)
{
  mpeg2t_pid_t *pidptr, *p;
  mpeg2t_pmap_t *pmap;
  mpeg2t_unk_pid_t *unk;

  pidptr = ptr->pas.pid.next_pid;

  while (pidptr != NULL) {
    mpeg2t_message(LOG_CRIT, "cleaning %p pid %x", 
		   pidptr, pidptr->pid);
    switch (pidptr->pak_type) {
    case MPEG2T_ES_PAK:
      clean_es_pid((mpeg2t_es_t *)pidptr);
      break;
    case MPEG2T_PROG_MAP_PAK:
      pmap = (mpeg2t_pmap_t *)pidptr;
      CHECK_AND_FREE(pmap->prog_info);
      clean_pid(pidptr);
      break;
    case MPEG2T_PAS_PAK:
      clean_pid(pidptr);
      break;
    }
    p = pidptr;
    pidptr = pidptr->next_pid;
    free(p);
  }
  clean_pid(&ptr->pas.pid);
  while (ptr->unk_pids != NULL) {
    unk = ptr->unk_pids;
    ptr->unk_pids = unk->next_unk;
    free(unk);
  }
  SDL_DestroyMutex(ptr->pid_mutex);
  free(ptr);
}

/*
 * Other API routines
 */
void *mpeg2t_get_userdata (mpeg2t_pid_t *pid)
{
  return pid->userdata;
}

void mpeg2t_set_userdata (mpeg2t_pid_t *pid, void *ud)
{
  pid->userdata = ud;
}

void mpeg2t_set_frame_status (mpeg2t_es_t *es_pid, 
			      uint32_t flags)
{
  es_pid->save_frames = 0;
  es_pid->report_psts = 0;
  if (flags == MPEG2T_PID_NOTHING) {
    mpeg2t_clear_frames(es_pid);
    return;
  }
  if ((flags & MPEG2T_PID_REPORT_PSTS) != 0) {
    es_pid->report_psts = 1;
  }
  if ((flags & MPEG2T_PID_SAVE_FRAME) != 0) {
    es_pid->save_frames = 1;
  }
}

void mpeg2t_clear_frames (mpeg2t_es_t *es_pid) 
{
  SDL_LockMutex(es_pid->list_mutex);
  es_pid->have_ps_ts = 0;
  es_pid->have_dts = 0;
  es_pid->have_seq_header = 0;
  es_pid->frames_in_list = 0;
  while (es_pid->list != NULL) {
    mpeg2t_frame_t *f = es_pid->list;
    es_pid->list = f->next_frame;
    mpeg2t_free_frame(f);
  }
  clean_es_data(es_pid);
  SDL_UnlockMutex(es_pid->list_mutex);
}  

int mpeg2t_write_stream_info (mpeg2t_es_t *es_pid, 
			      char *buffer,
			      size_t buflen)
{
  int ret = -1;
  switch (es_pid->stream_type) {
  case 1:
  case 2:
    // mpeg1 or mpeg2 video
    //ret = process_mpeg2t_mpeg_video(es_pid, esptr, buflen);
    ret = mpeg2t_mpeg_video_info(es_pid, buffer, buflen);
    break;
  case 3:
  case 4:
    // mpeg1/mpeg2 audio (mp3 codec)
    ret = mpeg2t_mpeg_audio_info(es_pid, buffer, buflen);
    break;
  case 0x10:
    ret = mpeg2t_mpeg4_video_info(es_pid, buffer, buflen);
    break;
  case 129:
    ret = mpeg2t_ac3_audio_info(es_pid, buffer, buflen);
    break;
  case 0x1b:
    ret = mpeg2t_h264_video_info(es_pid, buffer, buflen);
    break;
  case 0xf:
    // aac
    break;
  }
  return ret;
}
