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
#include <ismacryplib.h>
#include "our_bytestream.h"
#include "mp4_file.h"
#include "player_util.h"

//Uncomment these #defines to dump buffers to file.
//#define OUTPUT_TO_FILE 1
//#define ISMACRYP_DEBUG 1

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
		 bool has_video);
  ~CMp4ByteStream();
  int eof(void);
  void reset(void);
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *pts,
			void **ud);
  void used_bytes_for_frame(uint32_t bytes);
  int can_skip_frame(void) { return 1; };
  bool skip_next_frame(frame_timestamp_t *ts, int *hasSyncFrame, 
		      uint8_t **buffer,
		      uint32_t *buflen, void **ud);
  void check_for_end_of_frame(void);
  double get_max_playtime(void);

  const char *get_inuse_kms_uri(void);

  void play(uint64_t start);

  u_int8_t *get_buffer() {return m_buffer; }
  void set_buffer(u_int8_t *buffer) {
    memset(m_buffer, 0, m_max_frame_size * sizeof(u_int8_t));
  }
  uint32_t get_this_frame_size() {return m_this_frame_size;}
  void set_this_frame_size(uint32_t frame_size)
    {m_this_frame_size = frame_size;}
#ifdef ISMACRYP_DEBUG
  FILE *my_enc_file;
  FILE *my_unenc_file;
  FILE *my_unenc_file2;
#endif
 protected:
#ifdef OUTPUT_TO_FILE
  FILE *m_output_file;
#endif
  void read_frame(uint32_t frame, frame_timestamp_t *ts);
  CMp4File *m_parent;
  bool m_eof;
  MP4TrackId m_track;
  MP4SampleId m_frames_max;
  uint32_t m_sample_freq;

  MP4SampleId m_frame_on;
  uint64_t m_frame_on_ts;
  uint32_t m_frame_on_sample_ts;
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
  bool m_has_video;
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

class CMp4TextByteStream : public CMp4ByteStream
{
 public:
  CMp4TextByteStream(CMp4File *parent, MP4TrackId track,
		     const char *base_url) :
    CMp4ByteStream(parent, track, "cntl", 0) {
    m_base_url = base_url;
  };
  ~CMp4TextByteStream(void) {
    //CHECK_AND_FREE(m_base_url);
  };
  bool start_next_frame(uint8_t **buffer, 
			uint32_t *buflen,
			frame_timestamp_t *ptr,
			void **ud);
 protected:
  const char *m_base_url;
};

			  
class CMp4H264VideoByteStream : public CMp4VideoByteStream
{
 public:
  CMp4H264VideoByteStream(CMp4File *parent, 
			  MP4TrackId track) :
    CMp4VideoByteStream(parent, track) {
    m_translate_buffer = NULL;
    m_translate_buffer_size = 0;
    m_buflen_size = 0;
  };
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *pts,
			void **ud);
 protected:
  uint32_t read_nal_size(uint8_t *buffer) {
    if (m_buflen_size == 1) {
      return *buffer;
    } else if (m_buflen_size == 2) {
      return (buffer[0] << 8) | buffer[1];
    } else if (m_buflen_size == 3) {
      return (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    }
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
  };
  uint8_t *m_translate_buffer;
  uint32_t m_translate_buffer_size;
  uint32_t m_buflen_size;
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

/*
 * CMp4EncVideoByteStream is for encrypted video streams.  
 * It is inherited from CMp4VideoByteStreamBase.
 */
class CMp4EncVideoByteStream : public CMp4VideoByteStream
{
 public:
  CMp4EncVideoByteStream(CMp4File   *parent,
			 MP4TrackId track,
                         uint64_t   IVLength ) :
    CMp4VideoByteStream(parent, track) {
    ismacryp_rc_t rc = ismacrypInitSession(&m_ismaCryptSId,KeyTypeVideo);

    if (rc != ismacryp_rc_ok ) {
      player_error_message("can't initialize video ismacryp session rc: %u\n", rc);
    }
    else {
       rc = ismacrypSetIVLength(m_ismaCryptSId, (uint8_t)IVLength);
       if( rc != ismacryp_rc_ok )
         player_error_message(
          "can't set IV length for ismacryp decode session %d, rc: %u\n",
          m_ismaCryptSId, rc);
    }
  };
  ~CMp4EncVideoByteStream() {
    ismacryp_rc_t rc = ismacrypEndSession(m_ismaCryptSId);
    if (rc != ismacryp_rc_ok ) {
       player_error_message(
          "could not end video ismacryp session %d, rc: %u\n",
          m_ismaCryptSId, rc);
     }
  }
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);
  ismacryp_session_id_t m_ismaCryptSId; // eventually make it private
                                        // and add accessor function
};  

