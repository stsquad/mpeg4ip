
#include "mpeg2_ps.h"
#include "mpeg2ps_private.h"
#include <mp4av.h>
//#define DEBUG_LOC 1
//#define DEBUG_STATE 1

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
					 uint8_t stream_id,
					 uint8_t substream)
{
  mpeg2ps_stream_t *ptr = MALLOC_STRUCTURE(mpeg2ps_stream_t);

  memset(ptr, 0, sizeof(*ptr));

  ptr->m_stream_id = stream_id;
  ptr->m_substream_id = substream;
  ptr->m_fd = fd;
  ptr->pes_buffer = (uint8_t *)malloc(4*4096);
  ptr->pes_buffer_size_max = 4 * 4096;
  return ptr;
}

void mpeg2ps_stream_destroy (mpeg2ps_stream_t *sptr)
{
  free(sptr);
}
static bool file_read_bytes (int fd,
			     uint8_t *buffer, 
			     uint32_t len)
{
  uint32_t readval = read(fd, buffer, len);
  return readval == len;
}

// note: len could be negative.
static void file_skip_bytes (int fd, int32_t len)
{
  lseek(fd, len, SEEK_CUR);
}

static off_t file_location (int fd)
{
  return lseek(fd, 0, SEEK_CUR);
}


/*
 * adv_past_pack_hdr - read the pack header, advance past it
 * we don't do anything with the data
 */
