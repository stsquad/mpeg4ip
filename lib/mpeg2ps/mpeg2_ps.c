
#include "mpeg2_ps.h"
#include "mpeg2ps_private.h"
#include <mp4av.h>
//#define DEBUG_LOC 1
//#define DEBUG_STATE 1
static void mpeg2ps_init_buffer (mpeg2ps_buffer_t *ifp, 
				 int fd)
{
  ifp->m_buffer = (uint8_t *)malloc(2048);
  ifp->m_buffer_size_max = ifp->m_buffer == NULL ? 0 : 2048;
  ifp->m_fd = fd;
}

static void mpeg2ps_free_buffer (mpeg2ps_buffer_t *ifp)
{
  CHECK_AND_FREE(ifp->m_buffer);
}

  
static void move_bytes (mpeg2ps_buffer_t *ifp, uint32_t bytestoread) 
{
  uint32_t left;
  uint32_t toreadbytes;
  uint32_t readbytes;

  // shift
  left = ifp->m_buffer_size - ifp->m_buffer_on;
  printf("move bytes %d %d\n", left, bytestoread);

  memmove(ifp->m_buffer,
	  ifp->m_buffer + ifp->m_buffer_on, 
	  left);

  bytestoread -= left;
  if (bytestoread + left > ifp->m_buffer_size_max) {
    uint32_t realloc_size = ifp->m_buffer_size_max + 
      ((bytestoread + 2047) & 0x7ff);
    printf("realloc %d to %d\n", ifp->m_buffer_size_max, bytestoread);
    ifp->m_buffer = realloc(ifp->m_buffer, realloc_size);
    ifp->m_buffer_size_max = realloc_size;
  }

  toreadbytes = ifp->m_buffer_size_max - left;

  readbytes = read(ifp->m_fd, ifp->m_buffer + left, toreadbytes);

  ifp->m_buffer_size = readbytes + left;
  ifp->m_buffer_on = 0;
}

static inline uint8_t *look_bytes (mpeg2ps_buffer_t *ifp, uint32_t bytestoread)
{
  if (ifp->m_buffer_on + bytestoread < ifp->m_buffer_size) {
    return ifp->m_buffer + ifp->m_buffer_on;
  }

  move_bytes(ifp, bytestoread);
  if (bytestoread > ifp->m_buffer_size) {
    return NULL;
  }
  return ifp->m_buffer;
}

static inline void adv_bytes (mpeg2ps_buffer_t *ifp, uint32_t done)
{
  uint32_t past_end;
  ifp->m_buffer_on += done;
  if (ifp->m_buffer_on >= ifp->m_buffer_size) {
    past_end = ifp->m_buffer_on - ifp->m_buffer_size;
    if (past_end > 0) {
      lseek(ifp->m_fd, past_end, SEEK_CUR);
    }
    ifp->m_buffer_size = read(ifp->m_fd, ifp->m_buffer, ifp->m_buffer_size_max);
    ifp->m_buffer_on = 0;
  }
}

static uint8_t *find_next_start_offset_in_buffer (mpeg2ps_buffer_t *ifp)
{
  uint32_t ix;
  uint8_t *pak;
 
  for (ix = ifp->m_buffer_on, pak = ifp->m_buffer + ifp->m_buffer_on; 
       ix < ifp->m_buffer_size - 3; ix++, pak++) {
   if (*pak == 0 &&
       pak[1] == 0 &&
       pak[2] == 1) {
     if (ix > 0)
       adv_bytes(ifp, ix - ifp->m_buffer_on);
     return look_bytes(ifp, 4);
   }
 }
 return NULL;
}

static uint8_t *find_next_start_offset (mpeg2ps_buffer_t *ifp)
{
  uint8_t *ret;
  if (ifp->m_buffer_size == 0) {
    adv_bytes(ifp, 0);
  }
  do {
    ret = find_next_start_offset_in_buffer(ifp);
    if (ret != NULL) {
      return ret;
    }
    ifp->m_buffer_on = ifp->m_buffer_size - 3;
    ret = look_bytes(ifp, 4); // should reset buffer
  } while (ret != NULL);
  return NULL;
}

      



