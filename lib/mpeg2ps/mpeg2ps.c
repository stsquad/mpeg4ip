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
 *		Bill May wmay@cisco.com
 */

/*
 * mpeg2ps.c - parse program stream and vob files
 */
#include "mpeg2_ps.h"
#include "mpeg2ps_private.h"
#include <mp4av.h>
#include <mp4av_h264.h>

//#define DEBUG_LOC 1
//#define DEBUG_STATE 1
static const uint lpcm_freq_tab[4] = {48000, 96000, 44100, 32000};

/*************************************************************************
 * File access routines.  Could all be inlined
 *************************************************************************/
static FDTYPE file_open (const char *name)
{
  return open(name, OPEN_RDONLY);
}

static bool file_okay (FDTYPE fd)
{
  return fd >= 0;
}

static void file_close (FDTYPE fd)
{
  close(fd);
}

static bool file_read_bytes (FDTYPE fd,
			     uint8_t *buffer, 
			     uint32_t len)
{
  uint32_t readval = read(fd, buffer, len);
  return readval == len;
}

// note: len could be negative.
static void file_skip_bytes (FDTYPE fd, int32_t len)
{
  lseek(fd, len, SEEK_CUR);
}

static off_t file_location (FDTYPE fd)
{
  return lseek(fd, 0, SEEK_CUR);
}

static off_t file_seek_to (FDTYPE fd, off_t loc)
{
  return lseek(fd, loc, SEEK_SET);
}

