
#include "mpeg4ip.h"
#include "mpeg2_transport.h"
#include <assert.h>
#include "mpeg2t_private.h"
#define DEBUG 1
#ifdef DEBUG
#define CHECK_MP2T_HEADER assert(*pHdr == MPEG2T_SYNC_BYTE)
#else
#define CHECK_MP2T_HEADER 
#endif

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

const uint8_t *mpeg2t_transport_payload_start (const uint8_t *pHdr,
					     uint32_t *payload_len)
{
  uint32_t adaption_control;
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
    return pHdr + 5 + pHdr[4];
  }

  *payload_len = 0;
  return NULL;
}

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

static mpeg2t_pid_t *mpeg2t_lookup_pid (mpeg2t_t *ptr,
					uint16_t pid)
{
  mpeg2t_pid_t *pidptr = &ptr->pas.pid;

  while (pidptr != NULL && pidptr->pid != pid) {
    pidptr = pidptr->next_pid;
  }
  return pidptr;
}

static void add_to_pidQ (mpeg2t_t *ptr, mpeg2t_pid_t *pidptr)
{
  mpeg2t_pid_t *p = &ptr->pas.pid;

  while (p->next_pid != NULL) {
    p = p->next_pid;
  }
  p->next_pid = pidptr;
}

static void create_pmap (mpeg2t_t *ptr, uint16_t prog_num, uint16_t pid)
{
  mpeg2t_pmap_t *pmap;

  mpeg2t_message(LOG_INFO, "Adding pmap prog_num %x pid %x", prog_num, pid);
  pmap = MALLOC_STRUCTURE(mpeg2t_pmap_t);
  if (pmap == NULL) return;
  memset(pmap, 0, sizeof(*pmap));
  pmap->pid.pak_type = MPEG2T_PROG_MAP_PAK;
  pmap->pid.pid = pid;
  pmap->program_number = prog_num;
  add_to_pidQ(ptr, &pmap->pid);
}

static void create_es (mpeg2t_t *ptr, 
		       uint16_t pid, 
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
  es->pid.pak_type = MPEG2T_ES_PAK;
  es->pid.pid = pid;
  es->stream_type = stream_type;
  if (es_info_len != 0) {
    es->es_data = (uint8_t *)malloc(es_info_len);
    if (es->es_data != NULL) {
      memcpy(es->es_data, es_data, es_info_len);
      es->es_info_len = es_info_len;
    }
  }
  es->work_max_size = 4096;
  add_to_pidQ(ptr, &es->pid);
}
  
static void mpeg2t_process_pas (mpeg2t_t *ptr, const uint8_t *buffer)
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

  if (pasptr == NULL) return;

  if (mpeg2t_payload_unit_start_indicator(buffer) == 0) {
    ret = mpeg2t_join_pak(&ptr->pas.pid, 
			  pasptr,
			  buflen,
			  mpeg2t_continuity_counter(buffer));
    if (ret <= 0) return; // not done, or bad
    pasptr = ptr->pas.pid.data;
    section_len = ptr->pas.pid.data_len;
  } else {
    if (*pasptr + 1 > buflen) 
      return;
    buflen -= *pasptr + 1;
    pasptr += *pasptr + 1; // go through the pointer field
    
    if (*pasptr != 0 || (pasptr[1] & 0xc0) != 0x80) {
      mpeg2t_message(LOG_ERR, "PAS field not 0");
      return;
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
      return;
    }
    // At this point, pasptr points to transport_stream_id
  }
  
  ptr->pas.transport_stream_id = ((pasptr[0] << 8 | pasptr[1]));
  ptr->pas.version_number = (pasptr[2] >> 1) & 0x1f;
  mapptr = &pasptr[5];
  section_len -= 5 + 4; // remove CRC and stuff before map list
  for (len = 0; len < section_len; len += 4, mapptr += 4) {
    prog_num = (mapptr[0] << 8) | mapptr[1];
    if (prog_num != 0) {
      pid = ((mapptr[2] << 8) | mapptr[3]) & 0x1fff;
      if (mpeg2t_lookup_pid(ptr, pid) == NULL) {
	create_pmap(ptr, prog_num, pid);
      }
    }
  }
}