static uint64_t read_pts (uint8_t *pak)
{
  uint64_t pts;
  uint16_t temp;
  
  pts = ((pak[0] >> 1) & 0x7);
  pts <<= 15;
  temp = convert16(&pak[1]) >> 1;
  pts |= temp;
  pts <<= 15;
  temp = convert16(&pak[3]) >> 1;
  pts |= temp;
  return pts;
}


mpeg2ps_stream_t *mpeg2ps_stream_create (int fd, 
					 uint8_t stream_id)
{
  mpeg2ps_stream_t *ptr = MALLOC_STRUCTURE(mpeg2ps_stream_t);

  memset(ptr, 0, sizeof(*ptr));

  ptr->m_stream_id = stream_id;

  mpeg2ps_init_buffer(&ptr->m_b, fd);
  
  return ptr;
}

void mpeg2ps_stream_destroy (mpeg2ps_stream_t *sptr)
{
  mpeg2ps_free_buffer(&sptr->m_b);
  CHECK_AND_FREE(sptr->frame_buffer);
  free(sptr);
}
static void adv_past_pack_hdr (mpeg2ps_stream_t *sptr, 
			       bool *have_eof)
{
  uint8_t *pak;
  uint8_t stuffed, ix;
  pak = look_bytes(&sptr->m_b, 4 + 17);
  if (pak == NULL) {
    *have_eof = TRUE;
    return;
  }
  if ((pak[4] & 0xc0) != 0x40) {
    adv_bytes(&sptr->m_b, 4 + 8);
    return;
  }
  stuffed = pak[13] & 0x7;
  ix = 0;
  if (stuffed > 0) {
    for (ix = 0; ix < stuffed && pak[14 + ix] == 0xff; ix++);
  }
  adv_bytes(&sptr->m_b, 10 + ix);
}
    
static bool mpeg2ps_stream_find_pes_hdr (mpeg2ps_stream_t *sptr,
					 bool *have_eof)
{
  uint8_t *hdr;
  uint8_t stream_id;
  while (1) {
    hdr = find_next_start_offset(&sptr->m_b);
    if (hdr == NULL) {
      *have_eof = TRUE;
      return TRUE;
    }
    stream_id = hdr[3];
    if (stream_id >= 0xbb) {
      sptr->m_state = STREAM_FIND_STREAM_ID;
      return FALSE;
    }
    if (stream_id == 0xba) {
      adv_past_pack_hdr(sptr, have_eof);
      continue;
    }
    adv_bytes(&sptr->m_b, 4); // go past this start code
  }
  return TRUE;
}

static inline uint32_t read_hdr (uint8_t *pak)
{
#ifdef WORDS_BIGENDIAN
  return *(uint32_t *)pak;
#else
  return (ntohl(*(uint32_t *)pak));
#endif
}

