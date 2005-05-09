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
 * mpeg4_file.cpp
 * Create media structures for session for an mp4v file (raw mpeg4)
 */
#include "mpeg4.h"
#include "typeapi.h"
#include "mode.hpp"
#include "vopses.hpp"
//#include "sys/tps_enhcbuf.hpp"
//#include "sys/decoder/enhcbufdec.hpp"
#include "bitstrm.hpp"
#include "vopsedec.hpp"

#define logit divx->m_vft->log_msg
/*
 * divx_find_header
 * Search buffer to find the next start code
 */
static int divx_find_header (iso_decode_t *divx,
			     uint32_t start_offset)
{
  for (uint32_t ix = start_offset; ix + 4 < divx->m_buffer_size; ix++) {
    if ((divx->m_buffer[ix] == 0) &&
	(divx->m_buffer[ix + 1] == 0) &&
	(divx->m_buffer[ix + 2] == 1)) {
      return ix;
    }
  }
  return -1;
}

/*
 * divx_reset_buffer
 * Move end of buffer to beginning, fill in rest of buffer
 */
static int divx_reset_buffer (iso_decode_t *divx)
{			     
  uint32_t diff, read;

  if (divx->m_buffer_size > divx->m_buffer_on) {
    diff = divx->m_buffer_size - divx->m_buffer_on;
    memmove(divx->m_buffer,
	    &divx->m_buffer[divx->m_buffer_on],
	    diff);
  } else diff = 0;

  divx->m_buffer_size = diff;
  read = fread(divx->m_buffer + diff, 
	       1, 
	       divx->m_buffer_size_max - diff,
	       divx->m_ifile);
  divx->m_buffer_on = 0;
  if (read <= 0) {
    if (divx->m_buffer_size < 4) divx->m_buffer_size = 0;
    return -1;
  }
  divx->m_buffer_size += read;
  if (divx->m_buffer_size < 4) {
    divx->m_buffer_size = 0;
    return -1;
  }
  return diff;
}

/*
 * divx_buffer_load
 * Make sure we have at least 1 full VOP frame in the buffer
 */
static int divx_buffer_load (iso_decode_t *divx, uint8_t *ftype) 
{
  int next_hdr, left, value;

  if (divx->m_buffer_on + 3 >= divx->m_buffer_size) {
    if (divx_reset_buffer(divx) < 0) return -1;
  }

  next_hdr = divx_find_header(divx, divx->m_buffer_on);
  if (next_hdr < 0) return -1;

  // We've got the first header pointed to by m_buffer_on
  divx->m_buffer_on = next_hdr;

  // Is it a VOP header ?  If not, find the first VOP header
  if (divx->m_buffer[next_hdr + 3] != 0xb6) {
    value = 0;
    do {
      // Increment when we've got a header pointed to by next_hdr
      if (value >= 0) next_hdr += 4;

      value = divx_find_header(divx, next_hdr);
      if (value < 0) {
	if (divx->m_buffer_on == 0 &&
	  divx->m_buffer_size == divx->m_buffer_size_max) {
	  // weirdness has happened.  We've got a full buffer of
	  // headers, no frames
	  return -1;
	}
	left = divx_reset_buffer(divx);
	if (left < 0) {
	  // No more new data - we've reached the end... 
	  return divx->m_buffer_size;
	}
	// okay - this case is gross - we'll start checking from the
	// beginning
	next_hdr = left - 4;
      } else {
	next_hdr = value;
      }
    } while (value < 0 || divx->m_buffer[next_hdr + 3] != 0xb6);
  }

  // next_hdr contains the location of the first VOP.
  // Record the file type (top 2 bits) after 00 00 01 b6
  *ftype = divx->m_buffer[next_hdr + 4];

  // Find the next header.
  value = divx_find_header(divx, next_hdr + 4);
  if (value >= 0) {
    // Cool - it's in the header...
    return value;
  }
  //
  // Not in the header - reset the buffer, then continue search
  //
  value = divx->m_buffer_size - divx->m_buffer_on;
  left = divx_reset_buffer(divx);
  if (left < 0) return divx->m_buffer_size;

  value = divx_find_header(divx, value);
  if (value >= 0) {
    return value;
  }

  // We don't have enough read in... Go forward
  while (divx->m_buffer_size_max < 65535) {
    int old, readsize;
    divx->m_buffer = (uint8_t *)realloc(divx->m_buffer,
					divx->m_buffer_size_max + 1024);
    readsize = fread(&divx->m_buffer[divx->m_buffer_size_max],
		     1, 
		     1024, 
		     divx->m_ifile);
    if (readsize <= 0) {
      return (divx->m_buffer_size - divx->m_buffer_on);
    }
    old = divx->m_buffer_size;
    divx->m_buffer_size += readsize;
    divx->m_buffer_size_max += 1024;
    value = divx_find_header(divx, old - 4);
    if (value >= 0) return value;
  }
  return -1;
}

