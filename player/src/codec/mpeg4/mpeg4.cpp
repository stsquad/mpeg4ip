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
 * mpeg4.cpp - implementation with ISO reference codec
 */

#include <stdio.h>
#include <stdlib.h>
#include "our_bytestream_mem.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WINDOWS
#include <windows.h>
#include <mmsystem.h>
#endif // __PC_COMPILER_

#include <type/typeapi.h>
#include <sys/mode.hpp>
#include <sys/vopses.hpp>
#include <tools/entropy/bitstrm.hpp>
#include <sys/tps_enhcbuf.hpp>
#include <sys/decoder/enhcbufdec.hpp>
#include <sys/decoder/vopsedec.hpp>
#include "mpeg4.h"
#include <player_util.h>
#ifndef __GLOBAL_VAR_
#define __GLOBAL_VAR_
#endif

#include <sys/global.hpp>
static void c_get_more (void *ud, 
			unsigned char **buffer, 
			uint32_t *buflen,
			uint32_t used)
{
  uint32_t ret;

  COurInByteStream *bs = (COurInByteStream *)ud;

  bs->get_more_bytes(buffer, &ret, used, 1);
  *buflen = ret;
}
static void c_just_fail (void *ud, 
			unsigned char **buffer, 
			uint32_t *buflen,
			 uint32_t used)
{
  throw 0;
}

CMpeg4Codec::CMpeg4Codec(CVideoSync *v, 
			 COurInByteStream *pbytestrm, 
			 format_list_t *media_fmt,
			 video_info_t *vinfo,
			 const unsigned char *userdata,
			 uint32_t ud_size) :
  CVideoCodecBase(v, pbytestrm, media_fmt, vinfo, userdata, ud_size)
{
  m_main_short_video_header = FALSE;
  m_bytestream = pbytestrm;
  m_pvodec = new CVideoObjectDecoder();
  m_decodeState = DECODE_STATE_VOL_SEARCH;
  if (media_fmt != NULL && media_fmt->fmt_param != NULL) {
    // See if we can decode a passed in vovod header
    if (parse_vovod(media_fmt->fmt_param, 1, 0) == 1) {
      m_decodeState = DECODE_STATE_WAIT_I;
    }
  } else if (userdata != NULL) {
    if (parse_vovod((const char *)userdata, 0, ud_size) == 1) {
      m_decodeState = DECODE_STATE_WAIT_I;
    }
  } else if (vinfo != NULL) {
    m_pvodec->FakeOutVOVOLHead(vinfo->height,
			       vinfo->width,
			       vinfo->frame_rate,
			       &m_bSpatialScalability);
    m_video_sync->config(vinfo->width,
			 vinfo->height,
			 vinfo->frame_rate);

    m_decodeState = DECODE_STATE_NORMAL;
  } 

  m_num_wait_i = 0;
  m_num_wait_i_frames = 0;
  m_total_frames = 0;
}


CMpeg4Codec::~CMpeg4Codec()
{
  player_debug_message("MPEG4 codec results:");
  player_debug_message("total frames    : %u", m_total_frames);
  player_debug_message("wait for I times: %u", m_num_wait_i);
  player_debug_message("wait I frames   : %u", m_num_wait_i_frames);
  delete m_pvodec;

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
int CMpeg4Codec::parse_vovod (const char *vovod,
			      int ascii,
			      uint32_t len)
{
  unsigned char buffer[255];
  unsigned char *bufptr;

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
    bufptr = (unsigned char *)&buffer[0];
  } else {
    bufptr = (unsigned char *)vovod;
  }


  // Create a byte stream to take from our buffer.
  // Temporary set of our bytestream
  // Get the VOL header.  If we fail, set the bytestream back
  int havevol = 0;
  do {
    try {
      m_pvodec->SetUpBitstreamBuffer(c_just_fail, NULL,
				     bufptr, len);
      m_pvodec->decodeVOLHead();
      m_pvodec->postVO_VOLHeadInit(m_pvodec->getWidth(),
				   m_pvodec->getHeight(),
				   &m_bSpatialScalability);
      player_debug_message("Found VOL in header");
	
      m_video_sync->config(m_pvodec->getWidth(),
			   m_pvodec->getHeight(),
			   m_pvodec->getClockRate());
      havevol = 1;
    } catch (int err) {
      player_debug_message("Caught exception in VOL mem header search");
    }
    uint32_t used;
    used = m_pvodec->get_used_bytes();
    bufptr += used;
    len -= used;
  } while (havevol == 0 && len > 0);


  // We've found the VO VOL header - that's good.
  // Reset the byte stream back to what it was, delete our temp stream
  //player_debug_message("Decoded vovod header correctly");
  return havevol;
}

void CMpeg4Codec::do_pause (void)
{
  m_decodeState = DECODE_STATE_WAIT_I;
}

void CMpeg4Codec::error_return (uint32_t buflen) 
{
	int bytes = m_pvodec->get_used_bytes();
	if (bytes == 0) 
		m_bytestream->used_bytes_for_frame(buflen);
	else
		m_bytestream->used_bytes_for_frame(bytes);
}

