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
#ifdef ISMACRYPT
#include <ismacryplib.h>
#endif
#include "our_bytestream.h"
#include "mp4_file.h"
#include "player_util.h"
//#define OUTPUT_TO_FILE 1

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
  void reset(void);
  uint64_t start_next_frame(uint8_t **buffer,
			    uint32_t *buflen,
			    void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  int can_skip_frame(void) { return 1; };
  int skip_next_frame(uint64_t *ts, int *hasSyncFrame, uint8_t **buffer,
		      uint32_t *buflen, void **ud);
  void check_for_end_of_frame(void);
  double get_max_playtime(void);

  void play(uint64_t start);
#ifdef ISMACRYPT
  u_int8_t *get_buffer() {return m_buffer; }
  void set_buffer(u_int8_t *buffer) {memcpy(m_buffer, buffer, sizeof(buffer));}
  uint32_t get_this_frame_size() {return m_this_frame_size;}
  void set_this_frame_size(uint32_t frame_size)
    {m_this_frame_size = frame_size;}
#endif
 private:
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  void read_frame(uint32_t frame);
  CMp4File *m_parent;
  int m_eof;
  MP4TrackId m_track;
  MP4SampleId m_frames_max;

  MP4SampleId m_frame_on;
  uint64_t m_frame_on_ts;
  int m_frame_on_has_sync;
  
  MP4SampleId m_frame_in_buffer;
  uint64_t m_frame_in_buffer_ts;
  int m_frame_in_buffer_has_sync;

  u_int32_t m_max_frame_size;
  u_int8_t *m_buffer;
  uint32_t m_byte_on;
  uint32_t m_this_frame_size;
  uint64_t m_total;
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

#ifdef ISMACRYPT
/*
 * CMp4EncVideoByteStream is for encrypted video streams.  
 * It is inherited from CMp4VideoByteStreamBase.
 */
class CMp4EncVideoByteStream : public CMp4VideoByteStream
{
 public:
  CMp4EncVideoByteStream(CMp4File *parent,
			 MP4TrackId track) :
    CMp4VideoByteStream(parent, track) {
    if (ismacrypInitSession(&m_ismaCryptSId) != 0) {
      // error
      printf("can't initialize video ismacryp session\n");
    }
  };
  ~CMp4EncVideoByteStream() {
    if (ismacrypEndSession(m_ismaCryptSId) != 0) {
       // error
       printf("could not end the video ismacryp session\n");
     }
  }
  uint64_t start_next_frame(uint8_t **buffer,
			    uint32_t *buflen,
			    void **ud);
  ismacryp_session_id_t m_ismaCryptSId; // eventually make it private
                                        // and add accessor function
};  

/*
 * CMp4EncAudioByteStream is for encrypted audio streams.  
 * It is inherited from CMp4AudioByteStreamBase.
 */
class CMp4EncAudioByteStream : public CMp4AudioByteStream
{
 public:
  CMp4EncAudioByteStream(CMp4File *parent,
			 MP4TrackId track) :
    CMp4AudioByteStream(parent, track) {
    if (ismacrypInitSession(&m_ismaCryptSId) != 0) {
	  // error
      printf("can't initialize audio ismacryp session\n");
    }};
  ~CMp4EncAudioByteStream() {
    if (ismacrypEndSession(m_ismaCryptSId) != 0) {
      // error
      printf("could not end the audio ismacryp session\n");
    }
  }
  uint64_t start_next_frame(uint8_t **buffer,
			    uint32_t *buflen,
			    void **ud); 
  ismacryp_session_id_t m_ismaCryptSId; // eventually make it private
                                        // and add accessor function
};
#endif

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mp4f_message, "mp4file")
#else
#define mp4f_message(loglevel, fmt...) message(loglevel, "mp4file", fmt)
#endif

#endif



