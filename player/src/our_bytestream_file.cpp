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

COurInByteStreamFile::COurInByteStreamFile (const char *filename) :
  COurInByteStream()
{
  m_filename = strdup(filename);
  m_file = fopen(m_filename, "rb");
  m_frames = 0;
  m_total = 0;
  m_bookmark_loaded = 0;
  m_bookmark = 0;
  m_buffer_size_max = 4096;
  m_buffer_size = 1;
  m_orig_buffer = (unsigned char *)malloc(m_buffer_size_max);
  m_bookmark_buffer = (unsigned char *)malloc(m_buffer_size_max);
  m_file_pos_head = m_file_pos_tail = NULL;
  read_frame();
}

COurInByteStreamFile::~COurInByteStreamFile(void)
{
  if (m_file) {
    fclose(m_file);
    m_file = NULL;
  }
  if (m_filename) {
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
}

void COurInByteStreamFile::set_start_time (uint64_t start) 
{
  if (start == 0) {
    // reset it back to the beginning
    m_frames = 0;
    m_play_start_time = start;
    reset();
  } else {
    // Non-zero start time - see if we have the time in our list.
    long pos_to_set = 0;
    uint64_t frames = 0;
    uint64_t time;
    m_bookmark_loaded = 0;
    m_bookmark = 0;
    time = 0;
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
	  time = p->timestamp;
	  pos_to_set = p->file_position;
	  frames = p->frames;
	}
      } else {
	// After last pointer.
	time = m_file_pos_tail->timestamp;
	pos_to_set = m_file_pos_tail->file_position;
	frames = m_file_pos_tail->frames;
      }
    }
    // seek to last known position
    fseek(m_file, pos_to_set, SEEK_SET); 
    read_frame();
    // Read forward from there until we hit the time we want.
    m_frames = frames;
    m_play_start_time = time;
    while (m_play_start_time < start) {
      m_codec->skip_frame(time);
      m_play_start_time = (m_frames * 1000) / m_frame_per_sec;
      m_frames++;
    }
  }
}

uint64_t COurInByteStreamFile::start_next_frame (void) 
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
  return (ret);
}

void COurInByteStreamFile::read_frame (void)
{
  if (m_bookmark) {
    if (m_bookmark_loaded) {
      m_buffer_on = m_bookmark_buffer;
      m_buffer_size = m_bookmark_loaded_size;
      m_buffer_position = m_bookmark_loaded_position;
      m_index = 0;
      return;
    }
	
    m_bookmark_loaded_position = ftell(m_file);
    m_bookmark_loaded_size = m_buffer_size = 
		fread(m_bookmark_buffer, 1, m_buffer_size_max, m_file);
    if (m_bookmark_loaded_size > 0) {
      m_index = 0;
      m_bookmark_loaded = 1;
      m_buffer_on = m_bookmark_buffer;
    }
    return;
  }
  if (m_bookmark_loaded) {
    m_bookmark_loaded = 0;
    m_buffer_on = m_bookmark_buffer;
    m_bookmark_buffer = m_orig_buffer;
    m_orig_buffer = m_buffer_on;
    m_buffer_size = m_bookmark_loaded_size;
    m_buffer_position = m_bookmark_loaded_position;
    m_index = 0;
    return;
  }
  m_buffer_position = ftell(m_file);
  m_buffer_size = fread(m_orig_buffer, 1, m_buffer_size_max, m_file);
  m_buffer_on = m_orig_buffer;
  m_index = 0;
}

int COurInByteStreamFile::eof (void) 
{ 
  return (m_buffer_size == 0);
}

unsigned char COurInByteStreamFile::get (void) 
{
  unsigned char ret = m_buffer_on[m_index];
  if (m_buffer_size == 0) {
    throw THROW_PAST_EOF;
  }
  m_index++;
  m_total++;
  if (m_index >= m_buffer_size) {
    read_frame();
  }
  return (ret);
}

unsigned char COurInByteStreamFile::peek (void)
{
  return (m_buffer_on[m_index]);
}

void COurInByteStreamFile::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark = 1;
    m_bookmark_buffer_size = m_buffer_size;
    m_bookmark_index = m_index;
    m_bookmark_total = m_total;
  } else {
    m_bookmark = 0;
    m_buffer_size = m_bookmark_buffer_size;
    m_index = m_bookmark_index;
    m_buffer_on = m_orig_buffer;
    m_total = m_bookmark_total;
  }
}

void COurInByteStreamFile::reset (void)
{
  fseek(m_file, 0, SEEK_SET); 
  m_bookmark_loaded = 0;
  m_bookmark = 0;
  m_frames = 0;
  read_frame();
}

ssize_t COurInByteStreamFile::read (unsigned char *buffer, size_t bytestoread) 
{
  if (m_bookmark && bytestoread > m_buffer_size_max) {
    // Can't do this...
    return 0;
  }
  size_t inbuffer;
  ssize_t readbytes = 0;
  if (m_buffer_size == 0) {
    throw THROW_PAST_EOF;
  }
  do {
    inbuffer = m_buffer_size - m_index;
    if (bytestoread < inbuffer)
      inbuffer = bytestoread;
    memcpy(buffer, &m_buffer_on[m_index], inbuffer);
    readbytes += inbuffer;
    buffer += inbuffer;
    bytestoread -= inbuffer;
    m_index += inbuffer;
    if (m_index >= m_buffer_size) {
      read_frame();
      if (m_buffer_size == 0) {
	throw THROW_PAST_EOF;
      }
    }
    m_total += inbuffer;
  } while (bytestoread > 0 && m_buffer_size > 0);

  return (readbytes);
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
