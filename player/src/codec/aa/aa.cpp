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
#include "aa.h"
extern "C" {
#include <faad/filestream.h>
}

/*
 * C interfaces for faac callbacks
 */
int c_read_byte (FILE_STREAM *fs, int *err)
{
  CAACodec *aa = (CAACodec *)fs->userdata;
  return (aa->read_byte(fs, err));
}

unsigned long c_filelength (FILE_STREAM *fs)
{
  return 0;
}

void c_seek (FILE_STREAM *fs, unsigned long offset, int mode)
{
}

void c_close(FILE_STREAM *fs)
{
}

void c_reset (FILE_STREAM *fs)
{
  CAACodec *a = (CAACodec *)fs->userdata;
  a->reset();
}
int c_peek (FILE_STREAM *fs, void *data, int len)
{
  CAACodec *a = (CAACodec *)fs->userdata;
  return (a->peek(data, len));
}

/*
 * Create CAACodec class
 */
CAACodec::CAACodec (CAudioSync *a, 
		    CInByteStreamBase *pbytestrm,
		    format_list_t *media_fmt,
		    audio_info_t *audio,
		    const char *userdata,
		    size_t userdata_size) : 
  CAudioCodecBase(a, pbytestrm, media_fmt, audio, userdata, userdata_size)
{
  m_orig_bytestream = pbytestrm;
  // Start setting up FAAC stuff...
  m_fs = open_filestream_yours(c_read_byte,
			       c_filelength,
			       c_seek,
			       c_close,
			       c_reset,
			       c_peek,
			       this);

  m_resync_with_header = 1;
  m_record_sync_time = 1;

  m_audio_inited = 0;
  // Use media_fmt to indicate that we're streaming.
  // create a CInByteStreamMem that will be used to copy from the
  // streaming packet for use locally.  This will allow us, if we need
  // to skip, to get the next frame.
  if ((media_fmt != NULL) || 
      (audio && audio->stream_has_length)) {
    m_local_bytestream = new CInByteStreamMem;
    m_bytestream = m_local_bytestream;
    m_local_buffersize = 4096;
    m_local_buffer = (char *)malloc(m_local_buffersize);
    // haven't checked for null buffer
    if (media_fmt) 
      m_freq = media_fmt->rtpmap->clock_rate;
    else 
      m_freq = audio->freq;
  } else {
    m_bytestream = m_orig_bytestream;
    m_local_bytestream = NULL;
    m_local_buffersize = 0;
    m_local_buffer = NULL;
    if (audio != NULL) {
      m_freq = audio->freq;
    } else {
      m_freq = 44100;
    }
  }
  player_debug_message("Setting freq to %d", m_freq);
}

CAACodec::~CAACodec()
{
  aac_decode_free();
  if (m_local_bytestream != NULL) {
    delete m_local_bytestream;
    m_local_bytestream = NULL;
  }
  if (m_local_buffer != NULL) {
    free(m_local_buffer);
    m_local_buffer = NULL;
  }
}

/*
 * Handle pause - basically re-init the codec
 */
void CAACodec::do_pause (void)
{
  m_resync_with_header = 1;
  m_record_sync_time = 1;
  m_audio_inited = 0;
  //    player_debug_message("AA got do pause");
}

/*
 * Decode task call for FAAC
 */
int CAACodec::decode (uint64_t rtpts, int from_rtp)
{
  int bits = -1;
  //  struct timezone tz;

  if (m_record_sync_time) {
    m_first_time_offset = 0;
    m_current_frame = 0;
    m_record_sync_time = 0;
    m_current_time = rtpts;
    m_last_rtp_ts = rtpts;
  } else {
    if (m_last_rtp_ts == rtpts) {
      m_current_time += ((1024 * 1000) / m_freq);
      m_current_frame++;
    } else {
      m_last_rtp_ts = rtpts;
      m_current_time = rtpts;
      m_current_frame = 0;
    }

    // Note - here m_current_time should pretty much always be >= rtpts.  
    // If we're not, we most likely want to stop and resync.  We don't
    // need to keep decoding - just decode this frame and indicate we
    // need a resync... That should handle fast forwards...  We need
    // someway to handle reverses - perhaps if we're more than .5 seconds
    // later...
  }
  // player_debug_message("AA at %lld", m_current_time);
  try {
    if (m_local_bytestream) {
      // This means that we have a bytestream that's different from
      // a straight file read.  In the rtp case, we've got a 2 byte
      // length field, then no adts header field.  It does make syncing
      // a bit easier if we lose sync, but we're doing an extra move of
      // the data.
      uint16_t length;
      // read 2 byte length
      length = (m_orig_bytestream->get() & 0xff) << 8;
      length |= (m_orig_bytestream->get() & 0xff);
      if (length == 0) {
	return (0);
      }
      if (length > m_local_buffersize) {
	free(m_local_buffer);
	m_local_buffersize = length * 2;
	m_local_buffer = (char *)malloc(m_local_buffersize);
      }
      /*
       * copy from the original bytestream to local memory
       */
      for (uint16_t ix = 0; ix < length; ix++) {
	m_local_buffer[ix] = m_orig_bytestream->get();
      }
      m_local_bytestream->set_memory(m_local_buffer, length);
    }
  } catch (const char *err) {
#ifdef DEBUG_SYNC
    player_error_message("Got exception %s at %llu", err, m_current_time);
#endif
    m_resync_with_header = 1;
    m_record_sync_time = 1;
    return (bits);
  }

  if (m_audio_inited == 0) {
    /*
     * If not initialized, do so.  Note - will need to change off 
     * 44100 to a real value.
     */
    aac_decode_init_your_filestream(m_fs);
    aac_decode_init(&m_fInfo);
    m_audio_sync->set_config(m_freq, 2, AUDIO_S16MSB, 1024);
    m_audio_inited = 1;
  }

  try {
    short *buff;

    /* 
     * Get an audio buffer
     */
    buff = m_audio_sync->get_audio_buffer();
    if (buff == NULL) {
      //player_debug_message("Can't get buffer in aa");
      return (-1);
    }

    bits = aac_decode_frame(buff);

    if (bits > 0) {
      /*
       * good result - give it to audio sync class
       */
      m_audio_sync->filled_audio_buffer(m_current_time, 
					m_resync_with_header);
      if (m_resync_with_header == 1) {
	m_resync_with_header = 0;
#ifdef DEBUG_SYNC
	player_debug_message("Back to good at %llu", m_current_time);
#endif
      }
    } else {
      player_debug_message("Bits return is %d", bits);
      m_resync_with_header = 1;
#ifdef DEBUG_SYNC
      player_debug_message("Audio decode problem - at %llu", 
			   m_current_time);
#endif
    }
  } catch (const char *err) {
#ifdef DEBUG_SYNC
    player_error_message("Got exception %s at %llu", err, m_current_time);
#endif
    m_resync_with_header = 1;
    m_record_sync_time = 1;
  }
  return (bits);
}

int CAACodec::read_byte(FILE_STREAM *fs, int *err)
{
  if (m_bytestream->eof()) { 
    *err = 1; 
    return -1; 
  }
  return (m_bytestream->get());
}

void CAACodec::reset (void)
{
  m_bytestream->reset();
}

int CAACodec::peek (void *data, int len)
{
  Char *dptr = (Char *)data;
  m_bytestream->bookmark(1);
  for (int ix = 0; ix < len; ix++) {
    *dptr++ = m_bytestream->get();
  }
  m_bytestream->bookmark(0);
  return (len);
}
/* end file aa.cpp */