static bool mpeg2ps_stream_find_stream_id (mpeg2ps_stream_t *sptr, 
					   bool *have_eof)
{
  uint8_t *pak;
  uint32_t hdr;
  uint16_t pes_len;
  mpeg2ps_buffer_t *ifp = &sptr->m_b;
  while (1) {
    pak = look_bytes(ifp, 6);
    if (pak == NULL) {
      *have_eof = TRUE;
      return TRUE;
    }
    hdr = read_hdr(pak);
    if ((hdr & MPEG2_PS_START_MASK) != MPEG2_PS_START) {
      sptr->m_state = STREAM_LOOK_START_CODE;
      return FALSE;
    }
    if (hdr < MPEG2_PS_SYSSTART) {
      if (hdr == MPEG2_PS_PACKSTART) {
	// pack start code - we can skip down
	adv_past_pack_hdr(sptr, have_eof);
	continue;
      }
      adv_bytes(ifp, 4);
      sptr->m_state = STREAM_LOOK_START_CODE;
      return FALSE;
    }

    pes_len = convert16(pak + 4);
#if 0
    printf("loc: "X64" len %u\n", lseek(sptr->m_b.m_fd, 0, SEEK_CUR) -
	   sptr->m_b.m_buffer_size + sptr->m_b.m_buffer_on,
	   pes_len);
#endif
    if (pak[3] == sptr->m_stream_id) {
#ifdef DEBUG_LOC
      uint64_t loc = 
	lseek(sptr->m_b.m_fd, 0, SEEK_CUR) -
	sptr->m_b.m_buffer_size + sptr->m_b.m_buffer_on;
#endif
      // advance past header, reading pts
      sptr->have_pts = FALSE;
      adv_bytes(ifp, 6);
      pak = look_bytes(ifp, 1);
      if (pak == NULL) {
	*have_eof = TRUE;
	return TRUE;
      }
      while (*pak == 0xff) {
	adv_bytes(ifp, 1);
	pak = look_bytes(ifp, 1);
	if (pak == NULL) {
	  *have_eof = TRUE;
	  return TRUE;
	}
	pes_len--;
	if (pes_len == 0) {
	  return FALSE; // tricky way to get back into this state
	}
      }
      if ((*pak & 0xc0) == 0x40) { 
	// buffer scale & size
	adv_bytes(ifp, 2);
	pak = look_bytes(ifp, 1);
	if (pak == NULL) {
	  *have_eof = TRUE;
	  return TRUE;
	}
	pes_len -= 2;
      }

      if ((*pak & 0xf0) == 0x20) {
	// have pts
	pak = look_bytes(ifp, 5);
	sptr->have_pts = TRUE;
	sptr->pts = sptr->dts = read_pts(pak);
	pes_len -= 5;
	adv_bytes(ifp, 5);
      } else if ((*pak & 0xf0) == 0x30) {
	// have pts and dts
	pak = look_bytes(ifp, 10);
	sptr->pts = read_pts(pak);
	sptr->dts = read_pts(pak + 5);
	sptr->have_pts = TRUE;
	pes_len -= 10;
	adv_bytes(ifp, 10);
      } else if ((*pak & 0xc0) == 0x80) {
	// mpeg2 pes header  - we're pointing at the flags field now
	pak = look_bytes(ifp, 3 + 10);
	if ((pak[1] & 0xc0) == 0x80) {
	  // just pts
	  sptr->have_pts = TRUE;
	  sptr->pts = sptr->dts = read_pts(pak + 3);
	} else if ((pak[1] & 0xc0) == 0xc0) {
	  // pts and dts
	  sptr->pts = read_pts(pak + 3);
	  sptr->dts = read_pts(pak + 3  + 5);    
	}
	pes_len -= 3 + pak[2]; // remove pes header
	adv_bytes(ifp, 3 + pak[2]);
      } else if (*pak != 0xf) {
	adv_bytes(ifp, pes_len);
	return TRUE;
      } else {
	// skip past 0xf
	adv_bytes(ifp, 1);
	pes_len--;
      }
      sptr->pes_bytes_left = pes_len;
#ifdef DEBUG_LOC
      printf("stream id loc: "X64" len %u\n", loc, pes_len);
#endif

      if (sptr->frame_loaded > 0)
	sptr->m_state = STREAM_READ_FRAME;
      else
	sptr->m_state = STREAM_FIND_STREAM_HDR; 
      return FALSE;
    }
    // not the stream_id we're looking for - skip to the end
    adv_bytes(&sptr->m_b, 6 + pes_len);
  }
  
  return FALSE;
}
#define IS_MPEG_START(a) ((a) == 0xb3 || (a) == 0x00 || (a) == 0xb8)

static bool check_mpeg_header_with_leftover (mpeg2ps_stream_t *sptr, 
					     uint8_t *pak)
{
  switch (sptr->left_over_bytes_cnt) {
  case 1:
    if (pak[0] == 0 && pak[1] == 1 && 
	(IS_MPEG_START(pak[2]))) 
      return TRUE;
    break;
  case 2:
    if (pak[0] == 1 && 
	(IS_MPEG_START(pak[1]))) 
      return TRUE;
    break;
  case 3:
    if (IS_MPEG_START(pak[0])) {
      return TRUE;
    }
    break;
  }
  sptr->left_over_bytes_cnt = 0;
  return FALSE;
}