static void mpeg2t_process_pmap (mpeg2t_t *ptr, 
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

  buflen = 188;
  // process pas pointer
  pmapptr = mpeg2t_transport_payload_start(buffer, &buflen);
  if (pmapptr == NULL) return;

  if (mpeg2t_payload_unit_start_indicator(buffer) == 0) {
    ret = mpeg2t_join_pak(ifptr, 
			  pmapptr,
			  buflen,
			  mpeg2t_continuity_counter(buffer));
    if (ret <= 0) return;
    pmapptr = ifptr->data;
    section_len = ifptr->data_len;
  } else {
    if (*pmapptr + 1 > buflen) 
      return;
    buflen -= *pmapptr + 1;
    pmapptr += *pmapptr + 1; // go through the pointer field
    if (*pmapptr != 2 || (pmapptr[1] & 0xc0) != 0x80) {
      mpeg2t_message(LOG_ERR, "PMAP start field not 2");
      return;
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
      return;
    }
    // pmapptr points to program number
  }

			    
  prog_num = ((pmapptr[0] << 8) | pmapptr[1]);
  if (prog_num != pmap_pid->program_number) {
    mpeg2t_message(LOG_ERR, 
		   "Prog Map error - program number doesn't match - pid %x orig %x from pak %x", 
		   pmap_pid->pid.pid, pmap_pid->program_number, prog_num);
    return;
  }
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
    if (len > section_len) return;

    if (len == pmap_pid->prog_info_len) {
      // same length - do a compare
    } else {
    }
    pmapptr += len;
    section_len -= len;
  }
  section_len -= 4; // remove CRC
  len = 0;
  while (len < section_len) {
    stream_type = pmapptr[0];
    e_pid = ((pmapptr[1] << 8) | pmapptr[2]) & 0x1fff;
    es_len = ((pmapptr[3] << 8) | pmapptr[4]) & 0xfff;
    if (es_len + len > section_len) return;
    if (mpeg2t_lookup_pid(ptr, e_pid) == NULL) {
      mpeg2t_message(LOG_INFO, "Creating es pid %x", e_pid);
      create_es(ptr, e_pid, stream_type, &pmapptr[5], es_len);
    }
    // create_es
    len += 5 + es_len;
    pmapptr += 5 + es_len;
  }
}

static void clean_es_data (mpeg2t_es_t *es_pid) 
{
  switch (es_pid->stream_type) {
  case 1:
  case 2:
    // mpeg1 or mpeg2 video
    es_pid->work_state = 0;
    es_pid->header = 0;
    es_pid->work_loaded = 0;
    es_pid->have_seq_header = 0;
    break;
  case 3:
  case 4:
    // mpeg1/mpeg2 audio (mp3 codec
    if (es_pid->work != NULL ) {
      free(es_pid->work);
      es_pid->work = NULL;
    }
    es_pid->work_loaded = 0;
    es_pid->left = 0;
    break;
  case 0xf:
    // aac
    break;
  }
}  

void mpeg2t_malloc_es_work (mpeg2t_es_t *es_pid, uint32_t frame_len)
{
  uint8_t *frameptr;

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
  es_pid->work->ps_ts = es_pid->ps_ts;
  es_pid->have_ps_ts = 0;
  es_pid->work_loaded = 0;
}

