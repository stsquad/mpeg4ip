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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg2f_bytestream.h - provides bytestream access to quicktime files
 */

#ifndef __MPEG2F_BYTESTREAM_H__
#define __MPEG2F_BYTESTREAM_H__
#include "our_bytestream.h"
#include "player_util.h"
#include "mpeg2t_file.h"
#include "mpeg2t_private.h"
#include "mpeg2t/mpeg2_transport.h"
//#define OUTPUT_TO_FILE 1

/*
 * CMpeg2tByteStreamBase provides base class access to quicktime files.
 * Most functions are shared between audio and video.
 */
class CMpeg2fByteStream : public COurInByteStream
{
 public:
  CMpeg2fByteStream(CMpeg2tFile *f, 
		    mpeg2t_es_t *es_pid,
		    const char *type,
		    int has_video);
  ~CMpeg2fByteStream();
  virtual int eof(void);
  virtual void reset(void);
  virtual bool start_next_frame(uint8_t **buffer,
				uint32_t *buflen,
				frame_timestamp_t *ts,
				void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  int can_skip_frame(void) { return 1; };
  bool skip_next_frame(frame_timestamp_t *ts, int *hasSyncFrame, 
		      uint8_t **buffer,
		      uint32_t *buflen, void **ud);
  void check_for_end_of_frame(void);
  virtual double get_max_playtime(void);

  virtual void play(uint64_t start);
  virtual void pause(void);
 protected:
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  int m_has_video;
  mpeg2t_es_t *m_es_pid;
  virtual int get_timestamp_for_frame(mpeg2t_frame_t *, frame_timestamp_t *ts) = 0;
  mpeg2t_frame_t *m_frame;
  int m_timestamp_loaded;
  uint64_t m_last_timestamp;
  uint32_t m_audio_last_freq_timestamp;
  int m_frames_since_last_timestamp;
  CMpeg2tFile *m_file;
};

/*
 * CMpeg2fVideoByteStream is for video streams.  It is inherited from
 * CMpeg2fByteStreamBase.
 */
class CMpeg2fVideoByteStream : public CMpeg2fByteStream
{
 public:
  CMpeg2fVideoByteStream(CMpeg2tFile *f, mpeg2t_es_t *es_pid) :
    CMpeg2fByteStream(f, es_pid, "video", 1) {
    m_have_prev_frame_type = 0;
  };
  void reset(void);
  void play(uint64_t start);
 protected:
  int get_timestamp_for_frame(mpeg2t_frame_t *, frame_timestamp_t *ts);
  int m_have_prev_frame_type;
  int m_prev_frame_type;
  uint64_t m_prev_ts;
};

/*
 * CMpeg2fAudioByteStream is for audio streams.  It is inherited from
 * CMpeg2tByteStreamBase.
 */
class CMpeg2fAudioByteStream : public CMpeg2fByteStream
{
 public:
  CMpeg2fAudioByteStream(CMpeg2tFile *f, mpeg2t_es_t *es_pid) :
    CMpeg2fByteStream(f, es_pid, "audio", 0) {};
  void reset(void);
 protected:
  int get_timestamp_for_frame(mpeg2t_frame_t *, frame_timestamp_t *ts);

};

#endif