/*
 * divx_file_check
 * Check the file.  If it's a .divx file, look for the VOL, and record
 * where the I frames are.
 */
codec_data_t *mpeg4_iso_file_check (lib_message_func_t message,
				    const char *name, 
				    double *max,
				    char *desc[4],
				    CConfigSet *pConfig)
{
  int len;
  iso_decode_t *iso;
  uint32_t framecount = 0;
  uint64_t calc;

  /*
   * Compare with .divx extension
   */
  len = strlen(name);
  if ((strcasecmp(name + len - 5, ".divx") != 0) &&
      (strcasecmp(name + len - 5, ".xvid") != 0) &&
      (strcasecmp(name + len - 5, ".mp4v") != 0) &&
      (strcasecmp(name + len - 4, ".m4v") != 0) &&
      (strcasecmp(name + len - 4, ".cmp") != 0)) {
    
    message(LOG_DEBUG, "mp4iso", "suffix not correct %s", name);
    return NULL;
  }

  /*
   * Malloc the divx structure, init some variables
   */
  iso = MALLOC_STRUCTURE(iso_decode_t);
  memset(iso, 0, sizeof(*iso));

  iso->m_main_short_video_header = FALSE;
  iso->m_pvodec = new CVideoObjectDecoder();
  iso->m_decodeState = DECODE_STATE_VOL_SEARCH;

  iso->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (iso->m_ifile == NULL) {
    free(iso);
    return NULL;
  }

  iso->m_buffer = (uint8_t *)malloc(16 * 1024);
  iso->m_buffer_size_max = 16 * 1024;
  iso->m_fpos = new CFilePosRecorder();
  iso->m_fpos->record_point(0, 0, 0);

  /*
   * Start searching the file, find the VOL, mark the I frames
   */
  int havevol = 0;
  int nextframe;
  uint8_t ftype;
  nextframe = divx_buffer_load(iso, &ftype);

  do {
    if (havevol == 0) {
      iso->m_pvodec->SetUpBitstreamBuffer((unsigned char *)&iso->m_buffer[iso->m_buffer_on],
					  iso->m_buffer_size - iso->m_buffer_on);
      try {
	iso->m_pvodec->decodeVOLHead();
	havevol = 1;
	iso->m_buffer_on = nextframe;
	iso->m_framerate = iso->m_pvodec->getClockRate();
	message(LOG_DEBUG, "mp4iso", "Found vol in mpeg4 file clock rate %d",
		iso->m_framerate);
      } catch (...) {
	iso->m_buffer_on += iso->m_pvodec->get_used_bytes();
	//iso->m_buffer_on = iso->m_buffer_size - 3;
      }
    } else {
      // If we have an I_VOP, mark it.
      if ((ftype & 0xc0) == 0) {
	calc = framecount * 1000;
	
	calc /= iso->m_framerate;
	//message(LOG_DEBUG, "mp4iso", "I frame at %u "U64, framecount, calc);
	uint64_t offset;
	fpos_t pos;
        if (fgetpos(iso->m_ifile, &pos) > 0) {
	  FPOS_TO_VAR(pos, uint64_t, offset);
	  iso->m_fpos->record_point(offset - 
				    iso->m_buffer_size - 
				    iso->m_buffer_on, 
				    calc, 
				    framecount);
	}
      }
      iso->m_buffer_on = nextframe;
    }
    framecount++;
    nextframe = divx_buffer_load(iso, &ftype);
  } while (nextframe != -1);

  if (havevol == 0) {
    iso_clean_up(iso);
    return NULL;
  }
  if (iso->m_framerate < 0 || iso->m_framerate > 60) iso->m_framerate = 30;
  *max = (double)framecount / (double)iso->m_framerate;
  rewind(iso->m_ifile);
  return ((codec_data_t *)iso);
}