static off_t file_size (FDTYPE fd)
{
  off_t ret = lseek(fd, 0, SEEK_END);
  file_seek_to(fd, 0);
  return ret;
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


static mpeg2ps_stream_t *mpeg2ps_stream_create (uint8_t stream_id,
						uint8_t substream)
{
  mpeg2ps_stream_t *ptr = MALLOC_STRUCTURE(mpeg2ps_stream_t);

  memset(ptr, 0, sizeof(*ptr));

  ptr->m_stream_id = stream_id;
  ptr->m_substream_id = substream;
  ptr->is_video = stream_id >= 0xe0;
  ptr->pes_buffer = (uint8_t *)malloc(4*4096);
  ptr->pes_buffer_size_max = 4 * 4096;
  return ptr;
}

static void mpeg2ps_stream_destroy (mpeg2ps_stream_t *sptr)
{
  mpeg2ps_record_pes_t *p;
  while (sptr->record_first != NULL) {
    p = sptr->record_first;
    sptr->record_first = p->next_rec;
    free(p);
  }
  if (sptr->m_fd != FDNULL) {
    file_close(sptr->m_fd);
    sptr->m_fd = FDNULL;
  }
  CHECK_AND_FREE(sptr->pes_buffer);
  free(sptr);
}


/*
 * adv_past_pack_hdr - read the pack header, advance past it
 * we don't do anything with the data
 */
static void adv_past_pack_hdr (FDTYPE fd, 
			       uint8_t *pak,
			       uint32_t read_from_start)
{
  uint8_t stuffed;
  uint8_t readbyte;
  uint8_t val;
  if (read_from_start < 5) {
    file_skip_bytes(fd, 5 - read_from_start);
    file_read_bytes(fd, &readbyte, 1);
    val = readbyte;
  } else {
    val = pak[4];
  }
    
  // we've read 6 bytes
  if ((val & 0xc0) != 0x40) {
    // mpeg1
    file_skip_bytes(fd, 12 - read_from_start); // skip 6 more bytes
    return;
  }
  file_skip_bytes(fd, 13 - read_from_start);
  file_read_bytes(fd, &readbyte, 1);
  stuffed = readbyte & 0x7;
  file_skip_bytes(fd, stuffed);
}

/*
 * find_pack_start
 * look for the pack start code in the file - read 512 bytes at a time, 
 * searching for that code.
 * Note: we may also be okay looking for >= 00 00 01 bb
 */
static bool find_pack_start (FDTYPE fd, 
			     uint8_t *saved,
			     uint32_t len)
{
  uint8_t buffer[512];
  uint32_t buffer_on = 0, new_offset, scode;
  memcpy(buffer, saved, len);
  if (file_read_bytes(fd, buffer + len, sizeof(buffer) - len) == FALSE) {
    return FALSE;
  }
  while (1) {
    if (MP4AV_Mpeg3FindNextStart(buffer + buffer_on,
				 sizeof(buffer) - buffer_on, 
				 &new_offset, 
				 &scode) >= 0) {
      buffer_on += new_offset;
      if (scode == MPEG2_PS_PACKSTART) {
	file_skip_bytes(fd, buffer_on - 512); // go back to header
	return TRUE;
      }
      buffer_on += 1;
    } else {
      len = 0;
      if (buffer[sizeof(buffer) - 3] == 0 &&
	  buffer[sizeof(buffer) - 2] == 0 &&
	  buffer[sizeof(buffer) - 1] == 1) {
	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 1;
	len = 3;
      } else if (*(uint16_t *)(buffer + sizeof(buffer) - 2) == 0) {
	buffer[0] = 0;
	buffer[1] = 0;
	len = 2;
      } else if (buffer[sizeof(buffer) - 1] == 0) {
	buffer[0] = 0;
	len = 1;
      }
      if (file_read_bytes(fd, buffer + len, sizeof(buffer) - len) == FALSE) {
	return FALSE;
      }
      buffer_on = 0;
    }
  }
  return FALSE;
}

/*
 * copy_bytes_to_pes_buffer - read pes_len bytes into the buffer, 
 * adjusting it if we need it
 */
static void copy_bytes_to_pes_buffer (mpeg2ps_stream_t *sptr, 
			       uint16_t pes_len)
{
  uint32_t to_move;

  if (sptr->pes_buffer_size + pes_len > sptr->pes_buffer_size_max) {
    // if no room in the buffer, we'll move it - otherwise, just fill
    // note - we might want a better strategy about moving the buffer - 
    // right now, we might be moving a number of bytes if we have a large
    // followed by large frame.
    to_move = sptr->pes_buffer_size - sptr->pes_buffer_on;
    memmove(sptr->pes_buffer,
	    sptr->pes_buffer + sptr->pes_buffer_on,
	    to_move);
    sptr->pes_buffer_size = to_move;
    sptr->pes_buffer_on = 0;
    //printf("moving %d bytes\n", to_move);
    if (to_move + pes_len > sptr->pes_buffer_size_max) {
      sptr->pes_buffer = (uint8_t *)realloc(sptr->pes_buffer, 
					    to_move + pes_len + 2048);
      sptr->pes_buffer_size_max = to_move + pes_len + 2048;
    }
  }
  file_read_bytes(sptr->m_fd, sptr->pes_buffer + sptr->pes_buffer_size, pes_len);
  sptr->pes_buffer_size += pes_len;
#if 0
  printf("copying %u bytes - on %u size %u\n",
	 pes_len, sptr->pes_buffer_on, sptr->pes_buffer_size);
#endif
}

/*
 * read_to_next_pes_header - read the file, look for the next valid
 * pes header.  We will skip over PACK headers, but not over any of the
 * headers listed in 13818-1, table 2-18 - basically, anything with the
 * 00 00 01 and the next byte > 0xbb.
 * We return the pes len to read, and the "next byte"
 */
static bool read_to_next_pes_header (FDTYPE fd, 
				     uint8_t *stream_id,
				     uint16_t *pes_len)
{
  uint32_t hdr;
  uint8_t local[6];

  while (1) {
    // read the pes header
    if (file_read_bytes(fd, local, 6) == FALSE) {
      return FALSE;
    }

    hdr = convert32(local);
    // if we're not a 00 00 01, read until we get the next pack start
    // we might want to also read until next PES - look into that.
    if (((hdr & MPEG2_PS_START_MASK) != MPEG2_PS_START) ||
	(hdr < MPEG2_PS_END)) {
      if (find_pack_start(fd, local, 6) == FALSE) {
	return FALSE;
      }
      continue;
    }
    if (hdr == MPEG2_PS_PACKSTART) {
      // pack start code - we can skip down
      adv_past_pack_hdr(fd, local, 6);
      continue;
    }
    if (hdr == MPEG2_PS_END) {
      file_skip_bytes(fd, -2);
      continue;
    }

    // we should have a valid stream and pes_len here...
    *stream_id = hdr & 0xff;
    *pes_len = convert16(local + 4);
#if 0
    printf("loc: "X64" %x %x len %u\n", file_location(fd) - 6,
	   local[3],
	   *stream_id,
	   *pes_len);
#endif
    return TRUE;
  }
  return FALSE;
}

/*
 * read_pes_header_data
 * this should read past the pes header for the audio and video streams
 * it will store the timestamps if it reads them
 */
static bool read_pes_header_data (FDTYPE fd, 
				  uint16_t orig_pes_len,
				  uint16_t *pes_left,
				  bool *have_ts,
				  mpeg2ps_ts_t *ts)
{
  uint16_t pes_len = orig_pes_len;
  uint8_t local[10];
  uint32_t hdr_len;

  ts->have_pts = FALSE;
  ts->have_dts = FALSE;
  *have_ts = false;
  if (file_read_bytes(fd, local, 1) == FALSE) {
    return FALSE;
  }
  pes_len--; // remove this first byte from length
  while (*local == 0xff) {
    if (file_read_bytes(fd, local, 1) == FALSE) {
      return FALSE;
    }
    pes_len--;
    if (pes_len == 0) {
      *pes_left = 0;
      return TRUE;
    }
  }
  if ((*local & 0xc0) == 0x40) { 
    // buffer scale & size
    file_skip_bytes(fd, 1);
    if (file_read_bytes(fd, local, 1) == FALSE) {
      return FALSE;
    }
    pes_len -= 2;
  }

  if ((*local & 0xf0) == 0x20) {
    // mpeg-1 with pts
    if (file_read_bytes(fd, local + 1, 4) == FALSE) {
      return FALSE;
    }
    ts->have_pts = TRUE;
    ts->pts = ts->dts = read_pts(local);
    //printf("mpeg1 pts "U64"\n", ts->pts);
    *have_ts = true;
    pes_len -= 4;
  } else if ((*local & 0xf0) == 0x30) {
    // have mpeg 1 pts and dts
    if (file_read_bytes(fd, local + 1, 9) == FALSE) {
      return FALSE;
    }
    ts->have_pts = TRUE;
    ts->have_dts = TRUE;
    *have_ts = true;
    ts->pts = read_pts(local);
    ts->dts = read_pts(local + 5);
    pes_len -= 9;
  } else if ((*local & 0xc0) == 0x80) {
    // mpeg2 pes header  - we're pointing at the flags field now
    if (file_read_bytes(fd, local + 1, 2) == FALSE) {
      return FALSE;
    }
    hdr_len = local[2];
    pes_len -= hdr_len + 2; // first byte removed already
    if ((local[1] & 0xc0) == 0x80) {
      // just pts
      ts->have_pts = TRUE;
      file_read_bytes(fd, local, 5);
      ts->pts = ts->dts = read_pts(local);
      //printf("pts "U64"\n", ts->pts);
      *have_ts = true;
      hdr_len -= 5;
    } else if ((local[1] & 0xc0) == 0xc0) {
      // pts and dts
      ts->have_pts = TRUE;
      ts->have_dts = TRUE;
      *have_ts = true;
      file_read_bytes(fd, local, 10);
      ts->pts = read_pts(local);
      ts->dts = read_pts(local  + 5);    
      hdr_len -= 10;
    }
    file_skip_bytes(fd, hdr_len);
  } else if (*local != 0xf) {
    file_skip_bytes(fd, pes_len);
    pes_len = 0;
  } 
  *pes_left = pes_len;
  return TRUE;
}

static bool search_for_next_pes_header (mpeg2ps_stream_t *sptr, 
					uint16_t *pes_len,
					bool *have_ts, 
					off_t *found_loc)
{
  uint8_t stream_id;
  uint8_t local;
  off_t loc;
  mpeg2ps_ts_t *ps_ts;
  while (1) {
    // this will read until we find the next pes.  We don't know if the
    // stream matches - this will read over pack headers
    if (read_to_next_pes_header(sptr->m_fd, &stream_id, pes_len) == FALSE) {
      return FALSE;
    }

    if (stream_id != sptr->m_stream_id) {
      file_skip_bytes(sptr->m_fd, *pes_len);
      continue;
    }
    loc = file_location(sptr->m_fd) - 6; 
    // advance past header, reading pts
    // if our existing next_ps has pts stuff, we use the next_next
    // this happens when a frame is small, and at the very end of 
    // a pes, with a frame starting at the next pes.
    ps_ts = (sptr->next_pes_ts.have_pts || sptr->next_pes_ts.have_dts) ?
      &sptr->next_next_pes_ts : &sptr->next_pes_ts;

    if (read_pes_header_data(sptr->m_fd, 
			     *pes_len, 
			     pes_len, 
			     have_ts, 
			     ps_ts) == FALSE) {
      return FALSE;
    }
#if 0
    printf("loc: "X64" have ts %u "U64" %d\n",
	   loc, *have_ts, sptr->next_pes_ts.dts, *pes_len);
#endif
    // If we're looking at a private stream, make sure that the sub-stream
    // matches.
    if (sptr->m_stream_id == 0xbd) {
      // ac3 or pcm
      file_read_bytes(sptr->m_fd, &local, 1);
      *pes_len -= 1;
      if (local != sptr->m_substream_id) {
	file_skip_bytes(sptr->m_fd, *pes_len);
	continue; // skip to the next one
      }
      if (sptr->m_substream_id >= 0xa0) {
	*pes_len -= 6;
	file_read_bytes(sptr->m_fd, sptr->audio_private_stream_info, 6);
#if 0
	printf("reading %x %x %x %x\n", 
	       sptr->audio_private_stream_info[0],
	       sptr->audio_private_stream_info[1],
	       sptr->audio_private_stream_info[2],
	       sptr->audio_private_stream_info[3]);
#endif
      } else {
	*pes_len -= 3;
	file_skip_bytes(sptr->m_fd, 3); // 4 bytes - we don't need now...
      }
      // we need more here...
    }
    if (*have_ts) {
      mpeg2ps_record_pts(sptr, loc, ps_ts);
    }
    if (found_loc != NULL) *found_loc = loc;
    return true;
  }
  return false;
}

/*
 * mpeg2ps_stream_read_next_pes_buffer - for the given stream, 
 * go forward in the file until the next PES for the stream is read.  Read
 * the header (pts, dts), and read the data into the pes_buffer pointer
 */
static bool mpeg2ps_stream_read_next_pes_buffer (mpeg2ps_stream_t *sptr)
{
  uint16_t pes_len;
  bool have_ts;

  if (search_for_next_pes_header(sptr, &pes_len, &have_ts, NULL) == false) {
    return false;
  }

  copy_bytes_to_pes_buffer(sptr, pes_len);

  return TRUE;
}


static void copy_next_pes_ts_to_frame_ts (mpeg2ps_stream_t *sptr)
{
  sptr->frame_ts = sptr->next_pes_ts;
#if 1
  sptr->next_pes_ts = sptr->next_next_pes_ts;
  sptr->next_next_pes_ts.have_pts = 
    sptr->next_next_pes_ts.have_dts = false;
#else
  sptr->next_pes_ts.have_pts = sptr->next_pes_ts.have_dts = false;
#endif
}

/***************************************************************************
 * Frame reading routine.  For each stream, the fd's should be different.
 * we will read from the pes stream, and save it in the stream's pes buffer.
 * This will give us raw data that we can search through for frame headers, 
 * and the like.  We shouldn't read more than we need - when we need to read, 
 * we'll put the whole next pes buffer in the buffer
 *
 * Audio routines are of the format:
 *   look for header
 *   determine length
 *   make sure length is in buffer
 *
 * Video routines
 *   look for start header (GOP, SEQ, Picture)
 *   look for pict header
 *   look for next start (END, GOP, SEQ, Picture)
 *   
 ***************************************************************************/
#define IS_MPEG_START(a) ((a) == 0xb3 || (a) == 0x00 || (a) == 0xb8)

static bool 
mpeg2ps_stream_find_mpeg_video_frame (mpeg2ps_stream_t *sptr)
{
  uint32_t offset, scode;
  bool have_pict;
  bool started_new_pes = false;
  uint32_t start;
  /*
   * First thing - determine if we have enough bytes to read the header.
   * if we do, we have the correct timestamp.  If not, we read the new
   * pes, so we'd want to use the timestamp we read.
   */
  sptr->frame_ts = sptr->next_pes_ts; 
#if 0
  printf("frame read %u "U64"\n", sptr->frame_ts.have_dts, sptr->frame_ts.dts);
#endif
  if (sptr->pes_buffer_size <= sptr->pes_buffer_on + 4) {
    if (sptr->pes_buffer_size != sptr->pes_buffer_on)
      started_new_pes = true;
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  while (MP4AV_Mpeg3FindNextStart(sptr->pes_buffer + sptr->pes_buffer_on, 
				  sptr->pes_buffer_size - sptr->pes_buffer_on,
				  &offset,
				  &scode) < 0 ||
	 (!IS_MPEG_START(scode & 0xff))) {
    if (sptr->pes_buffer_size > 3)
      sptr->pes_buffer_on = sptr->pes_buffer_size - 3;
    else {
      sptr->pes_buffer_on = sptr->pes_buffer_size;
      started_new_pes = true;
    }
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  sptr->pes_buffer_on += offset;
  if (offset == 0 && started_new_pes) {
    // nothing...  we've copied the timestamp already.
  } else {
    // we found the new start, but we pulled in a new pes header before
    // starting.  So, we want to use the header that we read.
    copy_next_pes_ts_to_frame_ts(sptr);
    //printf("  %u "U64"\n", sptr->frame_ts.have_dts, sptr->frame_ts.dts);
  }
#if 0
  printf("header %x at %d\n", scode, sptr->pes_buffer_on);
#endif

  if (scode == MPEG3_PICTURE_START_CODE) {
    sptr->pict_header_offset = sptr->pes_buffer_on;
    have_pict = true;
  } else have_pict = false;

  start = 4 + sptr->pes_buffer_on;
  while (1) {
    
    if (MP4AV_Mpeg3FindNextStart(sptr->pes_buffer + start, 
				 sptr->pes_buffer_size - start,
				 &offset,
				 &scode) < 0) {
      start = sptr->pes_buffer_size - 3;
      start -= sptr->pes_buffer_on;
      sptr->pict_header_offset -= sptr->pes_buffer_on;
      if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
	return FALSE;
      }
      start += sptr->pes_buffer_on;
      sptr->pict_header_offset += sptr->pes_buffer_on;
    } else {
#if 0
      printf("2header %x at %d\n", scode, start);
#endif

      start += offset;
      if (have_pict == FALSE) {
	if (scode == MPEG3_PICTURE_START_CODE) {
	  have_pict = true;
	  sptr->pict_header_offset = start;
	}
      } else {
	if (IS_MPEG_START(scode & 0xff) ||
	    scode == MPEG3_SEQUENCE_END_START_CODE) {
	  sptr->frame_len = start - sptr->pes_buffer_on;
	  sptr->have_frame_loaded = true;
	  return TRUE;
	}
      }
      start += 4;
    }
  }
  return FALSE;
}

static bool 
mpeg2ps_stream_find_h264_video_frame (mpeg2ps_stream_t *sptr)
{
  uint32_t offset;
  bool have_pict;
  bool started_new_pes = false;
  uint32_t start;
  uint8_t nal_type;
  /*
   * First thing - determine if we have enough bytes to read the header.
   * if we do, we have the correct timestamp.  If not, we read the new
   * pes, so we'd want to use the timestamp we read.
   */
  sptr->frame_ts = sptr->next_pes_ts; 
  if (sptr->pes_buffer_size <= sptr->pes_buffer_on + 4) {
    if (sptr->pes_buffer_size != sptr->pes_buffer_on)
      started_new_pes = true;
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  if (h264_is_start_code(sptr->pes_buffer + sptr->pes_buffer_on) == false) {
    do {
      uint32_t offset = h264_find_next_start_code(sptr->pes_buffer +
						  sptr->pes_buffer_on, 
						  sptr->pes_buffer_size -
						  sptr->pes_buffer_on);
      if (offset == 0) {
	if (sptr->pes_buffer_size > 3)
	  sptr->pes_buffer_on = sptr->pes_buffer_size - 3;
	else {
	  sptr->pes_buffer_on = sptr->pes_buffer_size;
	  started_new_pes = true;
	}
	if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
	  return FALSE;
	}
      } else
	sptr->pes_buffer_on += offset;
    } while (h264_is_start_code(sptr->pes_buffer + sptr->pes_buffer_on) == false);
  }

  if (started_new_pes) {
    // nothing...  we've copied the timestamp already.
  } else {
    // we found the new start, but we pulled in a new pes header before
    // starting.  So, we want to use the header that we read.
    copy_next_pes_ts_to_frame_ts(sptr);
  }
#if 0
  printf("header %x at %d\n", scode, sptr->pes_buffer_on);
#endif

  have_pict = false;

  start = 4 + sptr->pes_buffer_on;
  while (1) {
    
    if ((offset = h264_find_next_start_code(sptr->pes_buffer + start, 
					    sptr->pes_buffer_size - start))
	== 0) {
      start = sptr->pes_buffer_size - 4;
      start -= sptr->pes_buffer_on;
      sptr->pict_header_offset -= sptr->pes_buffer_on;
      if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
	return FALSE;
      }
      start += sptr->pes_buffer_on;
      sptr->pict_header_offset += sptr->pes_buffer_on;
    } else {
#if 0
      printf("2header %x at %d\n", scode, start);
#endif

      start += offset;
      nal_type = h264_nal_unit_type(sptr->pes_buffer + start);
      if (have_pict == FALSE) {
	if (h264_nal_unit_type_is_slice(nal_type)) {
	  have_pict = true;
	  sptr->pict_header_offset = start;
	}
      } else {
	if (H264_NAL_TYPE_ACCESS_UNIT == nal_type) {
	  sptr->frame_len = start - sptr->pes_buffer_on;
	  sptr->have_frame_loaded = true;
	  return TRUE;
	}
      }
      start += 4;
    }
  }
  return FALSE;
}

static bool 
mpeg2ps_stream_figure_out_video_type (mpeg2ps_stream_t *sptr) 
{
  bool started_new_pes = false;

  /*
   * First thing - determine if we have enough bytes to read the header.
   * if we do, we have the correct timestamp.  If not, we read the new
   * pes, so we'd want to use the timestamp we read.
   */
  sptr->frame_ts = sptr->next_pes_ts; 
  if (sptr->pes_buffer_size <= sptr->pes_buffer_on + 5) {
    if (sptr->pes_buffer_size != sptr->pes_buffer_on)
      started_new_pes = true;
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }

  if (h264_is_start_code(sptr->pes_buffer + sptr->pes_buffer_on) &&
      h264_nal_unit_type(sptr->pes_buffer + sptr->pes_buffer_on) == 
      H264_NAL_TYPE_ACCESS_UNIT) {
    sptr->have_h264 = true;
    sptr->determined_type = true;
    return mpeg2ps_stream_find_h264_video_frame(sptr);
  } 
  // figure it's mpeg2
  sptr->have_h264 = false;
  sptr->determined_type = true;
  return mpeg2ps_stream_find_mpeg_video_frame(sptr);
}

static bool mpeg2ps_stream_find_lpcm_frame (mpeg2ps_stream_t *sptr)
{
  uint8_t frames;
  uint32_t this_pes_byte_count;

  if (sptr->pes_buffer_size == sptr->pes_buffer_on) {
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == false) {
      return false;
    }
  }
  if (sptr->channels == 0) {
    // means we haven't read anything yet - we need to read for
    // frequency and channels.
    sptr->channels = 1 + (sptr->audio_private_stream_info[LPCM_INFO] & 0x7);
    sptr->freq = lpcm_freq_tab[(sptr->audio_private_stream_info[LPCM_INFO] >> 4) & 7];
  }

  copy_next_pes_ts_to_frame_ts(sptr);
  if (sptr->lpcm_read_offset) {
    // we need to read bytes - 4 bytes.  This should only occur when 
    // we seek.  Otherwise, we've already reall read the bytes when
    // we read the last pes
    uint32_t bytes_to_skip;
    bytes_to_skip = 
      ntohs(*(uint16_t *)&sptr->audio_private_stream_info[LPCM_PES_OFFSET_MSB]);
    bytes_to_skip -= 4;

    while (bytes_to_skip > sptr->pes_buffer_size - sptr->pes_buffer_on) {
      if (mpeg2ps_stream_read_next_pes_buffer(sptr) == false) {
	return FALSE;
      }
    } 
    sptr->pes_buffer_on += bytes_to_skip;
    sptr->lpcm_read_offset = false;
  }

  // calculate the number of bytes in this LPCM frame.  There are
  // 150 PTS ticks per LPCM frame.  We will read all the PCM frames
  // referenced by the header.
  frames = sptr->audio_private_stream_info[LPCM_FRAME_COUNT];
  this_pes_byte_count = frames;
  this_pes_byte_count *= 150;
  this_pes_byte_count *= sptr->freq;
  this_pes_byte_count *= sptr->channels * sizeof(int16_t);
  this_pes_byte_count /= 90000;
  //printf("fcount %u bytes %u\n", frames, this_pes_byte_count);

  sptr->frame_len = this_pes_byte_count;

  while (sptr->pes_buffer_size - sptr->pes_buffer_on < sptr->frame_len) {
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  sptr->have_frame_loaded = true;
#if 0
  printf("lpcm size %u %u %u %u\n", sptr->pes_buffer_size - sptr->pes_buffer_on, 
	 sptr->frame_len,
	 sptr->frame_ts.have_dts, sptr->frame_ts.have_pts);
#endif
  return TRUE;
}

static bool mpeg2ps_stream_find_ac3_frame (mpeg2ps_stream_t *sptr)
{
  const uint8_t *ret;
  uint32_t diff;
  bool started_new_pes = false;
  sptr->frame_ts = sptr->next_pes_ts; // set timestamp after searching
  if (sptr->pes_buffer_size <= sptr->pes_buffer_on + 6) {
    if (sptr->pes_buffer_size != sptr->pes_buffer_on)
      started_new_pes = true;
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  while (MP4AV_Ac3ParseHeader(sptr->pes_buffer + sptr->pes_buffer_on,
			      sptr->pes_buffer_size - sptr->pes_buffer_on,
			       &ret, 
			      NULL,
			      NULL,
			      &sptr->frame_len, 
			      NULL) <= 0) {
    // don't have frame
    if (sptr->pes_buffer_size > 6) {
      sptr->pes_buffer_on = sptr->pes_buffer_size - 6;
      started_new_pes = true;
    } else {
      sptr->pes_buffer_on = sptr->pes_buffer_size;
    }
#if 0
    printf("no frame - moving %u of %u\n",
	   sptr->pes_buffer_on, sptr->pes_buffer_size);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  // have frame.
  diff = ret - (sptr->pes_buffer + sptr->pes_buffer_on);
  sptr->pes_buffer_on += diff;
  if (diff == 0 && started_new_pes) {
    // we might have a new PTS - but it's not here
  } else {
    copy_next_pes_ts_to_frame_ts(sptr);
  }
  while (sptr->pes_buffer_size - sptr->pes_buffer_on < sptr->frame_len) {
#if 0
    printf("don't have enough - on %u size %u %u %u\n", sptr->pes_buffer_on, 
	   sptr->pes_buffer_size,
	   sptr->pes_buffer_size - sptr->pes_buffer_on, 
	   sptr->frame_len);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  sptr->have_frame_loaded = true;
  return TRUE;
}

static bool mpeg2ps_stream_find_mp3_frame (mpeg2ps_stream_t *sptr)
{
  const uint8_t *ret;
  uint32_t diff;
  bool started_new_pes = false;

  sptr->frame_ts = sptr->next_pes_ts;
  if (sptr->pes_buffer_size <= sptr->pes_buffer_on + 4) {
    if (sptr->pes_buffer_size != sptr->pes_buffer_on)
      started_new_pes = true;
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  while (MP4AV_Mp3GetNextFrame(sptr->pes_buffer + sptr->pes_buffer_on,
			       sptr->pes_buffer_size - sptr->pes_buffer_on,
			       &ret, 
			       &sptr->frame_len, 
			       TRUE, 
			       TRUE) == FALSE) {
    // don't have frame
    if (sptr->pes_buffer_size > 3) {
      if (sptr->pes_buffer_on != sptr->pes_buffer_size) {
	sptr->pes_buffer_on = sptr->pes_buffer_size - 3;
      }
      started_new_pes = true; // we have left over bytes...
    } else {
      sptr->pes_buffer_on = sptr->pes_buffer_size;
    }
#if 0
    printf("no frame - moving %u of %u\n",
	   sptr->pes_buffer_on, sptr->pes_buffer_size);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  // have frame.
  diff = ret - (sptr->pes_buffer + sptr->pes_buffer_on);
  sptr->pes_buffer_on += diff;
  if (diff == 0 && started_new_pes) {

  } else {
    copy_next_pes_ts_to_frame_ts(sptr);
  }
  while (sptr->pes_buffer_size - sptr->pes_buffer_on < sptr->frame_len) {
#if 0
    printf("don't have enough - on %u size %u %u %u\n", sptr->pes_buffer_on, 
	   sptr->pes_buffer_size,
	   sptr->pes_buffer_size - sptr->pes_buffer_on, 
	   sptr->frame_len);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr) == FALSE) {
      return FALSE;
    }
  }
  sptr->have_frame_loaded = true;
  return TRUE;
}

/*
 * mpeg2ps_stream_read_frame.  read the correct frame based on stream type.
 * advance_pointers is false when we want to use the data
 */
static bool mpeg2ps_stream_read_frame (mpeg2ps_stream_t *sptr,
				       uint8_t **buffer, 
				       uint32_t *buflen,
				       bool advance_pointers)
{
  //  bool done = FALSE;
  if (sptr->is_video) {
    if (sptr->determined_type == false) {
      if (mpeg2ps_stream_figure_out_video_type(sptr)) {
	*buffer = sptr->pes_buffer + sptr->pes_buffer_on;
	*buflen = sptr->frame_len;
	if (advance_pointers) {
	  sptr->pes_buffer_on += sptr->frame_len;
	}
	return TRUE;
      }
      return FALSE;
    } else {
      if (sptr->have_h264) {
	if (mpeg2ps_stream_find_h264_video_frame(sptr)) {
	  *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
	  *buflen = sptr->frame_len;
	  if (advance_pointers) {
	    sptr->pes_buffer_on += sptr->frame_len;
	  }
	  return TRUE;
	}
      } else {
	if (mpeg2ps_stream_find_mpeg_video_frame(sptr)) {
	  *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
	  *buflen = sptr->frame_len;
	  if (advance_pointers) {
	    sptr->pes_buffer_on += sptr->frame_len;
	  }
	  return TRUE;
	}
      }
    return FALSE;
    }
  } else if (sptr->m_stream_id == 0xbd) {
    // would need to handle LPCM here
    if (sptr->m_substream_id >= 0xa0) {
      if (mpeg2ps_stream_find_lpcm_frame(sptr)) {
	*buffer = sptr->pes_buffer + sptr->pes_buffer_on;
	*buflen = sptr->frame_len;
	if (advance_pointers) {
	  sptr->pes_buffer_on += sptr->frame_len;
	}
	return TRUE;
      }
      return FALSE;
    }
    if (mpeg2ps_stream_find_ac3_frame(sptr)) {
      *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
      *buflen = sptr->frame_len;
      if (advance_pointers)
	sptr->pes_buffer_on += sptr->frame_len;
      return TRUE;
    }
    return FALSE;
  } else if (mpeg2ps_stream_find_mp3_frame(sptr)) {
    *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
    *buflen = sptr->frame_len;
    if (advance_pointers)
      sptr->pes_buffer_on += sptr->frame_len;
    return TRUE;
  }
  return FALSE;
}

/*
 * get_info_from_frame - we have a frame, get the info from it.
 */
static void get_info_from_frame (mpeg2ps_stream_t *sptr, 
				 uint8_t *buffer, 
				 uint32_t buflen)
{
  if (sptr->is_video) {
    if (sptr->have_h264) {
      bool found_seq = false;
      do {
	if (h264_nal_unit_type(buffer) == H264_NAL_TYPE_SEQ_PARAM) {
	  h264_decode_t dec;
	  found_seq = true;
	  if (h264_read_seq_info(buffer, buflen, &dec) >= 0) {
	    sptr->video_profile = dec.profile;
	    sptr->video_level = dec.level;
	    sptr->w = dec.pic_width;
	    sptr->h = dec.pic_height;
	  }
	} else {
	  uint32_t offset = h264_find_next_start_code(buffer, buflen);
	  if (offset == 0) buflen = 0;
	  else {
	    buffer += offset;
	    buflen -= offset;
	  }
	}
      } while (found_seq == false && buflen > 0);
      mpeg2ps_message(LOG_ERR, "need to info h264");
      sptr->ticks_per_frame = 90000 / 30;
      
    } else {
      if (MP4AV_Mpeg3ParseSeqHdr(buffer, buflen,
				 &sptr->have_mpeg2,
				 &sptr->h,
				 &sptr->w,
				 &sptr->frame_rate,
				 &sptr->bit_rate,
				 NULL,
				 &sptr->video_profile) < 0) {
	mpeg2ps_message(LOG_ERR, "Can't parse sequence header in first frame - stream\n",
			sptr->m_stream_id);
	sptr->m_stream_id = 0;
	sptr->m_fd = FDNULL;
      }
      
      sptr->ticks_per_frame = (uint64_t)(90000.0 / sptr->frame_rate);
      mpeg2ps_message(LOG_INFO,"stream %x - %u x %u, %g at %g "U64,
		      sptr->m_stream_id, sptr->w, sptr->h, sptr->bit_rate,
		      sptr->frame_rate, sptr->ticks_per_frame);
    }
    return;
  }

  if (sptr->m_stream_id >= 0xc0) {
    // mpeg audio
    MP4AV_Mp3Header hdr = MP4AV_Mp3HeaderFromBytes(buffer);
    sptr->channels = MP4AV_Mp3GetChannels(hdr);
    sptr->freq = MP4AV_Mp3GetHdrSamplingRate(hdr);
    sptr->samples_per_frame = MP4AV_Mp3GetHdrSamplingWindow(hdr);
    sptr->bitrate = MP4AV_Mp3GetBitRate(hdr) * 1000; // give bps, not kbps
    sptr->layer = MP4AV_Mp3GetHdrLayer(hdr);
    sptr->version = MP4AV_Mp3GetHdrVersion(hdr);
  } else if (sptr->m_stream_id == 0xbd) {
    if (sptr->m_substream_id >= 0xa0) {
      // PCM - ???
      sptr->samples_per_frame = 0;
      sptr->bitrate = sptr->freq * sptr->channels * sizeof(int16_t);
    } else if (sptr->m_substream_id >= 0x80) {
      // ac3
      const uint8_t *temp;
      MP4AV_Ac3ParseHeader(buffer, buflen, &temp,
			   &sptr->bitrate,
			   &sptr->freq,
			   NULL,
			   &sptr->channels);
      sptr->samples_per_frame = 256 * 6;
    } else {
      mpeg2ps_message(LOG_ERR, "unknown audio private stream id %x %x", 
		      sptr->m_stream_id,
		      sptr->m_substream_id);
      return;
    }
  } else {
    mpeg2ps_message(LOG_ERR, "unknown stream id %x", sptr->m_stream_id);
    return;
  }
    
  mpeg2ps_message(LOG_INFO, "audio stream %x - freq %u chans %u bitrate %u spf %u", 
		sptr->m_stream_id, sptr->freq, sptr->channels, sptr->bitrate, 
		sptr->samples_per_frame);
}

/*
 * clear_stream_buffer - called when we seek to clear out any data in 
 * the buffers
 */
static void clear_stream_buffer (mpeg2ps_stream_t *sptr)
{
  sptr->pes_buffer_on = sptr->pes_buffer_size = 0;
  sptr->frame_len = 0;
  sptr->have_frame_loaded = false;
  sptr->next_pes_ts.have_dts = sptr->next_pes_ts.have_pts = false;
  sptr->frame_ts.have_dts = sptr->frame_ts.have_pts = false;
  sptr->lpcm_read_offset = true;
}

/*
 * convert_to_msec - convert ts (at 90000) to msec, based on base_ts and
 * frames_since_last_ts.
 */
static uint64_t convert_ts (mpeg2ps_stream_t *sptr,
			    mpeg2ps_ts_type_t ts_type,
			    uint64_t ts,
			    uint64_t base_ts,
			    uint32_t frames_since_ts)
{
  uint64_t ret, calc;
  ret = ts - base_ts;
  if (sptr->is_video) {
    // video
    ret += frames_since_ts * sptr->ticks_per_frame;
  } else {
    // audio
    if (frames_since_ts != 0 && sptr->freq != 0) {
      calc = (frames_since_ts * 90000 * sptr->samples_per_frame) / sptr->freq;
      ret += calc;
    }
  }
  if (ts_type == TS_MSEC)
    ret /= TO_U64(90); // * 1000 / 90000
#if 0
  printf("stream %x - ts "U64" base "U64" frames since %d ret "U64"\n",
	 sptr->m_stream_id, ts, base_ts, frames_since_ts, ret);
#endif
  return ret;
}

/*
 * find_stream_from_id - given the stream, get the sptr.
 * only used in inital set up, really.  APIs use index into 
 * video_streams and audio_streams arrays.
 */
static mpeg2ps_stream_t *find_stream_from_id (mpeg2ps_t *ps, 
					      uint8_t stream_id, 
					      uint8_t substream)
{
  uint8_t ix;
  if (stream_id >= 0xe0) {
    for (ix = 0; ix < ps->video_cnt; ix++) {
      if (ps->video_streams[ix]->m_stream_id == stream_id) {
	return ps->video_streams[ix];
      }
    }
  } else {
    for (ix = 0; ix < ps->audio_cnt; ix++) {
      if (ps->audio_streams[ix]->m_stream_id == stream_id &&
	  (stream_id != 0xbd ||
	   substream == ps->audio_streams[ix]->m_substream_id)) {
	return ps->audio_streams[ix];
      }
    }
  }
  return NULL;
}

/*
 * add_stream - add a new stream
 */
static bool add_stream (mpeg2ps_t *ps,
			uint8_t stream_id,
			uint8_t substream,
			off_t first_loc,
			mpeg2ps_ts_t *ts)
{
  mpeg2ps_stream_t *sptr;
  
  sptr = find_stream_from_id(ps, stream_id, substream);
  if (sptr != NULL) return FALSE;

  // need to add

  sptr = mpeg2ps_stream_create(stream_id, substream);
  sptr->first_pes_loc = first_loc;
  if (ts == NULL ||
      (ts->have_dts == false && ts->have_pts == false)) {
    sptr->first_pes_has_dts = false;
    mpeg2ps_message(LOG_CRIT, "stream %x doesn't have start pts",
		    sptr->m_stream_id);
  } else {
    sptr->start_dts = ts->have_dts ? ts->dts : ts->pts;
    sptr->first_pes_has_dts = true;
  }
  if (sptr->is_video) {
    // can't be more than 16 - e0 to ef...
    ps->video_streams[ps->video_cnt] = sptr;
    mpeg2ps_message(LOG_DEBUG, 
		    "added video stream %x "X64" "U64, 
		    stream_id, first_loc, sptr->start_dts);
    ps->video_cnt++;
  } else {
    if (ps->audio_cnt >= 32) {
      mpeg2ps_stream_destroy(sptr);
      return FALSE;
    }
    mpeg2ps_message(LOG_DEBUG, 
		    "added audio stream %x %x "X64" "U64, 
		    stream_id, substream, first_loc, sptr->start_dts);
    ps->audio_streams[ps->audio_cnt] = sptr;
    ps->audio_cnt++;
  }
  return TRUE;
}

static void check_fd_for_stream (mpeg2ps_t *ps, 
			  mpeg2ps_stream_t *sptr)
{
  if (sptr->m_fd != FDNULL) return;

  sptr->m_fd = file_open(ps->filename);
}

/*
 * advance_frame - when we're reading frames, this indicates that we're
 * done.  We will call this when we read a frame, but not when we
 * seek.  It allows us to leave the last frame we're seeking in the
 * buffer
 */
static void advance_frame (mpeg2ps_stream_t *sptr)
{
  sptr->pes_buffer_on += sptr->frame_len;
  sptr->have_frame_loaded = false;
  if (sptr->frame_ts.have_dts || sptr->frame_ts.have_pts) {
    if (sptr->frame_ts.have_dts) 
      sptr->last_ts = sptr->frame_ts.dts;
    else
      sptr->last_ts = sptr->frame_ts.pts;
    sptr->frames_since_last_ts = 0;
  } else {
    sptr->frames_since_last_ts++;
  }
}
/*
 * get_info_for_all_streams - loop through found streams - read an
 * figure out the info
 */
static void get_info_for_all_streams (mpeg2ps_t *ps)
{
  uint8_t stream_ix, max_ix, av;
  mpeg2ps_stream_t *sptr;
  uint8_t *buffer;
  uint32_t buflen;

  // av will be 0 for video streams, 1 for audio streams
  // av is just so I don't have to dup a lot of code that does the
  // same thing.
  for (av = 0; av < 2; av++) {
    if (av == 0) max_ix = ps->video_cnt;
    else max_ix = ps->audio_cnt;
    for (stream_ix = 0; stream_ix < max_ix; stream_ix++) {
      if (av == 0) sptr = ps->video_streams[stream_ix];
      else sptr = ps->audio_streams[stream_ix];

      if (file_seek_to(ps->fd, sptr->first_pes_loc) != sptr->first_pes_loc) {
	mpeg2ps_message(LOG_ERR, "stream %x error - can't seek to "X64" %s", 
			sptr->m_stream_id, sptr->first_pes_loc, 
			strerror(errno));
      }
      // we don't open a seperate file descriptor yet (only when they
      // start reading or seeking).  Use the one from the ps.
      sptr->m_fd = ps->fd; // for now
      clear_stream_buffer(sptr);
      if (mpeg2ps_stream_read_frame(sptr,
				    &buffer, 
				    &buflen,
				    false) == FALSE) {
	mpeg2ps_message(LOG_CRIT, "Couldn't read frame of stream %x",
			sptr->m_stream_id);
	sptr->m_stream_id = 0;
	sptr->m_fd = FDNULL;
	continue;
      }
      get_info_from_frame(sptr, buffer, buflen);
      //printf("got stream av %d %d\n", av, sptr->first_pes_has_dts);
      // here - if (sptr->first_pes_has_dts == false) should be processed
      if (sptr->first_pes_has_dts == false) {
	uint32_t frames_from_beg = 0;
	bool have_frame;
	do {
	  advance_frame(sptr);
	  have_frame = 
	    mpeg2ps_stream_read_frame(sptr, &buffer, &buflen, false);
	  frames_from_beg++;
	} while (have_frame && 
		 sptr->frame_ts.have_dts == false && 
		 sptr->frame_ts.have_pts == false && 
		 frames_from_beg < 1000);
	if (have_frame == false ||
	    (sptr->frame_ts.have_dts == false &&
	     sptr->frame_ts.have_pts == false)) {
	  mpeg2ps_message(LOG_ERR, 
			  "can't find initial pts of stream %x - have_frame %d cnt %u",
			  sptr->m_stream_id, have_frame, frames_from_beg);
	} else {
	  sptr->start_dts = sptr->frame_ts.have_dts ? sptr->frame_ts.dts : 
	    sptr->frame_ts.pts;
	  if (sptr->is_video) {
	    sptr->start_dts -= frames_from_beg * sptr->ticks_per_frame;
	  } else {
	    uint64_t conv;
	    conv = sptr->samples_per_frame * 90000;
	    conv /= (uint64_t)sptr->freq;
	    sptr->start_dts -= conv;
	  }
	  mpeg2ps_message(LOG_DEBUG, "stream %x - calc start pts of "U64,
			  sptr->m_stream_id, sptr->start_dts);
	}
      }
      clear_stream_buffer(sptr);
      sptr->m_fd = FDNULL;
    }
  }
}

/*
 * mpeg2ps_scan_file - read file, grabbing all the information that
 * we can out of it (what streams exist, timing, etc).
 */
static void mpeg2ps_scan_file (mpeg2ps_t *ps)
{
  uint8_t stream_id, stream_ix, substream, av_ix, max_cnt;
  uint16_t pes_len, pes_left;
  mpeg2ps_ts_t ts;
  off_t loc, first_video_loc = 0, first_audio_loc = 0;
  off_t check, orig_check;
  mpeg2ps_stream_t *sptr;
  bool valid_stream;
  uint8_t *buffer;
  uint32_t buflen;
  bool have_ts;

  ps->end_loc = file_size(ps->fd);
  orig_check = check = MAX(ps->end_loc / 50, 200 * 1024);

  /*
   * This part reads and finds the streams.  We check up until we
   * find audio and video plus a little, with a max of either 200K or
   * the file size / 50
   */
  loc = 0;
  while (read_to_next_pes_header(ps->fd, &stream_id, &pes_len) && 
	 loc < check) {
    pes_left = pes_len;
    if (stream_id >= 0xbd && stream_id < 0xf0) {
      loc = file_location(ps->fd) - 6;
      if (read_pes_header_data(ps->fd, 
			       pes_len, 
			       &pes_left, 
			       &have_ts, 
			       &ts) == FALSE) {
	return;
      }
      valid_stream = FALSE;
      substream = 0;
      if (stream_id == 0xbd) {
	if (file_read_bytes(ps->fd, &substream, 1) == FALSE) {
	  return;
	}
	pes_left--; // remove byte we just read
	if ((substream >= 0x80 && substream < 0x90) ||
	    (substream >= 0xa0 && substream < 0xb0)){
	  valid_stream = TRUE;
	}
      } else if (stream_id >= 0xc0 &&
		 stream_id <= 0xef) {
	// audio and video
	valid_stream = TRUE;
      }
#if 0
      mpeg2ps_message(LOG_DEBUG, 
		      "stream %x %x loc "X64" pts %d dts %d\n",
		      stream_id, substream, loc, ts.have_pts, ts.have_dts);
#endif
      if (valid_stream) {
	if (add_stream(ps, stream_id, substream, loc, &ts)) {
	  // added
	  if (stream_id >= 0xe0) {
	    if (ps->video_cnt == 1) {
	      first_video_loc = loc;
	    }
	  } else if (ps->audio_cnt == 1) {
	    first_audio_loc = loc;
	  }
	  if (ps->audio_cnt > 0 && ps->video_cnt > 0) {
	    off_t diff;
	    if (first_audio_loc > first_video_loc) 
	      diff = first_audio_loc - first_video_loc;
	    else 
	      diff = first_video_loc - first_audio_loc;
	    diff *= 2;
	    diff += first_video_loc;
	    if (diff < check) {
	      check = diff;
	    }
	  }
	}
      }
    }
    file_skip_bytes(ps->fd, pes_left);
  }
  if (ps->video_cnt == 0 && ps->audio_cnt == 0) {
    return;
  }
  /*
   * Now, we go to close to the end, and try to find the last 
   * dts that we can
   */
  //  printf("to end "X64"\n", end - orig_check);
  file_seek_to(ps->fd, ps->end_loc - orig_check);

  while (read_to_next_pes_header(ps->fd, &stream_id, &pes_len)) {
    loc = file_location(ps->fd) - 6;
    if (stream_id == 0xbd || (stream_id >= 0xc0 && stream_id < 0xf0)) {
      if (read_pes_header_data(ps->fd, 
			       pes_len, 
			       &pes_left, 
			       &have_ts, 
			       &ts) == FALSE) {
	return;
      }
      if (stream_id == 0xbd) {
	if (file_read_bytes(ps->fd, &substream, 1) == FALSE) {
	  return;
	}
	pes_left--; // remove byte we just read
	if (!((substream >= 0x80 && substream < 0x90) ||
	      (substream >= 0xa0 && substream < 0xb0))) {
	  file_skip_bytes(ps->fd, pes_left);
	  continue;
	}
      } else {
	substream = 0;
      }
      sptr = find_stream_from_id(ps, stream_id, substream);
      if (sptr == NULL) {
	mpeg2ps_message(LOG_INFO, 
			"adding stream from end search %x %x",
			stream_id, substream);
	add_stream(ps, stream_id, substream, 0, NULL);
	sptr = find_stream_from_id(ps, stream_id, substream);
      }
      if (sptr != NULL && have_ts) {
	sptr->end_dts = ts.have_dts ? ts.dts : ts.pts;
	sptr->end_dts_loc = loc;
      }
#if 0
      printf("loc "X64" stream %x %x", loc, stream_id, substream);
      if (ts.have_pts) printf(" pts "U64, ts.pts);
      if (ts.have_dts) printf(" dts "U64, ts.dts);
      printf("\n");
#endif
      file_skip_bytes(ps->fd, pes_left);
    }
  }

  /*
   * Now, get the info for all streams, so we can use it again
   * we could do this before the above, I suppose
   */
  get_info_for_all_streams(ps);

  ps->first_dts = MAX_UINT64;

  /*
   * we need to find the earliest start pts - we use that to calc
   * the rest of the timing, so we're 0 based.
   */
  for (av_ix = 0; av_ix < 2; av_ix++) {
    if (av_ix == 0) max_cnt = ps->video_cnt;
    else max_cnt = ps->audio_cnt;

    for (stream_ix = 0; stream_ix < max_cnt; stream_ix++) {
      sptr = av_ix == 0 ? ps->video_streams[stream_ix] :
	ps->audio_streams[stream_ix];
      if (sptr != NULL && sptr->start_dts < ps->first_dts) {
	ps->first_dts = sptr->start_dts;
      }
    }
  }

  mpeg2ps_message(LOG_INFO, "start ps is "U64, ps->first_dts);
  /*
   * Now, for each thread, we'll start at the last pts location, and
   * read the number of frames.  This will give us a max time
   */
  for (av_ix = 0; av_ix < 2; av_ix++) {
    if (av_ix == 0) max_cnt = ps->video_cnt;
    else max_cnt = ps->audio_cnt;
    for (stream_ix = 0; stream_ix < max_cnt; stream_ix++) {
      uint32_t frame_cnt_since_last;
	  sptr = av_ix == 0 ? ps->video_streams[stream_ix] :
	ps->audio_streams[stream_ix];
      
      // pick up here - find the final time...
      if (sptr->end_dts_loc != 0) {
	//printf("end loc "U64"\n", sptr->end_dts_loc);
	file_seek_to(ps->fd, sptr->end_dts_loc);
	sptr->m_fd = ps->fd;
	frame_cnt_since_last = 0;
	clear_stream_buffer(sptr);
	while (mpeg2ps_stream_read_frame(sptr,
					 &buffer, 
					 &buflen,
					 true)) {
	  //printf("loc "U64"\n", file_location(sptr->m_fd)); 
	  frame_cnt_since_last++;
	}
	sptr->m_fd = FDNULL;
	clear_stream_buffer(sptr);
	mpeg2ps_message(LOG_DEBUG, "stream %x last ts "U64" since last %u\n", 
			sptr->m_stream_id, 
			sptr->end_dts,
			frame_cnt_since_last);
	ps->max_time = MAX(ps->max_time, 
			   convert_ts(sptr, 
				      TS_MSEC,
				      sptr->end_dts,
				      ps->first_dts, 
				      frame_cnt_since_last));
      } else {
	mpeg2ps_message(LOG_DEBUG, "stream %x no end dts", sptr->m_stream_id);
      }
    }
  }

  ps->max_dts = (ps->max_time * 90) + ps->first_dts;
  mpeg2ps_message(LOG_DEBUG, "max time is "U64, ps->max_time);
  file_seek_to(ps->fd, 0);
}

/*************************************************************************
 * API routines
 *************************************************************************/
uint64_t mpeg2ps_get_max_time_msec (mpeg2ps_t *ps) 
{
  return ps->max_time;
}

uint32_t mpeg2ps_get_video_stream_count (mpeg2ps_t *ps)
{
  return ps->video_cnt;
}

// routine to check stream number passed.
static bool invalid_video_streamno (mpeg2ps_t *ps, uint streamno)
{
  if (streamno >= NUM_ELEMENTS_IN_ARRAY(ps->video_streams)) return true;
  if (ps->video_streams[streamno] == NULL) return true;
  return false;
}

char *mpeg2ps_get_video_stream_name (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return NULL;
  }
  if (ps->video_streams[streamno]->have_h264) {
    return h264_get_profile_level_string(ps->video_streams[streamno]->video_profile,
					 ps->video_streams[streamno]->video_level);
  }
  if (ps->video_streams[streamno]->have_mpeg2) {
    return strdup(mpeg2_type(ps->video_streams[streamno]->video_profile));
  }
  return strdup("Mpeg-1");
}

mpeg2ps_video_type_t mpeg2ps_get_video_stream_type (mpeg2ps_t *ps, 
						    uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return MPEG_AUDIO_UNKNOWN;
  }
  if (ps->video_streams[streamno]->have_h264) return MPEG_VIDEO_H264;

  return ps->video_streams[streamno]->have_mpeg2 ? MPEG_VIDEO_MPEG2 : 
    MPEG_VIDEO_MPEG1;
}

uint32_t mpeg2ps_get_video_stream_width (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return 0;
  }
  return ps->video_streams[streamno]->w;
}

uint32_t mpeg2ps_get_video_stream_height (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return 0;
  }
  return ps->video_streams[streamno]->h;
}

