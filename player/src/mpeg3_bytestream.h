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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mp4_bytestream.h - provides bytestream access to quicktime files
 */

#ifndef __MPEG3_BYTESTREAM_H__
#define __MPEG3_BYTESTREAM_H__
#include "mpeg2_ps.h"
#include "our_bytestream.h"
#include "player_util.h"
//#define OUTPUT_TO_FILE 1

/*
 * CMp4ByteStreamBase provides base class access to quicktime files.
 * Most functions are shared between audio and video.
 */
class CMpeg3VideoByteStream : public COurInByteStream
{
 public:
  CMpeg3VideoByteStream(mpeg2ps_t *file, int track);
  ~CMpeg3VideoByteStream();
  int eof(void);
  void reset(void);
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  int can_skip_frame(void) { return 1; };
  bool skip_next_frame(frame_timestamp_t *ts, int *hasSyncFrame, 
		      uint8_t **buffer,
		      uint32_t *buflen, void **ud);
  double get_max_playtime(void);

  void play(uint64_t start);
 private:
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  mpeg2ps_t *m_file;
  int m_stream;

  int m_eof;
  float m_frame_rate;
  long m_frame_on;
  uint64_t m_total;
  void set_timebase(uint64_t time);
  double m_max_time;
  int m_changed_time;
};

class CMpeg3AudioByteStream : public COurInByteStream
{
 public:
  CMpeg3AudioByteStream(mpeg2ps_t *file, int track);
  ~CMpeg3AudioByteStream();
  int eof(void);
  void reset(void);
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  int can_skip_frame(void) { return 1; };
  bool skip_next_frame(frame_timestamp_t *ts, int *hasSyncFrame, 
		      uint8_t **buffer,
		      uint32_t *buflen, void **ud);
  double get_max_playtime(void);

  void play(uint64_t start);
 private:
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  mpeg2ps_t *m_file;
  int m_stream;

  int m_eof;
  uint32_t m_freq;
  uint32_t m_chans;
  long m_frames_max;
  long m_frame_on;
  uint64_t m_total;
  void set_timebase(uint64_t time);
  double m_max_time;
  int m_changed_time;
};

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mpeg3f_message, "mpeg3file")
#else
#define mpeg3f_message(loglevel, fmt...) message(loglevel, "mpeg3file", fmt)
#endif

#endif



