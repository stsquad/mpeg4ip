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
 * divx.cpp - implementation with ISO reference codec
 */

#include <stdio.h>
#include <stdlib.h>
#include "our_bytestream_mem.h"

#include "divx.h"
#include "player_util.h"
#include "divxif.h"


static unsigned int c_get (void *ud)
{
  unsigned int ret;
  CDivxCodec *d = (CDivxCodec *)ud;
  ret = d->get();
  return (ret);
}

static void c_bookmark (void *ud, int val)
{
  CDivxCodec *d = (CDivxCodec *)ud;
  d->bookmark(val);
}

CDivxCodec::CDivxCodec(CVideoSync *v, 
		       CInByteStreamBase *pbytestrm, 
		       format_list_t *media_fmt,
		       video_info_t *vinfo,
		       const unsigned char *userdata,
		       uint32_t ud_size) :
  CVideoCodecBase(v, pbytestrm, media_fmt, vinfo, userdata, ud_size)
{
  m_bytestream = pbytestrm;
  m_last_time = 0;
  newdec_init(c_get, c_bookmark, this);
  m_decodeState = DIVX_STATE_VO_SEARCH;
  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    // See if we can decode a passed in vovod header
    if (parse_vovod(media_fmt->fmt_param, 1, 0) == 1) {
      m_decodeState = DIVX_STATE_WAIT_I;
    }
  } else if (userdata != NULL) {
    if (parse_vovod((const char *)userdata, 0, ud_size) == 1) {
      m_decodeState = DIVX_STATE_WAIT_I;
    }
  } else if (vinfo != NULL && vinfo->file_has_vol_header == 0) {
    mp4_hdr.width = vinfo->width;
    mp4_hdr.height = vinfo->height;
    
    // defaults from mp4_decoder.c routine dec_init...
    mp4_hdr.quant_precision = 5;
    mp4_hdr.bits_per_pixel = 8;

    mp4_hdr.quant_type = 0;

    mp4_hdr.time_increment_resolution = 15;
    mp4_hdr.time_increment_bits = 0;
    while (mp4_hdr.time_increment_resolution > 
	   (1 << mp4_hdr.time_increment_bits)) {
      mp4_hdr.time_increment_bits++;
    }
    mp4_hdr.fps = 30;

    mp4_hdr.complexity_estimation_disable = 1;

    m_video_sync->config(vinfo->width,
			 vinfo->height,
			 vinfo->frame_rate);
    post_volprocessing();
    m_decodeState = DIVX_STATE_NORMAL;
  } 

  m_dropped_b_frames = 0;
  m_num_wait_i = 0;
  m_num_wait_i_frames = 0;
  m_total_frames = 0;
}


CDivxCodec::~CDivxCodec()
{
  divx_message(LOG_NOTICE, "Divx codec results:");
  divx_message(LOG_NOTICE, "total frames    : %u", m_total_frames);
  divx_message(LOG_NOTICE, "dropped b frames: %u", m_dropped_b_frames);
  divx_message(LOG_NOTICE, "wait for I times: %u", m_num_wait_i);
  divx_message(LOG_NOTICE, "wait I frames   : %u", m_num_wait_i_frames);

}

// Convert a hex character to it's decimal value.
static char tohex (char a)
{ 
  if (isdigit(a))
    return (a - '0');
  return (tolower(a) - 'a' + 10);
}

// Parse the format config passed in.  This is the vo vod header
// that we need to get width/height/frame rate
int CDivxCodec::parse_vovod (const char *vovod,
			     int ascii,
			     uint32_t len)
{
  unsigned char buffer[255];
  const char *bufptr;
  COurInByteStreamMem *membytestream;
  int ret;

  if (ascii == 1) {
    const char *config = strcasestr(vovod, "config=");
    if (config == NULL) {
      return 0;
    }
    config += strlen("config=");
    const char *end;
    end = config;
    while (isxdigit(*end)) end++;
    if (config == end) {
      return 0;
    }
    // config string will run from config to end
    len = end - config;
    // make sure we have even number of digits to convert to binary
    if ((len % 2) == 1) 
      return 0;
    unsigned char *write;
    write = buffer;
    // Convert the config= from ascii to binary
    for (uint32_t ix = 0; ix < len; ix++) {
      *write = 0;
      *write = (tohex(*config)) << 4;
      config++;
      *write += tohex(*config);
      config++;
      write++;
    }
    len /= 2;
    bufptr = (const char *)buffer;
  } else {
    bufptr = vovod;
  }
  
  membytestream = new COurInByteStreamMem(bufptr, len);
  // Temporary set of our bytestream
  CInByteStreamBase *orig_bytestream;
  orig_bytestream = m_bytestream;
  m_bytestream = membytestream;

  // Get the VO/VOL header.  If we fail, set the bytestream back
  do {
    ret = 0;
    try {
      ret = getvolhdr();
      if (ret != 0) {
	divx_message(LOG_DEBUG, "Found VOL in header");
	m_video_sync->config(mp4_hdr.width,
			     mp4_hdr.height,
			     mp4_hdr.time_increment_resolution);
	post_volprocessing();
      }

    } catch (int err) {
      divx_message(LOG_INFO, "Caught exception in VO mem header search %s", 
		   membytestream->get_throw_error(err));
    }
  } while (ret == 0 && membytestream->eof() == 0);

  if (ret == 0) {
    m_bytestream = orig_bytestream;
    delete membytestream;
    return (0);
  }

  // We've found the VO VOL header - that's good.
  // Reset the byte stream back to what it was, delete our temp stream
  m_bytestream = orig_bytestream;
  delete membytestream;
  //player_debug_message("Decoded vovod header correctly");
  return ret;
}