double mpeg2ps_get_video_stream_bitrate (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return 0;
  }
  return ps->video_streams[streamno]->bit_rate;
}

double mpeg2ps_get_video_stream_framerate (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return 0;
  }
  return ps->video_streams[streamno]->frame_rate;
}

uint8_t mpeg2ps_get_video_stream_mp4_type (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_video_streamno(ps, streamno)) {
    return 0;
  }
  if (ps->video_streams[streamno]->have_h264) {
    return MP4_PRIVATE_VIDEO_TYPE;
  }
  if (ps->video_streams[streamno]->have_mpeg2) {
    return mpeg2_profile_to_mp4_track_type(ps->video_streams[streamno]->video_profile);
  } 
  return MP4_MPEG1_VIDEO_TYPE;
}

static bool invalid_audio_streamno (mpeg2ps_t *ps, uint streamno)
{
  if (streamno >= NUM_ELEMENTS_IN_ARRAY(ps->audio_streams)) return true;
  if (ps->audio_streams[streamno] == NULL) return true;
  return false;
}

uint32_t mpeg2ps_get_audio_stream_count (mpeg2ps_t *ps)
{
  return ps->audio_cnt;
}

const char *mpeg2ps_get_audio_stream_name (mpeg2ps_t *ps, 
					   uint streamno)
{
  if (invalid_audio_streamno(ps, streamno)) {
    return "none";
  }
  if (ps->audio_streams[streamno]->m_stream_id >= 0xc0) {
    switch (ps->audio_streams[streamno]->version) {
    case 3:
      switch (ps->audio_streams[streamno]->layer) {
      case 3: return "MP1 layer 1";
      case 2: return "MP1 layer 2";
      case 1: return "MP1 layer 3";
      }
      break;
    case 2:
      switch (ps->audio_streams[streamno]->layer) {
      case 3: return "MP2 layer 1";
      case 2: return "MP2 layer 2";
      case 1: return "MP2 layer 3";
      }
      break;
    case 0:
      switch (ps->audio_streams[streamno]->layer) {
      case 3: return "MP2.5 layer 1";
      case 2: return "MP2.5 layer 2";
      case 1: return "MP2.5 layer 3";
      }
      break;
      break;
    }
    return "unknown mpeg layer";
  }
  if (ps->audio_streams[streamno]->m_substream_id >= 0x80 &&
      ps->audio_streams[streamno]->m_substream_id < 0x90)
    return "AC3";

  return "LPCM";
}

