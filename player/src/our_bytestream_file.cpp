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

#include "our_bytestream_file.h"
#include "player_util.h"
//#define FILE_DEBUG

COurInByteStreamFile::COurInByteStreamFile (const char *filename) :
  COurInByteStream("File")
{
  m_filename = strdup(filename);
  m_file = fopen(m_filename, FOPEN_READ_BINARY);
  m_frames = 0;
  m_total = 0;
  m_buffer_size_max = 4096;
  m_buffer_size = 1;
  m_orig_buffer = (unsigned char *)malloc(m_buffer_size_max);
  m_file_pos_head = m_file_pos_tail = NULL;
  read_frame();
}

COurInByteStreamFile::~COurInByteStreamFile(void)
{
  if (m_file) {
    fclose(m_file);
    m_file = NULL;
  }
  if (m_filename != NULL) {
    free((void *)m_filename);
    m_filename = NULL;
  }
  player_debug_message("File bytes " LLU, m_total);
  while (m_file_pos_head != NULL) {
    frame_file_pos_t *p = m_file_pos_head;
    m_file_pos_head = m_file_pos_head->next;
    free(p);
  }
  m_file_pos_tail = NULL;
  m_buffer_on = NULL;
  if (m_orig_buffer != NULL) {
    free(m_orig_buffer);
  }
}

void COurInByteStreamFile::set_start_time (uint64_t start) 
{
#ifdef FILE_DEBUG
  player_debug_message("Skip ahead to %llu", start);
#endif
  if (start == 0) {
    // reset it back to the beginning
    m_frames = 0;
    m_play_start_time = start;
    reset();
  } else {
    // Non-zero start time - see if we have the time in our list.
    long pos_to_set = 0;
    uint64_t frames = 0;
    uint64_t start_time;
    start_time = 0;
    if (m_file_pos_tail != NULL) {
      // find closest
      if (start < m_file_pos_tail->timestamp) {
	frame_file_pos_t *p, *q;
	p = NULL;
	q = m_file_pos_head;
	while (q->timestamp < start) {
	  p = q;
	  q = p->next;
	}
	if (p != NULL) {
	  start_time = p->timestamp;
	  pos_to_set = p->file_position;
	  frames = p->frames;
	}
      } else {
	// After last pointer.
	start_time = m_file_pos_tail->timestamp;
	pos_to_set = m_file_pos_tail->file_position;
	frames = m_file_pos_tail->frames;
      }
    }
    // seek to last known position
    fseek(m_file, pos_to_set, SEEK_SET); 
    m_buffer_position = pos_to_set;
    read_frame();
    // Read forward from there until we hit the time we want.
#ifdef FILE_DEBUG
    player_debug_message("Skipping ahead - start position %ld", 
			 pos_to_set);
#endif
    m_frames = frames;
    m_play_start_time = start_time;
    while (m_play_start_time < start) {
#ifdef FILE_DEBUG
      player_debug_message("skip frame %llu time %llu", m_frames, 
			   m_play_start_time);
#endif
      m_codec->skip_frame(m_play_start_time,
			  m_buffer_on + m_index, 
			  m_buffer_size - m_index);
      m_frames++;
      m_play_start_time = (m_frames * 1000) / m_frame_per_sec;
    }
    player_debug_message("Done skipping");
  }
}

