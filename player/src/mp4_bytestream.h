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
 * mp4_bytestream.h - provides bytestream access to quicktime files
 */

#ifndef __MP4_BYTESTREAM_H__
#define __MP4_BYTESTREAM_H__
#include <mp4.h>
#include "our_bytestream.h"
#include "mp4_file.h"
#include "player_util.h"
//#define OUTPUT_TO_FILE 1

#define THROW_MP4_END_OF_DATA ((int) 1)
#define THROW_MP4_END_OF_FRAME ((int) 2)
/*
 * CMp4ByteStreamBase provides base class access to quicktime files.
 * Most functions are shared between audio and video.
 */
class CMp4ByteStream : public COurInByteStream
{
 public:
  CMp4ByteStream(CMp4File *parent,
		 MP4TrackId track,
		 const char *type,
		 int has_video);
  ~CMp4ByteStream();
  int eof(void);
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int bSet);
  void reset(void);
  uint64_t start_next_frame(unsigned char **buffer = NULL,
			    uint32_t *buflen = NULL);
  void used_bytes_for_frame(uint32_t bytes);
  void get_more_bytes(unsigned char **buffer, uint32_t *buflen, uint32_t used,
		      int nothrow);
  int can_skip_frame(void) { return 1; };
  int skip_next_frame(uint64_t *ts, int *hasSyncFrame, unsigned char **buffer,
		      uint32_t *buflen);
  ssize_t read(unsigned char *buffer, size_t bytes);
  ssize_t read (char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };
  const char *get_throw_error(int error);
  int throw_error_minor(int error);
  void check_for_end_of_frame(void);
  double get_max_playtime(void);

  void set_start_time(uint64_t start);
 private:
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  void read_frame(uint32_t frame);
  CMp4File *m_parent;
  int m_eof;
  MP4TrackId m_track;
  MP4SampleId m_frames_max;

  u_int8_t *m_buffer_on;
  MP4SampleId m_frame_on;
  uint64_t m_frame_on_ts;
  int m_frame_on_has_sync;
  
  MP4SampleId m_frame_in_buffer;
  uint64_t m_frame_in_buffer_ts;
  int m_frame_in_buffer_has_sync;

  MP4SampleId m_frame_in_bookmark;
  uint64_t m_frame_in_bookmark_ts;
  int m_frame_in_bookmark_has_sync;

  u_int32_t m_max_frame_size;
  u_int8_t *m_buffer;
  int m_bookmark;
  u_int8_t *m_bookmark_buffer;
  uint32_t m_byte_on, m_bookmark_byte_on;
  uint32_t m_this_frame_size, m_bookmark_this_frame_size;
  uint64_t m_total, m_total_bookmark;
  int m_bookmark_read_frame;
  int m_bookmark_read_frame_size;
  const char *m_type;
  void set_timebase(MP4SampleId frame);
  double m_max_time;
  int m_has_video;
};

/*
 * CMp4VideoByteStream is for video streams.  It is inherited from
 * CMp4ByteStreamBase.
 */
class CMp4VideoByteStream : public CMp4ByteStream
{
 public:
  CMp4VideoByteStream(CMp4File *parent,
		      MP4TrackId track) :
    CMp4ByteStream(parent, track, "video", 1) {};
};

/*
 * CMp4AudioByteStream is for audio streams.  It is inherited from
 * CMp4ByteStreamBase.
 */
class CMp4AudioByteStream : public CMp4ByteStream
{
 public:
  CMp4AudioByteStream(CMp4File *parent,
		      MP4TrackId track) :
    CMp4ByteStream(parent, track, "audio", 0) {};

};

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mp4f_message, "mp4file")
#else
#define mp4f_message(loglevel, fmt...) message(loglevel, "mp4file", fmt)
#endif

#endif



