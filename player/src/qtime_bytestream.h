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
 * qtime_bytestream.h - provides bytestream access to quicktime files
 */

#ifndef __QTIME_BYTESTREAM_H__
#define __QTIME_BYTESTREAM_H__
#include <quicktime.h>
#include "our_bytestream.h"
#include "qtime_file.h"


/*
 * CQTByteStreamBase provides base class access to quicktime files.
 * Most functions are shared between audio and video.
 */
class CQTByteStreamBase : public COurInByteStream
{
 public:
  CQTByteStreamBase(CQtimeFile *parent,
		    CPlayerMedia *m,
		    int track,
		    const char *type);
  ~CQTByteStreamBase();
  int eof(void);
  unsigned char get(void);
  unsigned char peek(void);
  void bookmark(int bSet);
  virtual void reset(void) = 0;
  virtual uint64_t start_next_frame (void) = 0;
  size_t read(unsigned char *buffer, size_t bytes);
  size_t read (char *buffer, size_t bytes) {
    return (read((unsigned char *)buffer, bytes));
  };
  void check_for_end_of_frame(void);
 protected:
  virtual void read_frame(void) = 0;
  CQtimeFile *m_parent;
  int m_eof;
  int m_track;
  size_t m_frame_on;
  size_t m_frames_max;
  size_t m_frame_rate;
  size_t m_max_frame_size;
  unsigned char *m_buffer;
  int m_bookmark;
  unsigned char *m_bookmark_buffer;
  unsigned char *m_buffer_on;
  size_t m_byte_on, m_bookmark_byte_on;
  size_t m_bookmark_frame_on;
  size_t m_this_frame_size, m_bookmark_this_frame_size;
  uint64_t m_total, m_total_bookmark;
  int m_bookmark_read_frame;
  int m_bookmark_read_frame_size;
  const char *m_type;
};

/*
 * CQTVideoByteStream is for video streams.  It is inherited from
 * CQTByteStreamBase.
 */
class CQTVideoByteStream : public CQTByteStreamBase
{
 public:
  CQTVideoByteStream(CQtimeFile *parent,
		     CPlayerMedia *m,
		     int track) :
    CQTByteStreamBase(parent, m, track, "video")
    {
    read_frame();
    };
  void reset(void);
  uint64_t start_next_frame(void);
  void set_start_time(uint64_t start);
  double get_max_playtime(void);
  void config(long num_frames, float frate, int time_scale);
 protected:
  void read_frame(void);
 private:
  void video_set_timebase(long frame);
  int m_time_scale;
  double m_max_time;
};

/*
 * CQTAudioByteStream is for audio streams.  It is inherited from
 * CQTByteStreamBase.
 */
class CQTAudioByteStream : public CQTByteStreamBase
{
 public:
  CQTAudioByteStream(CQtimeFile *parent,
		     CPlayerMedia *m,
		     int track,
		     int add_len_to_frame) :
    CQTByteStreamBase(parent, m, track, "audio")
    {
      m_add_len_to_stream = add_len_to_frame;
      read_frame();
    };
  void reset(void);
  uint64_t start_next_frame(void);
  void set_start_time(uint64_t start);
  double get_max_playtime (void) {
    double ret = m_frames_max * m_samples_per_frame;
    ret /= m_frame_rate;
    return (ret);
  };
  void config(long num_frames, size_t frate, int duration) {
    m_frames_max = num_frames;
    m_frame_rate = frate;
    m_samples_per_frame = duration;
  };
 protected:
  void read_frame(void);
  size_t read_a_frame(unsigned char **ppbuff);
 private:
  void audio_set_timebase(long frame);
  int m_add_len_to_stream;
  int m_samples_per_frame;
};

#endif



