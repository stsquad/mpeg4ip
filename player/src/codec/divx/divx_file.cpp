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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * divx_file.cpp
 * Read a raw file for divx.
 */
#include "divx.h"
#include <divxif.h>

/*
 * divx_find_header
 * Search buffer to find the next start code
 */
static int divx_find_header (divx_codec_t *divx,
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
static int divx_reset_buffer (divx_codec_t *divx)
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
  if (read <= 0) return -1;
  divx->m_buffer_size += read;
  return diff;
}

/*
 * divx_buffer_load
 * Make sure we have at least 1 full VOP frame in the buffer
 */
static int divx_buffer_load (divx_codec_t *divx, unsigned char *ftype) 
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
    divx->m_buffer = (unsigned char *)realloc(divx->m_buffer,
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
codec_data_t *divx_file_check (lib_message_func_t message,
			       const char *name, 
			       double *max,
			       char *desc[4])
{
  int len;
  divx_codec_t *divx;
  uint32_t framecount = 0;
  uint64_t calc;

  /*
   * Compare with .divx extension
   */
  len = strlen(name);
  if (strcasecmp(name + len - 5, ".divx") != 0) {
    return NULL;
  }

  /*
   * Malloc the divx structure, init some variables
   */
  divx = MALLOC_STRUCTURE(divx_codec_t);
  memset(divx, 0, sizeof(*divx));

  newdec_init();
  divx->m_decodeState = DIVX_STATE_VO_SEARCH;
  divx->m_last_time = 0;

  divx->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (divx->m_ifile == NULL) {
    free(divx);
    return NULL;
  }

  divx->m_buffer = (unsigned char *)malloc(16 * 1024);
  divx->m_buffer_size_max = 16 * 1024;
  divx->m_fpos = new CFilePosRecorder();
  divx->m_fpos->record_point(0, 0, 0);

  /*
   * Start searching the file, find the VOL, mark the I frames
   */
  int havevol = 0;
  int nextframe;
  unsigned char ftype;
  nextframe = divx_buffer_load(divx, &ftype);

  do {
    if (havevol == 0) {
      int ret = newdec_read_volvop(&divx->m_buffer[divx->m_buffer_on], 
				   divx->m_buffer_size - divx->m_buffer_on);
      if (ret == 1) {
	havevol = 1;
	if (mp4_hdr.fps < 0 || mp4_hdr.fps > 100) mp4_hdr.fps = 30;
	message(LOG_DEBUG, "divx", "Found vol in divx file");
	divx->m_buffer_on = nextframe;
      } else
	divx->m_buffer_on = divx->m_buffer_size - 3;
    } else {
      // If we have an I_VOP, mark it.
      if ((ftype & 0xc0) == 0) {
	calc = framecount * 1000;
	
	calc /= mp4_hdr.fps;
	//message(LOG_DEBUG, "divx", "I frame at %u "LLU, framecount, calc);
	divx->m_fpos->record_point(ftell(divx->m_ifile) - 
				   divx->m_buffer_size - 
				   divx->m_buffer_on, 
				   calc, 
				   framecount);
      }
      divx->m_buffer_on = nextframe;
    }
    framecount++;
    nextframe = divx_buffer_load(divx, &ftype);
  } while (nextframe != -1);

  if (havevol == 0) {
    divx_clean_up(divx);
    return NULL;
  }
  *max = (double)framecount / (double)mp4_hdr.fps;
  rewind(divx->m_ifile);
  return ((codec_data_t *)divx);
    
}

/*
 * divx_file_next_frame
 * Read in the next frame, return timestamp
 */
int divx_file_next_frame (codec_data_t *your_data,
			  unsigned char **buffer, 
			  uint64_t *ts)
{
  divx_codec_t *divx;
  int next_hdr, value;

  divx = (divx_codec_t *)your_data;

  // start at the next header
  next_hdr = divx_find_header(divx, divx->m_buffer_on);
  if (next_hdr < 0) {
    next_hdr = divx_reset_buffer(divx);
    if (next_hdr < 0) return 0;
    next_hdr = divx_find_header(divx, next_hdr);
    if (next_hdr < 0) return 0;
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

  *ts = (divx->m_frame_on * M_LLU) / mp4_hdr.fps;
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
  divx_codec_t *divx = (divx_codec_t *)your;
  divx->m_buffer_on += bytes;
}

/*
 * divx_file_seek_to - find the closest time to ts, start from 
 * there
 */
int divx_file_seek_to (codec_data_t *your, uint64_t ts)
{
  divx_codec_t *divx = (divx_codec_t *)your;

  const frame_file_pos_t *fpos = divx->m_fpos->find_closest_point(ts);

  divx->m_frame_on = fpos->frames;
  divx->m_buffer_on = 0;
  divx->m_buffer_size = 0;

  fseek(divx->m_ifile, fpos->file_position, SEEK_SET);
  divx_reset_buffer(divx);
  return 0;
}

/*
 * divx_file_eof
 * see if we've hit the end of the file
 */
int divx_file_eof (codec_data_t *ifptr)
{
  divx_codec_t *divx = (divx_codec_t *)ifptr;

  return divx->m_buffer_on == divx->m_buffer_size && feof(divx->m_ifile);
}