int CMpeg4Codec::decode (uint64_t ts, 
			 int from_rtp, 
			 unsigned char *buffer,
			 uint32_t buflen)
{
  Int iEof = 1;
  m_total_frames++;
  m_pvodec->SetUpBitstreamBuffer(c_get_more, 
				 m_bytestream, 
				 buffer, 
				 buflen);
  switch (m_decodeState) {
  case DECODE_STATE_VOL_SEARCH:
    try {
      m_pvodec->decodeVOLHead();
      m_pvodec->postVO_VOLHeadInit(m_pvodec->getWidth(),
				   m_pvodec->getHeight(),
				   &m_bSpatialScalability);
      //player_debug_message("Found VOL");
	
      m_video_sync->config(m_pvodec->getWidth(),
			   m_pvodec->getHeight(),
			   m_pvodec->getClockRate());

      m_decodeState = DECODE_STATE_WAIT_I;
    } catch (int err) {
      player_debug_message("Caught exception in VOL search %s", 
			   m_bytestream->get_throw_error(err));
	  error_return(buflen);
      return (-1);
    }
    //      return(0);
  case DECODE_STATE_WAIT_I:
    try {
      iEof = m_pvodec->decode(NULL, TRUE);
      if (iEof == -1) {
	m_num_wait_i_frames++;
	m_bytestream->used_bytes_for_frame(m_pvodec->get_used_bytes());
	return(0);
      }
      m_decodeState = DECODE_STATE_NORMAL;
      player_debug_message("Back to normal state");
      m_bCachedRefFrame = FALSE;
      m_bCachedRefFrameCoded = FALSE;
      m_cached_valid = FALSE;
      m_cached_time = 0;
    } catch (int err) { //(const char *err) {
#if 1
      //if (m_bytestream->throw_error_minor(err) != 0) {
	player_debug_message("Caught exception in WAIT_I %d", err);
	//}
#endif
	error_return(buflen);
      return (-1);
    }
    break;
  case DECODE_STATE_NORMAL:
    try {
      iEof = m_pvodec->decode(NULL, FALSE, FALSE);
    } catch (int err) {
      // This is because sometimes, the encoder doesn't read all the bytes
      // it should out of the rtp packet.  The rtp bytestream does a read
      // and determines that we're trying to read across bytestreams.
      // If we get this, we don't want to change anything - just fall up
      // to the decoder thread so it gives us a new timestamp.
		error_return(buflen);
      if (m_bytestream->throw_error_minor(err) != 0) {
	//player_debug_message("decode across ts");
		 
	return (-1);
      }
      player_debug_message("Mpeg4 ncaught %s -> waiting for I", 
			   m_bytestream->get_throw_error(err));
      m_decodeState = DECODE_STATE_WAIT_I;
      return (-1);
    } catch (...) {
		error_return(buflen);
	m_decodeState = DECODE_STATE_WAIT_I;
	return (-1);
    }
    break;
  }

  m_bytestream->used_bytes_for_frame(m_pvodec->get_used_bytes());
  /*
   * We've got a good frame.  See if we need to display it
   */
  const CVOPU8YUVBA *pvopcQuant = NULL;
  if (m_pvodec->fSptUsage() == 1) {
    player_debug_message("Sprite");
  }
  uint64_t displaytime = 0;
  int cached_ts = 0;
  if (iEof == EOF) {
    if (m_bCachedRefFrame) {
      m_bCachedRefFrame = FALSE;
      if (m_bCachedRefFrameCoded) {
	pvopcQuant = m_pvodec->pvopcRefQLater();
	displaytime = ts;
      }
    }
  } else {
    if (m_pvodec->vopmd().vopPredType == BVOP) {
      if (iEof != FALSE) {
	pvopcQuant = m_pvodec->pvopcReconCurr();
	displaytime = ts;
      } 
    } else {
      if (m_bCachedRefFrame) {
	m_bCachedRefFrame = FALSE;
	if (m_bCachedRefFrameCoded) {
	  pvopcQuant = m_pvodec->pvopcRefQPrev();
	  if (from_rtp) {
	    int old_was_valid = m_cached_valid;
	    displaytime = m_cached_time;
	    cached_ts = 1;
	    // old time stamp wasn't valid - instead of calculating it
	    // ourselves, just punt on it.
	    if (old_was_valid == 0) {
	      return (iEof == EOF ? -1 : 0);
	    }
	  } else {
	    displaytime = ts;
	  }
	}
      }

      m_cached_time = ts;
      m_cached_valid = TRUE;
      m_bCachedRefFrame = TRUE;
      m_bCachedRefFrameCoded = (iEof != FALSE);
    }
  }

  if (pvopcQuant != NULL) {
#if 0
    player_debug_message("frame rtp_ts "LLU" disp "LLU" cached %d", 
			 ts, displaytime, cached_ts);
#endif
    /*
     * Get the information to the video sync structure
     */
    const Uint8 *y, *u, *v;
    int pixelw_y, pixelw_uv;
    pixelw_y =  pvopcQuant->getPlane(Y_PLANE)->where().width;
    pixelw_uv = pvopcQuant->getPlane(U_PLANE)->where().width;

    y = pvopcQuant->getPlane(Y_PLANE)->pixels(0,0);
    u = pvopcQuant->getPlane(U_PLANE)->pixels(0,0);
    v = pvopcQuant->getPlane(V_PLANE)->pixels(0,0);
    m_last_time = displaytime;
#if 0
    player_debug_message("Adding video at "LLU" %d", displaytime,
			 m_pvodec->vopmd().vopPredType);
#endif

    uint64_t rettime;
    m_video_sync->set_video_frame(y, 
				    u, 
				    v, 
				    pixelw_y, 
				    pixelw_uv, 
				    displaytime, rettime);
  }
  return (iEof == EOF ? -1 : 1);
}


int CMpeg4Codec::skip_frame (uint64_t ts, 
			     unsigned char *buffer, 
			     uint32_t buflen) 
{
  return (decode(ts, 0, buffer, buflen));
}