void mpeg2t_finished_es_work (mpeg2t_es_t *es_pid,
			      uint32_t frame_len)
{
  mpeg2t_frame_t *p;
  es_pid->work->frame_len = frame_len;
  if (es_pid->list == NULL) {
    es_pid->list = es_pid->work;
  } else {
    p = es_pid->list;
    while (p->next_frame != NULL) p = p->next_frame;
    p->next_frame = es_pid->work;
  }
  es_pid->work = NULL;
  es_pid->work_loaded = 0;
}

  
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

  nextcc = ifptr->lastcc;
  nextcc = (nextcc + 1) & 0xf;
  pakcc = mpeg2t_continuity_counter(buffer);
  if (nextcc != pakcc) {
    mpeg2t_message(LOG_ERR, "cc error in PES %x should be %d is %d", 
	   ifptr->pid, nextcc, pakcc);
    clean_es_data(es_pid);
  }
  ifptr->lastcc = pakcc;

  buflen = 188;
  // process pas pointer
  esptr = mpeg2t_transport_payload_start(buffer, &buflen);
  if (esptr == NULL) return -1;

  
  if (mpeg2t_payload_unit_start_indicator(buffer) != 0) {
    // start of PES packet
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
	    (stream_id >= 0xf9 && stream_id <= 0xff)) {
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
      
    if (read_pes_options) {
      if (esptr[2] + 3 > buflen) {
	return 0;
      }
      //mpeg2t_read_pes_options(es_pid, esptr);
      if (((esptr[1] & 0xc0) == 0x80) ||
	  ((esptr[1] & 0xc0) == 0xc0)) {
	// read presentation timestamp
	uint64_t pts;
#if 1
	mpeg2t_message(LOG_DEBUG, "Stream %x %02x %02x %02x", 
	       stream_id, esptr[0], esptr[1], esptr[2]);
	mpeg2t_message(LOG_DEBUG, "PTS %02x %02x %02x %02x %02x", 
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
	es_pid->ps_ts = pts;
      }
      buflen -= esptr[2] + 3;
      esptr += esptr[2] + 3;
    }
  // process esptr, buflen
    if (buflen == 0) {
      es_pid->have_ps_ts = 0;
      return 0;
    }
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
    // mpeg1/mpeg2 audio (mp3 codec
    ret = process_mpeg2t_mpeg_audio(es_pid, esptr, buflen);
  break;
  case 0xf:
    // aac
    break;
  }
  es_pid->have_ps_ts = 0;
  return ret;
}
			    
      
mpeg2t_es_t *mpeg2t_process_buffer (mpeg2t_t *ptr, 
				    const uint8_t *buffer, 
				    uint32_t buflen,
				    uint32_t *buflen_used)
{
  uint32_t offset;
  uint32_t remaining;
  uint32_t used;
  uint16_t rpid;
  mpeg2t_pid_t *pidptr;

  used = 0;
  remaining = buflen;
  while (used < buflen) {
    offset = mpeg2t_find_sync_byte(buffer, remaining);
    if (offset >= remaining) {
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
    mpeg2t_message(LOG_DEBUG, "Buffer- PID %x start %d cc %d",
		   rpid, mpeg2t_payload_unit_start_indicator(buffer),
		   mpeg2t_continuity_counter(buffer));
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
	  mpeg2t_process_pas(ptr, buffer);
	  break;
	case MPEG2T_PROG_MAP_PAK:
	  mpeg2t_process_pmap(ptr, pidptr, buffer);
	  break;
	case MPEG2T_ES_PAK:
	  if (mpeg2t_process_es(ptr, pidptr, buffer) > 0) {
	    *buflen_used = used + 188;
	    return (mpeg2t_es_t *)pidptr;
	  }
	  break;
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
  
mpeg2t_t *create_mpeg2_transport (void)
{
  mpeg2t_t *ptr;

  ptr = MALLOC_STRUCTURE(mpeg2t_t);
  memset(ptr, 0, sizeof(ptr));
  ptr->pas.pid.pak_type = MPEG2T_PAS_PAK;
  ptr->pas.pid.next_pid = NULL;
  ptr->pas.pid.pid = 0;
  ptr->pas.pid.collect_pes = 1;
  return (ptr);
}