mpeg2ps_audio_type_t mpeg2ps_get_audio_stream_type (mpeg2ps_t *ps, 
						    uint streamno)
{
  if (invalid_audio_streamno(ps, streamno)) {
    return MPEG_AUDIO_UNKNOWN;
  }
  if (ps->audio_streams[streamno]->m_stream_id >= 0xc0) {
    return MPEG_AUDIO_MPEG;
  }
  if (ps->audio_streams[streamno]->m_substream_id >= 0x80 &&
      ps->audio_streams[streamno]->m_substream_id < 0x90)
    return MPEG_AUDIO_AC3;

  return MPEG_AUDIO_LPCM;
}

uint32_t mpeg2ps_get_audio_stream_sample_freq (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_audio_streamno(ps, streamno)) {
    return 0;
  }
  return ps->audio_streams[streamno]->freq;
}

uint32_t mpeg2ps_get_audio_stream_channels (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_audio_streamno(ps, streamno)) {
    return 0;
  }
  return ps->audio_streams[streamno]->channels;
}

uint32_t mpeg2ps_get_audio_stream_bitrate (mpeg2ps_t *ps, uint streamno)
{
  if (invalid_audio_streamno(ps, streamno)) {
    return 0;
  }
  return ps->audio_streams[streamno]->bitrate;
}