static void mpeg2ps_malloc_frame (mpeg2ps_stream_t *sptr, uint32_t len)
{
  if (len > sptr->frame_buffer_len) {
    sptr->frame_buffer = realloc(sptr->frame_buffer, len + 1);
    sptr->frame_buffer_len = len + 1;
  }

  sptr->frame_loaded = 0;
  if (sptr->left_over_bytes_cnt != 0) {
    uint32_t to_copy = sptr->left_over_bytes_cnt - sptr->left_over_bytes_start;
    sptr->frame_loaded = to_copy;
    memcpy(sptr->frame_buffer, 
	   sptr->left_over_bytes + sptr->left_over_bytes_start, 
	   to_copy);
    sptr->left_over_bytes_cnt = 0;
  }
    
}

static void copy_mpeg_to_leftover (mpeg2ps_stream_t *sptr, 
				   uint8_t *pak)
{
  if (pak[0] == 0 && pak[1] == 0 && pak[2] == 1) {
    sptr->left_over_bytes_cnt = 3;
    memcpy(sptr->left_over_bytes, pak, 3);
  } else if (pak[1] == 0 && pak[2] == 0) {
    sptr->left_over_bytes_cnt = 2;
    sptr->left_over_bytes[0] = 0;
    sptr->left_over_bytes[1] = 0;
  } else if (pak[2] == 0) {
    sptr->left_over_bytes_cnt = 1;
    sptr->left_over_bytes[0] = 0;
  } else
    sptr->left_over_bytes_cnt = 0;
}
  
static void copy_bytes_to_frame (mpeg2ps_stream_t *sptr, 
				 uint8_t *pak, 
				 uint32_t len)
{
  if (sptr->frame_loaded + len > sptr->frame_buffer_len) {
    sptr->frame_buffer = (uint8_t *)realloc(sptr->frame_buffer, 
					    sptr->frame_buffer_len + 
					    len + 4096);
    sptr->frame_buffer_len += len + 4096;
  }
  memcpy(sptr->frame_buffer + sptr->frame_loaded, 
	 pak, 
	 len);
  sptr->frame_loaded += len;
  //printf("copying %d bytes - %d total\n", len, sptr->frame_loaded);
}

static bool mpeg2ps_stream_find_mpeg_header (mpeg2ps_stream_t *sptr,
					     bool *have_eof)
{
  uint8_t  *pak = look_bytes(&sptr->m_b, sptr->pes_bytes_left);
  uint32_t ix;

  if (pak == NULL) {
    *have_eof = TRUE;
    return FALSE;
  }
  
  if (check_mpeg_header_with_leftover(sptr, pak)) {
    sptr->left_over_bytes_start = 0;
    mpeg2ps_malloc_frame(sptr, sptr->frame_buffer_len);
    return TRUE;
  }

  for (ix = 0; ix < sptr->pes_bytes_left - 3; ix++, pak++) {
    if (pak[0] == 0 &&
	pak[1] == 0 &&
	pak[2] == 1 &&
	(IS_MPEG_START(pak[3]))) {
      // copy header
      copy_bytes_to_frame(sptr, pak, 4);
      sptr->frame_len = sptr->frame_loaded;
      sptr->pes_bytes_left -= ix + 4;
      adv_bytes(&sptr->m_b, ix + 4);
      sptr->have_mpeg_pict_hdr = (pak[3] == 0);
      return TRUE;
    }
  }

  copy_mpeg_to_leftover(sptr, pak);
  adv_bytes(&sptr->m_b, sptr->pes_bytes_left);
  sptr->pes_bytes_left = 0;
  return FALSE;
}