void CDivxCodec::do_pause (void)
{
  m_decodeState = DIVX_STATE_WAIT_I;
}

int CDivxCodec::decode (uint64_t ts, int from_rtp)
{
  int ret;
  int do_wait_i = 0;

  m_total_frames++;
  
  switch (m_decodeState) {
  case DIVX_STATE_VO_SEARCH:
    try {
      ret = getvolhdr();
      if (ret != 0) {
	m_video_sync->config(mp4_hdr.width,
			     mp4_hdr.height,
			     mp4_hdr.fps);
	post_volprocessing();
	m_decodeState = DIVX_STATE_WAIT_I;
      } else {
	return (-1);
      }
    } catch (int err) {
      divx_message(LOG_INFO, "Caught exception in VO search %s", 
		   m_bytestream->get_throw_error(err));
      return (-1);
    }
    //      return(0);
  case DIVX_STATE_WAIT_I:
    do_wait_i = 1;
    break;
  case DIVX_STATE_NORMAL:
    break;
  }
  
  unsigned char *y, *u, *v;
  ret = m_video_sync->get_video_buffer(&y, &u, &v);
  if (ret == 0) {
    return (-1);
  }

  try {
    ret = newdec_frame(y, v, u, do_wait_i); // will eventually need to pass wait_i, etc

    if (ret == 0) {
      //player_debug_message("ret from newdec_frame %d "LLU, ret, ts);
      if (m_last_time != ts)
	m_decodeState = DIVX_STATE_WAIT_I;
      return (-1);
    }
    //player_debug_message("frame type %d", mp4_hdr.prediction_type);
    m_decodeState = DIVX_STATE_NORMAL;
    m_last_time = ts;
  } catch (int err) {
      if (m_bytestream->throw_error_minor(err) != 0) {
	//player_debug_message("decode across ts");
	return (-1);
      }
      divx_message(LOG_DEBUG, "divx caught %s", 
		   m_bytestream->get_throw_error(err));
      m_decodeState = DIVX_STATE_WAIT_I;
      return (-1);
  } catch (...) {
    divx_message(LOG_DEBUG, "divx caught exception");
    m_decodeState = DIVX_STATE_WAIT_I;
    return (-1);
  }

  uint64_t rettime;
  ret = m_video_sync->filled_video_buffers(ts, rettime);
  // disptime is time we've decoded.  Ret is time last buffer was played
  // at.  If we fall behind, we can do 2 things - either nothing  < 3 frames
  // worth - drop B's - up to 10 frames worth, or resync to the next I frame
  if (ret != 0) {
    //    uint64_t msec;
    //player_debug_message("Processed frame "LLU, ts);
#if 0
    msec = m_video_sync->get_video_msec_per_frame();
    if (msec > 0 && 
	((ts + msec) < rettime)) {
      player_debug_message("Out of sync - waiting for I "LLU " "LLU,
			   ts, rettime);
      m_decodeState = DIVX_STATE_WAIT_I;
      m_num_wait_i++;
    } else {
      m_dropFrame = FALSE;
    }
#endif
    return (1);
  }

      
  return (-1);
}

int CDivxCodec::skip_frame (uint64_t ts)
{
  return (decode(ts, 0));
}

unsigned char CDivxCodec::get (void)
{ 
  return m_bytestream->get(); 
}

void CDivxCodec::bookmark (int val)
{
  m_bytestream->bookmark(val);
}