mpeg2ps_t *mpeg2ps_init (const char *filename)
{
  mpeg2ps_t *ps = MALLOC_STRUCTURE(mpeg2ps_t);
#if 0
  uint8_t local[4];
  uint32_t hdr;
#endif
  if (ps == NULL) {
    return NULL;
  }
  memset(ps, 0, sizeof(*ps));
  ps->fd = file_open(filename);
  if (file_okay(ps->fd) == false) {
    free(ps);
    return NULL;
  }
  
#if 0
  file_read_bytes(ps->fd, local, 4);
  hdr = convert32(local);
  // this should accept all pes headers with 0xba or greater - so, 
  // we'll handle pack streams, or pes streams.
  if (((hdr & MPEG2_PS_START_MASK) != MPEG2_PS_START) ||
      (hdr < MPEG2_PS_PACKSTART)) {
    free(ps);
    return NULL;
  }
#endif
  ps->filename = strdup(filename);
  mpeg2ps_scan_file(ps);
  if (ps->video_cnt == 0 && ps->audio_cnt == 0) {
    mpeg2ps_close(ps);
    return NULL;
  }
  return ps;
}

void mpeg2ps_close (mpeg2ps_t *ps)
{
  uint ix;
  if (ps == NULL) return;
  for (ix = 0; ix < ps->video_cnt; ix++) {
    mpeg2ps_stream_destroy(ps->video_streams[ix]);
    ps->video_streams[ix] = NULL;
  }
  for (ix = 0; ix < ps->audio_cnt; ix++) {
    mpeg2ps_stream_destroy(ps->audio_streams[ix]);
    ps->audio_streams[ix] = NULL;
  }

  CHECK_AND_FREE(ps->filename);

  if (ps->fd != FDNULL) file_close(ps->fd);

  free(ps);
}