static void adv_past_pack_hdr (int fd, 
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
static bool find_pack_start (int fd, 
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
static bool read_to_next_pes_header (int fd, 
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
    printf("loc: "X64" %x len %u\n", lseek(fd, 0, SEEK_CUR) - 6,
	   local[3],
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
static bool read_pes_header_data (int fd, 
				  uint16_t orig_pes_len,
				  uint16_t *pes_left,
				  mpeg2ps_ts_t *ts)
{
  uint16_t pes_len = orig_pes_len;
  uint8_t local[10];
  uint32_t hdr_len;

  ts->have_pts = FALSE;
  ts->have_dts = FALSE;

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
    pes_len -= 4;
  } else if ((*local & 0xf0) == 0x30) {
    // have mpeg 1 pts and dts
    if (file_read_bytes(fd, local + 1, 9) == FALSE) {
      return FALSE;
    }
    ts->have_pts = TRUE;
    ts->have_dts = TRUE;
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
      hdr_len -= 5;
    } else if ((local[1] & 0xc0) == 0xc0) {
      // pts and dts
      ts->have_pts = TRUE;
      ts->have_dts = TRUE;
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

/*
 * mpeg2ps_stream_read_next_pes_buffer - for the given stream, 
 * go forward in the file until the next PES for the stream is read.  Read
 * the header (pts, dts), and read the data into the pes_buffer pointer
 */
static bool mpeg2ps_stream_read_next_pes_buffer (mpeg2ps_stream_t *sptr, 
						 bool *have_eof)
{
  uint16_t pes_len;
  uint8_t stream_id;
  uint8_t local;

  while (1) {
    // this will read until we find the next pes.  We don't know if the
    // stream matches - this will read over pack headers
    if (read_to_next_pes_header(sptr->m_fd, &stream_id, &pes_len) == FALSE) {
      *have_eof = TRUE;
      return FALSE;
    }

    if (stream_id != sptr->m_stream_id) {
      file_skip_bytes(sptr->m_fd, pes_len);
      continue;
    }

    // advance past header, reading pts
    if (read_pes_header_data(sptr->m_fd, pes_len, &pes_len, &sptr->ts) == FALSE) {
      *have_eof = TRUE;
      return FALSE;
    }

    // If we're looking at a private stream, make sure that the sub-stream
    // matches.
    if (sptr->m_stream_id == 0xbd) {
      // ac3 or pcm
      file_read_bytes(sptr->m_fd, &local, 1);
      pes_len--;
      if (local != sptr->m_substream_id) {
	file_skip_bytes(sptr->m_fd, pes_len);
	continue; // skip to the next one
      }
      pes_len -= 3;
      file_skip_bytes(sptr->m_fd, 3); // 4 bytes - we don't need now...
      // we need more here...
    }
    copy_bytes_to_pes_buffer(sptr, pes_len);
    return TRUE;
  }
  
  return FALSE;
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
mpeg2ps_stream_find_mpeg_video_frame (mpeg2ps_stream_t *sptr,  
				      bool *have_eof,
				      mpeg2ps_ts_t *ts)
{
  uint32_t offset, scode;
  bool have_pict;
  while (MP4AV_Mpeg3FindNextStart(sptr->pes_buffer + sptr->pes_buffer_on, 
				  sptr->pes_buffer_size - sptr->pes_buffer_on,
				  &offset,
				  &scode) < 0 ||
	 (!IS_MPEG_START(scode & 0xff))) {
    if (sptr->pes_buffer_size > 3)
      sptr->pes_buffer_on = sptr->pes_buffer_size - 3;
    else
      sptr->pes_buffer_on = sptr->pes_buffer_size;
    if (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == FALSE) {
      return FALSE;
    }
  }
  sptr->pes_buffer_on += offset;
#if 0
  printf("header %x at %d\n", scode, sptr->pes_buffer_on);
#endif

  *ts = sptr->ts; // set timestamp after searching
  have_pict = (scode == MPEG3_PICTURE_START_CODE);
  uint32_t start = 4 + sptr->pes_buffer_on;
  while (1) {
    
    if (MP4AV_Mpeg3FindNextStart(sptr->pes_buffer + start, 
				 sptr->pes_buffer_size - start,
				 &offset,
				 &scode) < 0) {
      start += sptr->pes_buffer_size - start - 3;
      start -= sptr->pes_buffer_on;
      if (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == FALSE) {
	return FALSE;
      }
      start += sptr->pes_buffer_on;
    } else {
#if 0
      printf("2header %x at %d\n", scode, start);
#endif

      start += offset;
      if (have_pict == FALSE) {
	have_pict = scode == MPEG3_PICTURE_START_CODE;
      } else {
	if (IS_MPEG_START(scode & 0xff) ||
	    scode == MPEG3_SEQUENCE_END_START_CODE) {
	  sptr->frame_len = start - sptr->pes_buffer_on;
	  return TRUE;
	}
      }
      start += 4;
    }
  }
  return FALSE;
}

static bool mpeg2ps_stream_find_ac3_frame (mpeg2ps_stream_t *sptr, 
					   uint32_t *frameSize, 
					   bool *have_eof, 
					   mpeg2ps_ts_t *ts)
{
  const uint8_t *ret;
  uint32_t diff;
  while (MP4AV_Ac3ParseHeader(sptr->pes_buffer + sptr->pes_buffer_on,
			      sptr->pes_buffer_size - sptr->pes_buffer_on,
			       &ret, 
			      NULL,
			      NULL,
			      frameSize, 
			      NULL) <= 0) {
    // don't have frame
    if (sptr->pes_buffer_size > 6) {
      sptr->pes_buffer_on = sptr->pes_buffer_size - 6;
    } else {
      sptr->pes_buffer_on = sptr->pes_buffer_size;
    }
#if 0
    printf("no frame - moving %u of %u\n",
	   sptr->pes_buffer_on, sptr->pes_buffer_size);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == FALSE) {
      return FALSE;
    }
  }
  // have frame.
  *ts = sptr->ts;
  diff = ret - (sptr->pes_buffer + sptr->pes_buffer_on);
  sptr->pes_buffer_on += diff;
  while (sptr->pes_buffer_size - sptr->pes_buffer_on < *frameSize) {
#if 0
    printf("don't have enough - on %u size %u %u %u\n", sptr->pes_buffer_on, 
	   sptr->pes_buffer_size,
	   sptr->pes_buffer_size - sptr->pes_buffer_on, 
	   *frameSize);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}
static bool mpeg2ps_stream_find_mp3_frame (mpeg2ps_stream_t *sptr,
					    uint32_t *frameSize,
					   bool *have_eof,
					   mpeg2ps_ts_t *ts)
{
  const uint8_t *ret;
  uint32_t diff;
  while (MP4AV_Mp3GetNextFrame(sptr->pes_buffer + sptr->pes_buffer_on,
			       sptr->pes_buffer_size - sptr->pes_buffer_on,
			       &ret, 
			       frameSize, 
			       TRUE, 
			       TRUE) == FALSE) {
    // don't have frame
    if (sptr->pes_buffer_size > 3) {
      sptr->pes_buffer_on = sptr->pes_buffer_size - 3;
    } else {
      sptr->pes_buffer_on = sptr->pes_buffer_size;
    }
#if 0
    printf("no frame - moving %u of %u\n",
	   sptr->pes_buffer_on, sptr->pes_buffer_size);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == FALSE) {
      return FALSE;
    }
  }
  // have frame.
  *ts = sptr->ts;
  diff = ret - (sptr->pes_buffer + sptr->pes_buffer_on);
  sptr->pes_buffer_on += diff;
  while (sptr->pes_buffer_size - sptr->pes_buffer_on < *frameSize) {
#if 0
    printf("don't have enough - on %u size %u %u %u\n", sptr->pes_buffer_on, 
	   sptr->pes_buffer_size,
	   sptr->pes_buffer_size - sptr->pes_buffer_on, 
	   *frameSize);
#endif
    if (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}


bool mpeg2ps_stream_read_frame (mpeg2ps_stream_t *sptr,
				const uint8_t **buffer, 
				uint32_t *buflen,
				bool *have_eof,
				mpeg2ps_ts_t *ts)
{
  //  bool done = FALSE;
  //bool ret;
#if 1
  uint32_t frameSize;
  if (sptr->m_stream_id >= 0xe0) {
    if (mpeg2ps_stream_find_mpeg_video_frame(sptr, have_eof, ts)) {
      *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
      *buflen = sptr->frame_len;
      sptr->pes_buffer_on += sptr->frame_len;
      return TRUE;
    }
    return FALSE;
  } else if (sptr->m_stream_id == 0xbd) {
    if (mpeg2ps_stream_find_ac3_frame(sptr, &frameSize, have_eof, ts)) {
      *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
      *buflen = frameSize;
      sptr->pes_buffer_on += frameSize;
      return TRUE;
    }
    return FALSE;
  } else if (mpeg2ps_stream_find_mp3_frame(sptr, &frameSize, have_eof, ts)) {
    *buffer = sptr->pes_buffer + sptr->pes_buffer_on;
    *buflen = frameSize;
    sptr->pes_buffer_on += frameSize;
#if 0
    printf("frame %u buffer on %u of %u\n", frameSize, sptr->pes_buffer_on,
	   sptr->pes_buffer_size);
#endif
    return TRUE;
  }
#else
  while (mpeg2ps_stream_read_next_pes_buffer(sptr, have_eof) == TRUE) {
    sptr->pes_buffer_size = 0;
    sptr->pes_buffer_on = 0;
  }
#endif
  return FALSE;
}

static void get_info_from_frame (mpeg2ps_stream_t *sptr, 
				 const uint8_t *buffer, 
				 uint32_t buflen)
{
  if (sptr->m_stream_id >= 0xe0) {
    if (MP4AV_Mpeg3ParseSeqHdr(buffer, buflen,
			       &sptr->have_mpeg2,
			       &sptr->h,
			       &sptr->w,
			       &sptr->frame_rate,
			       &sptr->bit_rate,
			       NULL) < 0) {
      printf("Can't parse sequence header in first frame - stream %x\n",
	     sptr->m_stream_id);
      sptr->m_stream_id = 0;
      sptr->m_fd = 0;
    }
    printf("stream %x - %u x %u, %g at %g\n",
	   sptr->m_stream_id, sptr->w, sptr->h, sptr->bit_rate,
	   sptr->frame_rate);
    return;
  }

  if (sptr->m_stream_id >= 0xc0) {
    // mpeg audio
    MP4AV_Mp3Header hdr = MP4AV_Mp3HeaderFromBytes(buffer);
    sptr->channels = MP4AV_Mp3GetChannels(hdr);
    sptr->freq = MP4AV_Mp3GetHdrSamplingRate(hdr);
    sptr->samples_per_frame = MP4AV_Mp3GetHdrSamplingWindow(hdr);
    sptr->bitrate = MP4AV_Mp3GetBitRate(hdr);
  } else if (sptr->m_stream_id == 0xbd) {
    if (sptr->m_substream_id >= 0xa0) {
      // PCM - ???
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
      printf("unknown stream id %x %x\n", sptr->m_stream_id,
	     sptr->m_substream_id);
      return;
    }
  } else {
    printf("unknown stream id %x\n", sptr->m_stream_id);
    return;
  }
    
  printf("audio stream %x - freq %u chans %u bitrate %u spf %u\n", 
	 sptr->m_stream_id, sptr->freq, sptr->channels, sptr->bitrate, 
	 sptr->samples_per_frame);
}
static void clear_stream_buffer (mpeg2ps_stream_t *sptr)
{
  sptr->pes_buffer_on = sptr->pes_buffer_size = 0;
  sptr->frame_len = 0;
}

void mpeg2ps_scan_file (int fd)
{
  uint8_t stream_id, stream_ix, substream;
  uint16_t pes_len, pes_left;
  mpeg2ps_ts_t ts;
  off_t loc, end, first_video_loc = 0, first_audio_loc = 0;
  off_t check, orig_check;
  mpeg2ps_t p, *ps;
  mpeg2ps_stream_t *sptr;
  bool valid_stream;
  const uint8_t *buffer;
  uint32_t buflen;
  bool have_eof;

  ps = &p;
  memset(ps, 0, sizeof(*ps));

  end = lseek(fd, 0, SEEK_END);
  orig_check = check = MAX(end / 50, 200 * 1024);
  lseek(fd, 0, SEEK_SET);
  loc = 0;
  while (read_to_next_pes_header(fd, &stream_id, &pes_len) && 
	 loc < check) {
    loc = file_location(fd) - 6;
    pes_left = pes_len;
    valid_stream = FALSE;
    substream = 0;
    if (stream_id == 0xbd) {
      if (read_pes_header_data(fd, pes_len, &pes_left, &ts) == FALSE) {
	return;
      }
      if (file_read_bytes(fd, &substream, 1) == FALSE) {
	return;
      }
      pes_left--; // remove byte we just read
      if ((substream >= 0x80 && substream < 0x90) ||
	  (substream >= 0xa0 && substream < 0xb0)){
	valid_stream = TRUE;
	stream_ix = substream - 0x80;
      }
    } else if (stream_id >= 0xc0 &&
	       stream_id <= 0xef) {
      // audio and video
      stream_ix = stream_id - 0x80;
      valid_stream = TRUE;
    }
    if (valid_stream) {
      if (ps->streams[stream_ix] == NULL) {
	// need to add
	ps->streams[stream_ix] = mpeg2ps_stream_create(0, stream_id,
						       substream);
	ps->streams[stream_ix]->first_pes_loc = loc;
	if (substream == 0) {
	  if (read_pes_header_data(fd, pes_len, &pes_left, &ts) == FALSE) {
	    return;
	  }
	}
	if ((stream_id >= 0xe0 && ts.have_dts == FALSE) ||
	    (ts.have_pts == FALSE && ts.have_dts == FALSE)) {
	  printf("stream_id %x no start dts\n", stream_id);
	  ps->streams[stream_ix]->m_stream_id = 0; // don't use this...
	} else {
	  printf("%d "U64" %d "U64"\n", 
		 ts.have_pts, ts.pts, 
		 ts.have_dts, ts.dts);
	  ps->streams[stream_ix]->start_dts = ts.have_dts ? ts.dts : ts.pts;
	  if (stream_id >= 0xe0) {
	    printf("added video stream %x "X64"\n", stream_id, loc);
	    if (ps->video_cnt == 0) {
	      first_video_loc = loc;
	    }
	    ps->video_cnt++;
	  } else {
	    printf("added audio stream %x %x "X64"\n", stream_id, substream,
		   loc);
	    if (ps->audio_cnt == 0) {
	      first_audio_loc = loc;
	    }
	    ps->audio_cnt++;
	  }
	}
	if (ps->audio_cnt > 0 && ps->video_cnt > 0) {
	  off_t diff;
	  diff = llabs(first_audio_loc - first_video_loc);
	  diff *= 2;
	  diff += first_video_loc;
	  if (diff < check) {
	    check = diff;
	    printf("changing check to "X64"\n", check);
	  }
	}
	  
      }
    }
    file_skip_bytes(fd, pes_left);
  }

  for (stream_ix = 0; stream_ix < 0x70; stream_ix++) {
    sptr = ps->streams[stream_ix];
    if (sptr != NULL && 
	sptr->m_stream_id != 0) {
      if (lseek(fd, sptr->first_pes_loc, SEEK_SET) != sptr->first_pes_loc) {
	printf("error - can't seek to "X64" %s\n", 
	       sptr->first_pes_loc, strerror(errno));
      }
      sptr->m_fd = fd; // for now
      if (mpeg2ps_stream_read_frame(sptr,
				    &buffer, 
				    &buflen,
				    &have_eof, 
				    &ts) == FALSE) {
	printf("Couldn't read frame of stream %x\n",
	       sptr->m_stream_id);
	sptr->m_stream_id = 0;
	sptr->m_fd = 0;
	continue;
      }
      get_info_from_frame(sptr, buffer, buflen);
      clear_stream_buffer(sptr);
      sptr->m_fd = 0;
    }
  }
  printf("to end "X64"\n", end - check);
  lseek(fd, 0 - orig_check, SEEK_END);
  while (read_to_next_pes_header(fd, &stream_id, &pes_len)) {
    loc = file_location(fd) - 6;
    if ((stream_id & 0xe0) == 0xc0 ||
	(stream_id & 0xf0) == 0xe0 ||
	(stream_id == 0xbd)) {
      if (read_pes_header_data(fd, pes_len, &pes_left, &ts) == FALSE) {
	return;
      }
      if (stream_id == 0xbd) {
	if (file_read_bytes(fd, &substream, 1) == FALSE) {
	  return;
	}
	pes_left--; // remove byte we just read
	stream_ix = substream - 0x80;
      } else {
	stream_ix = stream_id - 0x80;
	substream = 0;
      }
	
      if (ts.have_pts || ts.have_dts) {
	ps->streams[stream_ix]->end_dts = ts.have_dts ? ts.dts : ts.pts;
	ps->streams[stream_ix]->end_dts_loc = loc;
      }
#if 1
      printf("loc "X64" stream %x %x", loc, stream_id, substream);
      if (ts.have_pts) printf(" pts "U64, ts.pts);
      if (ts.have_dts) printf(" dts "U64, ts.dts);
      printf("\n");
#endif
      file_skip_bytes(fd, pes_left);
    }
  }

  for (stream_id = 0; stream_id < 0x70; stream_id++) {
    if (ps->streams[stream_id] != NULL && 
	ps->streams[stream_id]->m_stream_id != NULL) {
      printf("stream %x - start pts "U64" end pts "U64"\n", 
	     ps->streams[stream_id]->m_stream_id,
	     ps->streams[stream_id]->start_dts,
	     ps->streams[stream_id]->end_dts);
    }
  }
}

