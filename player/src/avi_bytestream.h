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

#ifndef __AVI_BYTESTREAM_H__
#define __AVI_BYTESTREAM_H__
#include "avi/avilib.h"
#include "our_bytestream.h"
#include "avi_file.h"

/*
 * CQTByteStreamBase provides base class access to quicktime files.
 * Most functions are shared between audio and video.
 */
class CAviByteStreamBase : public COurInByteStream
{
 public:
  CAviByteStreamBase(CAviFile *parent, const char *which);
  ~CAviByteStreamBase();
  
  int eof(void);
  virtual void reset(void) = 0;
  virtual bool start_next_frame (uint8_t **buf,
				 uint32_t *buflen,
				 frame_timestamp_t *ts,
				 void **ud) = 0;
  virtual void used_bytes_for_frame(uint32_t bytes) = 0;
 protected:
  CAviFile *m_parent;
  int m_eof;
  uint32_t m_frame_on;
  uint32_t m_frame_in_buffer;
  uint32_t m_frames_max;
  uint32_t m_max_frame_size;
  uint8_t *m_buffer;
  uint32_t m_buffer_on;
  uint32_t m_byte_on;
  uint32_t m_this_frame_size;
  uint64_t m_total;
};

/*
 * CAviVideoByteStream is for video streams.  It is inherited from
 * CAviByteStreamBase.
 */
class CAviVideoByteStream : public CAviByteStreamBase
{
 public:
  CAviVideoByteStream(CAviFile *parent) : 
    CAviByteStreamBase(parent, "video")
    {
    read_frame(0);
    };
  void reset(void);
  bool start_next_frame(uint8_t **buf,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  void play (uint64_t start);
  double get_max_playtime (void) {
    double ret = m_frames_max;
    ret /= m_frame_rate;
    return (ret);
  };
  void config(long num_frames, double frate) {
    m_frames_max = num_frames;
    m_frame_rate = frate;
  };
 protected:
  void read_frame(uint32_t frame_to_read);
 private:
  void video_set_timebase(long frame);
  double m_frame_rate;
};

/*
 * CAviAudioByteStream is for audio streams.  It is inherited from
 * CAviByteStreamBase.
 */
class CAviAudioByteStream : public CAviByteStreamBase
{
 public:
  CAviAudioByteStream(CAviFile *parent) :
    CAviByteStreamBase(parent, "audio")
    {
    };
  void reset(void);
  bool start_next_frame(uint8_t **buffer, 
			uint32_t *buflen, 
			frame_timestamp_t *ts,
			void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  void play(uint64_t start);
  double get_max_playtime (void) {
    return (0.0);
  };
 protected:
  void audio_set_timebase(long frame);
  long m_file_pos;
};

#endif