static bool mpeg2ps_stream_load_mpeg_video (mpeg2ps_stream_t *sptr,
					    bool *have_eof)
{
  uint8_t *pak = look_bytes(&sptr->m_b, sptr->pes_bytes_left);
  uint32_t offset = 0, new_offset;
  uint32_t scode;
  if (pak == NULL) {
    *have_eof = TRUE;
    return FALSE;
  }
  if (check_mpeg_header_with_leftover(sptr, pak)) {
    sptr->frame_len -= sptr->left_over_bytes_cnt;
    sptr->left_over_bytes_start = 0;
    return TRUE;
  }
  
  while (offset < sptr->pes_bytes_left) {
    if (MP4AV_Mpeg3FindNextStart(pak + offset, 
				 sptr->pes_bytes_left - offset, 
				 &new_offset, 
				 &scode) >= 0) {
      if (sptr->have_mpeg_pict_hdr &&
	  (IS_MPEG_START(scode & 0xff) ||
	   scode == MPEG3_SEQUENCE_END_START_CODE)) {
	offset += new_offset;
	copy_bytes_to_frame(sptr, pak, offset);
	sptr->frame_len = sptr->frame_loaded;
	adv_bytes(&sptr->m_b, offset);
	sptr->pes_bytes_left -= offset;
	return TRUE;
      }
      if (scode == MPEG3_PICTURE_START_CODE) {
	sptr->have_mpeg_pict_hdr = TRUE;
      }
    }
    offset += new_offset + 4;  // skip past header
  }
  copy_bytes_to_frame(sptr, pak, sptr->pes_bytes_left);
  sptr->frame_len = sptr->frame_loaded;
  copy_mpeg_to_leftover(sptr, pak + sptr->pes_bytes_left - 3);
  adv_bytes(&sptr->m_b, sptr->pes_bytes_left);
  sptr->pes_bytes_left = 0;
    // found header at offset.
  return FALSE; // not done yet
}
  
static bool mpeg2ps_stream_find_mp3_header (mpeg2ps_stream_t *sptr,
					    uint32_t *frameSize,
					    bool *have_eof)
{
  uint8_t *pak = look_bytes(&sptr->m_b, sptr->pes_bytes_left);
  const uint8_t *ret;

  if (pak == NULL) {
    *have_eof = TRUE;
    return FALSE;
  }
  if (sptr->left_over_bytes_cnt != 0) {
    memcpy(&sptr->left_over_bytes[3],
	   pak,
	   3);
    if (MP4AV_Mp3GetNextFrame(sptr->left_over_bytes,
			      6, 
			      &ret, 
			      frameSize,
			      TRUE, 
			      TRUE) == TRUE) {
      sptr->left_over_bytes_start = ret - sptr->left_over_bytes;
      return TRUE;
    }
    sptr->left_over_bytes_cnt = 0;
  }
  if (MP4AV_Mp3GetNextFrame(pak,
			    sptr->pes_bytes_left,
			    &ret,
			    frameSize,
			    TRUE, 
			    TRUE) == FALSE) {
    adv_bytes(&sptr->m_b, sptr->pes_bytes_left - 3);
    pak = look_bytes(&sptr->m_b, 3);
    memcpy(sptr->left_over_bytes, pak, 3);
    sptr->left_over_bytes_cnt = 3;
    return FALSE;
  }
  if (ret != pak) {
    sptr->pes_bytes_left -= (ret - pak);
    adv_bytes(&sptr->m_b, ret - pak);
  }
  return TRUE;
}
    