/*
 * check_fd_for_stream will make sure we have a fd for the stream we're
 * trying to read - we use a different fd for each stream
 */

/*
 * stream_convert_frame_ts_to_msec - given a "read" frame, we'll
 * calculate the msec and freq timestamps.  This can be called more
 * than 1 time, if needed, without changing any variables, such as
 * frames_since_last_ts, which gets updated in advance_frame
 */
static uint64_t stream_convert_frame_ts_to_msec (mpeg2ps_stream_t *sptr, 
						 mpeg2ps_ts_type_t ts_type,
						 uint64_t base_dts,
						 uint32_t *freq_ts)
{
  uint64_t calc_ts;
  uint frames_since_last = 0;
  uint64_t freq_conv;

  calc_ts = sptr->last_ts;
  if (sptr->frame_ts.have_dts) calc_ts = sptr->frame_ts.dts;
  else if (sptr->frame_ts.have_pts) calc_ts = sptr->frame_ts.dts;
  else frames_since_last = sptr->frames_since_last_ts + 1;

  if (freq_ts != NULL) {
#if 0
    printf("base dts "U64" "U64" %d %u %u\n", base_dts, calc_ts, frames_since_last,
	   sptr->samples_per_frame, sptr->freq);
#endif
    freq_conv = calc_ts - base_dts;
    freq_conv *= sptr->freq;
    freq_conv /= 90000;
    freq_conv += frames_since_last * sptr->samples_per_frame;
    *freq_ts = freq_conv & 0xffffffff;
  }
  return convert_ts(sptr, ts_type, calc_ts, base_dts, frames_since_last);
}