/*
 * CMp4EncH264VideoByteStream is for encrypted H264 AVC video streams.  
 * It is inherited from CMp4VideoByteStreamBase.
 */
class CMp4EncH264VideoByteStream : public CMp4VideoByteStream
{
 public:
  CMp4EncH264VideoByteStream(CMp4File   *parent,
			 MP4TrackId track,
                         uint64_t   IVLength ) :
    CMp4VideoByteStream(parent, track) {
    m_translate_buffer = NULL;
    m_translate_buffer_size = 0;
    m_buflen_size = 0;
    m_kms_uri = NULL;
    ismacryp_rc_t rc = ismacrypInitSession(&m_ismaCryptSId,KeyTypeVideo);

    if (rc != ismacryp_rc_ok ) {
      player_error_message("can't initialize video ismacryp session rc: %u\n", rc);
    }
    else {
       rc = ismacrypSetIVLength(m_ismaCryptSId, (uint8_t)IVLength);
       if( rc != ismacryp_rc_ok ) {
         player_error_message(
          "can't set IV length for ismacryp decode session %d, rc: %u\n",
          m_ismaCryptSId, rc);
       }
      ismacrypGetKMSUri(m_ismaCryptSId, &m_kms_uri);
    }
  };
  ~CMp4EncH264VideoByteStream() {
    ismacryp_rc_t rc = ismacrypEndSession(m_ismaCryptSId);
    if (rc != ismacryp_rc_ok ) {
       player_error_message(
          "could not end video ismacryp session %d, rc: %u\n",
          m_ismaCryptSId, rc);
     }
     if (m_kms_uri != NULL)
	free((void *)m_kms_uri);
  }
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud);

  const char *get_inuse_kms_uri() { return((const char *)m_kms_uri); }

  ismacryp_session_id_t m_ismaCryptSId; // eventually make it private
                                        // and add accessor function

 protected:
  uint32_t read_nal_size(uint8_t *buffer) {
    if (m_buflen_size == 1) {
      return *buffer;
    } else if (m_buflen_size == 2) {
      return (buffer[0] << 8) | buffer[1];
    } else if (m_buflen_size == 3) {
      return (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    }
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
  };
  uint8_t *m_translate_buffer;
  uint32_t m_translate_buffer_size;
  uint32_t m_buflen_size;
  const char *m_kms_uri;
};  

/*
 * CMp4EncAudioByteStream is for encrypted audio streams.  
 * It is inherited from CMp4AudioByteStreamBase.
 */
class CMp4EncAudioByteStream : public CMp4AudioByteStream
{
 public:
  CMp4EncAudioByteStream(CMp4File   *parent,
			 MP4TrackId track,
                         uint64_t   IVLength ) :
    CMp4AudioByteStream(parent, track) {
    ismacryp_rc_t rc = ismacrypInitSession(&m_ismaCryptSId,KeyTypeAudio);
    if ( rc != ismacryp_rc_ok ) {
      player_error_message("can't initialize audio ismacryp session rc: %u\n", rc);
    }
    else {
       rc = ismacrypSetIVLength(m_ismaCryptSId, (uint8_t)IVLength);
       if( rc != ismacryp_rc_ok )
         player_error_message(
          "can't set IV length for ismacryp decode session %d, rc: %u\n",
          m_ismaCryptSId, rc);
    }
  };
  ~CMp4EncAudioByteStream() {
    ismacryp_rc_t rc = ismacrypEndSession(m_ismaCryptSId);
    if ( rc != ismacryp_rc_ok) {
       player_error_message(
          "could not end audio ismacryp session %d, rc: %u\n",
          m_ismaCryptSId, rc);
    }
  };
  bool start_next_frame(uint8_t **buffer,
			uint32_t *buflen,
			frame_timestamp_t *ts,
			void **ud); 

  ismacryp_session_id_t m_ismaCryptSId; // eventually make it private
                                        // and add accessor function
};

#ifdef _WIN32
DEFINE_MESSAGE_MACRO(mp4f_message, "mp4file")
#else
#define mp4f_message(loglevel, fmt...) message(loglevel, "mp4file", fmt)
#endif

#endif