static bool mpeg2ps_stream_fill_buffer (mpeg2ps_stream_t *sptr)
{
  uint32_t left = sptr->frame_len - sptr->frame_loaded;
  uint32_t to_copy = MIN(left, sptr->pes_bytes_left);
  uint8_t *pak;
  pak = look_bytes(&sptr->m_b, to_copy);

  memcpy(sptr->frame_buffer + sptr->frame_loaded, 
	 pak, 
	 to_copy);
  sptr->pes_bytes_left -= to_copy;
  sptr->frame_loaded += to_copy;
#ifdef DEBUG_LOC
  printf("loc: "X64" copy %u  %u -left %d\n", lseek(sptr->m_b.m_fd, 0, SEEK_CUR) -
	 sptr->m_b.m_buffer_size + sptr->m_b.m_buffer_on,
	 to_copy, sptr->frame_loaded,
	 sptr->pes_bytes_left);
#endif
  left -= to_copy;
  adv_bytes(&sptr->m_b, to_copy);
  // state change
  if (sptr->pes_bytes_left == 0) {
    sptr->m_state = STREAM_FIND_STREAM_ID;
    return left == 0;
  }
  
  if (left == 0) {
    sptr->m_state = STREAM_FIND_STREAM_HDR;
  }
  return left == 0;
}

 
bool mpeg2ps_stream_read_frame (mpeg2ps_stream_t *sptr,
				const uint8_t **buffer, 
				uint32_t *buflen,
				bool *have_eof)
{
  bool done = FALSE;
  bool ret;

  while (done == FALSE) {
    switch (sptr->m_state) {
    case STREAM_LOOK_START_CODE:
      done = mpeg2ps_stream_find_pes_hdr(sptr, have_eof);
      break;
    case STREAM_FIND_STREAM_ID:
      done = mpeg2ps_stream_find_stream_id(sptr, have_eof);
      break;
    case STREAM_FIND_STREAM_HDR:
      if (sptr->m_stream_id == 0xe0) { // need to expand this
	ret = mpeg2ps_stream_find_mpeg_header(sptr, have_eof);
	if (ret == FALSE) {
	  sptr->m_state = STREAM_FIND_STREAM_ID;
	  break;
	}
	sptr->m_state = STREAM_READ_FRAME;
      } else if (sptr->m_stream_id == 0xc0) {
	ret = mpeg2ps_stream_find_mp3_header(sptr, 
					     &sptr->frame_len,
					     have_eof);
	if (ret) {
	  // have mp3 header
	  mpeg2ps_malloc_frame(sptr, sptr->frame_len);
	  sptr->m_state = STREAM_READ_FRAME;
	} else {
	  sptr->m_state = STREAM_FIND_STREAM_ID;
	}
      }
      break;
    case STREAM_READ_FRAME:
      if (sptr->m_stream_id == 0xe0) {
	ret = mpeg2ps_stream_load_mpeg_video(sptr, have_eof);
	if (*have_eof) {
	  done = TRUE;
	  break;
	}
	if (ret) {
	  // we have a frame
	  *buffer = sptr->frame_buffer;
	  *buflen = sptr->frame_len;
	  sptr->frame_loaded = sptr->frame_len = 0;
	  done = TRUE;
	  sptr->m_state = STREAM_FIND_STREAM_HDR;
	} else {
	  // we've read the entire PES
	  sptr->m_state = STREAM_FIND_STREAM_ID;
	}
      } else {
	ret = mpeg2ps_stream_fill_buffer(sptr);
	if (ret == TRUE) {
	  // have a frame - state change handled
	  *buffer = sptr->frame_buffer;
	  *buflen = sptr->frame_len;
	  sptr->frame_loaded = sptr->frame_len = 0;
	  done = TRUE;
	}
      }
    }
#if 0
    if (sptr->m_state >= 2) {
#endif
      {
#ifdef DEBUG_STATE
    printf("loc: "X64": state %d bytes left %d\n", 
	   lseek(sptr->m_b.m_fd, 0, SEEK_CUR) -
	   sptr->m_b.m_buffer_size + sptr->m_b.m_buffer_on,
	   sptr->m_state, 
	   sptr->pes_bytes_left);
#endif
    }
    if (*have_eof) {
      if (sptr->frame_len != 0 && sptr->frame_len == sptr->frame_loaded) {
	*buffer = sptr->frame_buffer;
	*buflen = sptr->frame_len;
	sptr->frame_len = sptr->frame_loaded = 0;
	return TRUE;
      }
      return FALSE;
    }
  }
  return TRUE;
}
				       