/*
 * mpeg2ps_get_video_frame - gets the next frame
 */    
bool mpeg2ps_get_video_frame(mpeg2ps_t *ps, uint streamno,
			     uint8_t **buffer, 
			     uint32_t *buflen,
			     uint8_t *frame_type,
			     mpeg2ps_ts_type_t ts_type,
			     uint64_t *timestamp)
{
  mpeg2ps_stream_t *sptr;
  if (invalid_video_streamno(ps, streamno)) return false;

  sptr = ps->video_streams[streamno];
  check_fd_for_stream(ps, sptr);

  if (sptr->have_frame_loaded == false) {
    // if we don't have the frame in the buffer (like after a seek), 
    // read the next frame
    if (mpeg2ps_stream_read_frame(sptr, buffer, buflen, false) == false) {
      return false;
    }
  } else {
    *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
    *buflen = sptr->frame_len;
  }
  // determine frame type
  if (frame_type != NULL) {
    *frame_type = MP4AV_Mpeg3PictHdrType(sptr->pes_buffer + 
					 sptr->pict_header_offset);
  }

  // and the timestamp
  if (timestamp != NULL) {
    *timestamp = stream_convert_frame_ts_to_msec(sptr, ts_type, 
						 ps->first_dts, NULL);
  }

  // finally, indicate that we read this frame - get ready for the next one.
  advance_frame(sptr);

  return true;
}


// see above comments      
bool mpeg2ps_get_audio_frame(mpeg2ps_t *ps, uint streamno,
			     uint8_t **buffer, 
			     uint32_t *buflen,
			     mpeg2ps_ts_type_t ts_type,
			     uint32_t *freq_timestamp,
			     uint64_t *timestamp)
{
  mpeg2ps_stream_t *sptr;
  uint64_t ts;
  if (invalid_audio_streamno(ps, streamno)) return false;

  sptr = ps->audio_streams[streamno];
  check_fd_for_stream(ps, sptr);

  if (sptr->have_frame_loaded == false) {
    if (mpeg2ps_stream_read_frame(sptr, buffer, buflen, false) == false) 
      return false;
  } else {
    *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
    *buflen = sptr->frame_len;
  }
  
  if (timestamp != NULL || freq_timestamp != NULL) {
    ts = stream_convert_frame_ts_to_msec(sptr, 
					 ts_type,
					 ps->first_dts,
					 freq_timestamp);
    if (timestamp != NULL) {
      *timestamp = ts;
    }
  }

  advance_frame(sptr);
  return true;
}

/***************************************************************************
 * seek routines
 ***************************************************************************/
