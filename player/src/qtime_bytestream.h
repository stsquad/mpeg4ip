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
#include <mp4/quicktime.h>
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
		    int track,
		    const char *name);
  ~CQTByteStreamBase();
  int eof(void);
  virtual void reset(void) = 0;
  virtual bool start_next_frame (uint8_t **buffer,
				 uint32_t *buflen,
				 frame_timestamp_t *ts,
				 void **ud) = 0;
  void used_bytes_for_frame(uint32_t bytes);
  void check_for_end_of_frame(void);
 protected:
  virtual void read_frame(uint32_t frame) = 0;
  CQtimeFile *m_parent;
  int m_eof;
  int m_track;
  uint32_t m_frame_on;
  uint32_t m_frames_max;
  uint32_t m_frame_rate;
  uint32_t m_max_frame_size;
  uint32_t m_frame_in_buffer;
  uint8_t *m_buffer;
  uint32_t m_byte_on;
  uint32_t m_this_frame_size;
  uint64_t m_total;
};

/*
 * CQTVideoByteStream is for video streams.  It is inherited from
 * CQTByteStreamBase.
 */
class CQTVideoByteStream : public CQTByteStreamBase
{
 public:
  CQTVideoByteStream(CQtimeFile *parent,
		     int track) :
    CQTByteStreamBase(parent, track, "video")
    {
    read_frame(0);
    };
  void reset(void);
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen, 
			frame_timestamp_t *ts,
			void **ud);
  int can_skip_frame(void) { return 1; };
  bool skip_next_frame (frame_timestamp_t *ts, int *hasSync, uint8_t **buffer,
		       uint32_t *buflen, void **ud);
  void play(uint64_t start);
  double get_max_playtime(void);
  void config(long num_frames, float frate, int time_scale);
 protected:
  void read_frame(uint32_t frame);
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
		     int track) :
    CQTByteStreamBase(parent, track, "audio")
    {
      read_frame(0);
    };
  void reset(void);
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);
  void play(uint64_t start);
  double get_max_playtime (void) {
    double ret = m_frames_max * m_samples_per_frame;
    ret /= m_frame_rate;
    return (ret);
  };
  void config(long num_frames, uint32_t frate, int duration) {
    m_frames_max = num_frames;
    m_frame_rate = frate;
    m_samples_per_frame = duration;
  };
 protected:
  void read_frame(uint32_t frame);
 private:
  void audio_set_timebase(long frame);
  int m_samples_per_frame;
};

#endif