uint64_t COurInByteStreamFile::start_next_frame (unsigned char **buffer, 
						 uint32_t *buflen) 
{
  uint64_t ret;
  ret = (m_frames * 1000) / m_frame_per_sec;
  if ((m_frames % m_frame_per_sec) == 0) {
    // Create file position pointer for this timestamp.
    frame_file_pos_t *newptr = NULL;
    // Make sure we don't already have this one in the list
    // only run through the list if current time is less than the
    // tail pointer time.
    if ((m_file_pos_tail != NULL) &&
	(m_file_pos_tail->timestamp < ret)) {
      newptr = m_file_pos_head;
      while (newptr != NULL && newptr->timestamp != ret) {
	newptr = newptr->next;
      }
    }
    if (newptr == NULL) {
      // Not in list - malloc, and add in sequential order.
      newptr = (frame_file_pos_t *)malloc(sizeof(frame_file_pos_t));
      newptr->timestamp = ret;
      newptr->file_position = m_buffer_position + m_index;
      newptr->frames = m_frames;
      newptr->next = NULL;
#ifdef FILE_DEBUG
      player_debug_message("Position %ld for ts %llu %llu", 
			   newptr->file_position,
			   ret,
			   m_frames);
#endif
      if (m_file_pos_tail == NULL) {
	// add at head
	m_file_pos_tail = m_file_pos_head = newptr;
      } else if (ret > m_file_pos_tail->timestamp) {
	// add at end
	m_file_pos_tail->next = newptr;
	m_file_pos_tail = newptr;
      } else {
	// insert in the middle.
	frame_file_pos_t *p, *q;
	p = m_file_pos_head;
	q = m_file_pos_head->next;
	while (q->timestamp < ret) {
	  p = q;
	  q = q->next;
	}
	newptr->next = q;
	p->next = newptr;
      }
    }
  }
  m_frames++;
  *buffer = m_buffer_on + m_index;
  *buflen = m_buffer_size - m_index;
#ifdef FILE_DEBUG
  player_debug_message("Start frame %lld %d %d", m_total, m_index, *buflen);
  if (m_buffer_on != NULL) 
    player_debug_message("%02x %02x %02x %02x", 
			 *(m_buffer_on + m_index), 
			 *(m_buffer_on + m_index + 1), 
			 *(m_buffer_on + m_index + 2), 
			 *(m_buffer_on + m_index + 3)); 
#endif
  return (ret);
}

void COurInByteStreamFile::used_bytes_for_frame (uint32_t bytes)
{
  m_index += bytes;
  m_total += bytes;
#ifdef FILE_DEBUG
  player_debug_message("used %d bytes - new index %d", bytes, m_index);
#endif
  if (m_index >= m_buffer_size) {
    read_frame();
  }
}

void COurInByteStreamFile::get_more_bytes (unsigned char **buffer,
					   uint32_t *buflen,
					   uint32_t used,
					   int get)
{
  uint32_t diff;
  m_index += used;
  m_total += used;

  diff = m_buffer_size - m_index; // contains # of bytes to save
#ifdef FILE_DEBUG
  player_debug_message("get_more - used %d index %d diff %d", 
		       used, m_index, diff);
#endif
  if (diff > 0) {
    memmove(m_buffer_on,
	    m_buffer_on + m_index,
	    diff);
    m_index = diff;
    read_frame(1);
    if (m_buffer_size <= diff) {
#ifdef FILE_DEBUG
      player_debug_message("Throwing EOF");
#endif
      throw THROW_PAST_EOF;
    }
  } else {
    read_frame();
    if (m_buffer_size == 0) {
      throw THROW_PAST_EOF;
#ifdef FILE_DEBUG
      player_debug_message("Throwing EOF");
#endif
    }
  }
  *buffer = m_buffer_on;
  *buflen = m_buffer_size;
}

void COurInByteStreamFile::read_frame (int from_index)
{
  m_buffer_position = ftell(m_file);
  if (from_index == 0) {
    m_buffer_size = fread(m_orig_buffer, 1, m_buffer_size_max, m_file);
#ifdef FILE_DEBUG
    player_debug_message("read frame - position %ld bufsize %d", 
			 m_buffer_position, m_buffer_size);
#endif
  } else {
    m_buffer_position -= m_index;
    m_buffer_size = fread(m_orig_buffer + m_index,
			  1, 
			  m_buffer_size_max - m_index,
			  m_file);
#ifdef FILE_DEBUG
    player_debug_message("read frame - position %ld index %d bufsize %d", 
			 m_buffer_position,
			 m_index,
			 m_buffer_size);
#endif
    if (m_buffer_size != 0) 
      m_buffer_size += m_index;
  }
  m_buffer_on = m_orig_buffer;
  m_index = 0;
}

int COurInByteStreamFile::eof (void) 
{ 
  return (m_buffer_size == 0);
}

void COurInByteStreamFile::reset (void)
{
  fseek(m_file, 0, SEEK_SET); 
  m_frames = 0;
  read_frame();
}

const char *COurInByteStreamFile::get_throw_error (int error)
{
  if (error == THROW_PAST_EOF) {
    return "Past EOF";
  }
  player_debug_message("File bytestream - unknown error %d", error);
  return "Unknown Error";
}

int COurInByteStreamFile::throw_error_minor (int error)
{
  return 0;
}
/* end file our_bytestream_file.cpp */