/*
 * mpeg2ps_binary_seek - look for a pts that's close to the one that
 * we're looking for.  We have a start ts and location, an end ts and
 * location, and what we're looking for
 */
static void mpeg2ps_binary_seek (mpeg2ps_t *ps, 
				 mpeg2ps_stream_t *sptr, 
				 uint64_t search_dts,
				 uint64_t start_dts,
				 off_t start_loc,
				 uint64_t end_dts, 
				 off_t end_loc)
{
  uint64_t dts_perc;
  off_t loc;
  uint16_t pes_len;
  bool have_ts = false;
  off_t found_loc;
  uint64_t found_dts;

  mpeg2ps_message(LOG_DEBUG, "bin search "U64, search_dts);
  while (1) {
    /*
     * It's not a binary search as much as using a percentage between
     * the start and end dts to start.  We subtract off a bit, so we
     * approach from the beginning of the file - we're more likely to 
     * hit a pts that way
     */
    dts_perc = (search_dts - start_dts) * 1000 / (end_dts - start_dts);
    dts_perc -= dts_perc % 10;

    loc = ((end_loc - start_loc) * dts_perc) / 1000;
  
    if (loc == start_loc || loc == end_loc) return;

    clear_stream_buffer(sptr);
    file_seek_to(sptr->m_fd, start_loc + loc);
    mpeg2ps_message(LOG_DEBUG, "start dl "U64" "U64" end dl "U64" "U64" new "U64,
		    start_dts, start_loc, 
		    end_dts, end_loc, 
		    loc);
    // we'll look for the next pes header for this stream that has a ts.
    do {
      if (search_for_next_pes_header(sptr, 
				     &pes_len, 
				     &have_ts, 
				     &found_loc) == false) {
	return;
      }
      if (have_ts == false) {
	file_skip_bytes(sptr->m_fd, pes_len);
      }
    } while (have_ts == false);

    // record that spot...
    mpeg2ps_record_pts(sptr, found_loc, &sptr->next_pes_ts);

    found_dts = sptr->next_pes_ts.have_dts ? 
      sptr->next_pes_ts.dts : sptr->next_pes_ts.pts;
    mpeg2ps_message(LOG_DEBUG, "found dts "U64" loc "U64,
		    found_dts, loc);
    /*
     * Now, if we're before the search ts, and within 5 seconds, 
     * we'll say we're close enough
     */
    if (found_dts + (5 * 90000) > search_dts &&
	found_dts < search_dts) {
      file_seek_to(sptr->m_fd, found_loc);
      return; // found it - we can seek from here
    }
    /*
     * otherwise, move the head or the tail (most likely the head).
     */
    if (found_dts > search_dts) {
      if (found_dts >= end_dts) {
	file_seek_to(sptr->m_fd, found_loc);
	return;
      }
      end_loc = found_loc;
      end_dts = found_dts;
    } else {
      if (found_dts <= start_dts) {
	file_seek_to(sptr->m_fd, found_loc);
	return;
      }
      start_loc = found_loc;
      start_dts = found_dts;
    }
  }
}

/*
 * mpeg2ps_seek_frame - seek to the next timestamp after the search timestamp
 * First, find a close DTS (usually minus 5 seconds or closer), then
 * read frames until we get the frame after the timestamp.
 */
static bool mpeg2ps_seek_frame (mpeg2ps_t *ps, 
				mpeg2ps_stream_t *sptr, 
				uint64_t search_msec_timestamp)
{
  uint64_t dts;
  mpeg2ps_record_pes_t *rec;
  uint64_t msec_ts;
  uint8_t *buffer;
  uint32_t buflen;

  check_fd_for_stream(ps, sptr);
  clear_stream_buffer(sptr);

  if (search_msec_timestamp <= 1000) { // first second, start from begin...
    file_seek_to(sptr->m_fd, sptr->first_pes_loc);
    return true;
  }
  dts = search_msec_timestamp * 90; // 1000 timescale to 90000 timescale
  dts += ps->first_dts;
  mpeg2ps_message(LOG_DEBUG, "%x seek msec "U64" dts "U64, 
		  sptr->m_stream_id, search_msec_timestamp, dts);
  /*
   * see if the recorded data has anything close
   */
  rec = search_for_ts(sptr, dts);
  if (rec != NULL) {
    // see if it is close
    mpeg2ps_message(LOG_DEBUG, "found rec dts "U64" loc "U64,
		    rec->dts, rec->location);
    // if we're plus or minus a second, seek to that.
    if (rec->dts + 90000 >= dts && rec->dts <= dts + 90000) {
      file_seek_to(sptr->m_fd, rec->location);
      return true;
    }
    // at this point, rec is > a distance.  If within 5 or so seconds, 
    // skip
    if (rec->dts > dts) {
      mpeg2ps_message(LOG_ERR, "stream %x seek frame error dts "U64" rec "U64, 
		      sptr->m_stream_id, dts, rec->dts);
      return false;
    }
    if (rec->dts + (5 * 90000) < dts) {
      // more than 5 seconds away - skip and search
      if (rec->next_rec == NULL) {
	mpeg2ps_binary_seek(ps, sptr, dts, 
			    rec->dts, rec->location,
			    sptr->end_dts, sptr->end_dts_loc);
      } else {
	mpeg2ps_binary_seek(ps, sptr, dts, 
			    rec->dts, rec->location,
			    rec->next_rec->dts, rec->next_rec->location);
      }
    }
    // otherwise, frame by frame search...
  } else {
    // we weren't able to find anything from the recording
    mpeg2ps_binary_seek(ps, sptr, dts, 
			sptr->start_dts, sptr->first_pes_loc,
			sptr->end_dts, sptr->end_dts_loc);
  }
  /*
   * Now, the fun part - read frames until we're just past the time
   */
  clear_stream_buffer(sptr); // clear out any data, so we can read it
  do {
    if (mpeg2ps_stream_read_frame(sptr, &buffer, &buflen, false) == false) 
      return false;

    msec_ts = stream_convert_frame_ts_to_msec(sptr, TS_MSEC, 
					      ps->first_dts, NULL);
    mpeg2ps_message(LOG_DEBUG, "%x read ts "U64, sptr->m_stream_id, msec_ts);
    if (msec_ts < search_msec_timestamp) {
      // only advance the frame if we're not greater than the timestamp
      advance_frame(sptr);
    }
  } while (msec_ts < search_msec_timestamp);

  return true;
}
		      
/*
 * mpeg2ps_seek_video_frame - seek to the location that we're interested
 * in, then scroll up to the next I frame
 */
bool mpeg2ps_seek_video_frame (mpeg2ps_t *ps, uint streamno,
			       uint64_t msec_timestamp)
{
  //  off_t closest_pes;
  uint8_t frame_type;
  mpeg2ps_stream_t *sptr;
  uint8_t *buffer;
  uint32_t buflen;
  uint64_t msec_ts;

  if (invalid_video_streamno(ps, streamno)) return false;

  sptr = ps->video_streams[streamno];
  if (mpeg2ps_seek_frame(ps, 
			 sptr,
			 msec_timestamp)
			  == false) return false;
  
  if (sptr->have_frame_loaded == false) {
    mpeg2ps_message(LOG_CRIT, "no frame loaded after search");
    return false;
  }
  /*
   * read forward until we find the next I frame
   */
  if (sptr->have_h264) {
    
    while (h264_access_unit_is_sync(sptr->pes_buffer + 
				    sptr->pict_header_offset,
				    sptr->pes_buffer_size - 
				    sptr->pict_header_offset) == false) {
      advance_frame(sptr);
      if (mpeg2ps_stream_read_frame(sptr, &buffer, &buflen, false) == false)
	return false;
      msec_ts = stream_convert_frame_ts_to_msec(sptr, TS_MSEC, ps->first_dts, NULL);
      mpeg2ps_message(LOG_DEBUG, "read ts "U64, msec_ts);
    }
  } else {
    frame_type = MP4AV_Mpeg3PictHdrType(sptr->pes_buffer + 
					sptr->pict_header_offset);
    while (frame_type != 1) {
      advance_frame(sptr);
      if (mpeg2ps_stream_read_frame(sptr, &buffer, &buflen, false) == false) 
	return false;
      frame_type = MP4AV_Mpeg3PictHdrType(sptr->pes_buffer + 
					  sptr->pict_header_offset);
      msec_ts = stream_convert_frame_ts_to_msec(sptr, TS_MSEC, ps->first_dts, NULL);
      mpeg2ps_message(LOG_DEBUG, "read ts "U64" type %d", msec_ts, frame_type);
    }
  }
  return true;
}
/*
 * mpeg2ps_seek_audio_frame - go to the closest audio frame after the
 * timestamp
 */
bool mpeg2ps_seek_audio_frame (mpeg2ps_t *ps,
			       uint streamno,
			       uint64_t msec_timestamp)
{
  //  off_t closest_pes;
  mpeg2ps_stream_t *sptr;
  
  if (invalid_audio_streamno(ps, streamno)) return false;
  
  sptr = ps->audio_streams[streamno];
  if (mpeg2ps_seek_frame(ps,
			 sptr,
			 msec_timestamp) == false) return false;
  
  return true;
}