/*
 * divx_file_next_frame
 * Read in the next frame, return timestamp
 */
int divx_file_next_frame (codec_data_t *your_data,
			  uint8_t **buffer, 
			  frame_timestamp_t *ts)
{
  iso_decode_t *divx;
  int next_hdr, value;

  divx = (iso_decode_t *)your_data;

  // start at the next header
  next_hdr = divx_find_header(divx, divx->m_buffer_on);
  if (next_hdr < 0) {
    next_hdr = divx_reset_buffer(divx);
    if (next_hdr < 0) {
      return 0;
    }
    next_hdr = divx_find_header(divx, next_hdr);
    if (next_hdr < 0) {
      return 0;
    }
  }
  divx->m_buffer_on = next_hdr;

  value = 0;
  // find first vop
  while (divx->m_buffer[next_hdr + 3] != 0xb6) {
    value = divx_find_header(divx, next_hdr + 4);
    if (value < 0) {
      value = divx_reset_buffer(divx);
      if (value < 0) return 0;
      next_hdr = divx_find_header(divx, value - 4);
    } else {
      next_hdr = value;
    }
  }

  // find the next start code, so we have 1 frame in buffer
  // Don't have to worry about going past end of buffer, or buffer length
  // not being enough - we've fixed that problem when initially reading
  // in the file.
  value = divx_find_header(divx, next_hdr + 4);
  if (value < 0) {
    divx_reset_buffer(divx);
    value = divx_find_header(divx, 4);
  }

  ts->msec_timestamp = (divx->m_frame_on * TO_U64(1000)) / divx->m_framerate;
  ts->timestamp_is_pts = false;
  *buffer = &divx->m_buffer[divx->m_buffer_on];
  divx->m_frame_on++;
  return divx->m_buffer_size - divx->m_buffer_on;
}

/*
 * divx_file_used_for_frame
 * Tell how many bytes we used
 */
void divx_file_used_for_frame (codec_data_t *your,
			       uint32_t bytes)
{
  iso_decode_t *divx = (iso_decode_t *)your;
  divx->m_buffer_on += bytes;
}

/*
 * divx_file_seek_to - find the closest time to ts, start from 
 * there
 */
int divx_file_seek_to (codec_data_t *your, uint64_t ts)
{
  iso_decode_t *divx = (iso_decode_t *)your;

  const frame_file_pos_t *fpos = divx->m_fpos->find_closest_point(ts);

  divx->m_frame_on = fpos->frames;
  divx->m_buffer_on = 0;
  divx->m_buffer_size = 0;

  fpos_t pos;
  VAR_TO_FPOS(pos, fpos->file_position);
  fsetpos(divx->m_ifile, &pos);
  divx_reset_buffer(divx);
  return 0;
}

/*
 * divx_file_eof
 * see if we've hit the end of the file
 */
int divx_file_eof (codec_data_t *ifptr)
{
  iso_decode_t *divx = (iso_decode_t *)ifptr;

  return divx->m_buffer_on == divx->m_buffer_size && feof(divx->m_ifile);
}


