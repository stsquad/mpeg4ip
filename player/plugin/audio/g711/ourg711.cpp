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
#include "ourg711.h"
#include "mp4av.h"
#include <mp4v2/mp4.h>
#define LOGIT g711->m_vft->log_msg
static int16_t alaw2linear(uint8_t a_val);
static int16_t ulaw2linear(uint8_t u_val);
/*
 * Create raw audio structure
 */
static codec_data_t *g711_codec_create (const char *stream_type,
					const char *compressor,
					int type, 
					int profile, 
					format_list_t *fptr,
					audio_info_t *audio,
					const uint8_t *userdata,
					uint32_t userdata_size,
					audio_vft_t *vft,
					void *ifptr)

{
  g711_codec_t *g711;

  g711 = (g711_codec_t *)malloc(sizeof(g711_codec_t));
  memset(g711, 0, sizeof(g711_codec_t));

  g711->m_vft = vft;
  g711->m_ifptr = ifptr;
  g711->m_initialized = 0;
  g711->m_temp_buff = NULL;
  g711->m_freq = 8000;
  g711->m_chans = 1;
  g711->m_bitsperchan = 16; // convert 8 to 16
  if (fptr != NULL) {
    g711->m_alaw = (strcmp(fptr->fmt, "8") == 0) ? 1 : 0;
  } else {
    if ((strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) &&
	(strcasecmp(compressor, "mp4a") == 0)) {
      if (type == MP4_ALAW_AUDIO_TYPE) {
	g711->m_alaw = 1;
      } else if (type == MP4_ULAW_AUDIO_TYPE) {
	g711->m_alaw = 0;
      } else {
	free(g711);
	return NULL;
      }
    } else if (strcasecmp(compressor, "ulaw") == 0) {
      g711->m_alaw = 0;
    } else if (strcasecmp(compressor, "alaw") == 0) {
      g711->m_alaw = 1;
    } else {
      free(g711);
      return NULL;
    }
  } 

  LOGIT(LOG_DEBUG, "g711", 
	"setting freq %d chans %d bitsper %d", 
	g711->m_freq,
	g711->m_chans, 
	g711->m_bitsperchan);

  return (codec_data_t *)g711;
}

static void g711_close (codec_data_t *ptr)
{
  g711_codec_t *g711 = (g711_codec_t *)ptr;
  if (g711->m_temp_buff != NULL) free(g711->m_temp_buff);
  free(g711);
}

/*
 * Handle pause - basically re-init the codec
 */
static void g711_do_pause (codec_data_t *ifptr)
{

}

/*
 * Decode task call for FAAC
 */
static int g711_decode (codec_data_t *ptr,
			frame_timestamp_t *pts,
			int from_rtp,
			int *sync_frame,
			uint8_t *buffer,
			uint32_t buflen,
			void *ud)
{
  g711_codec_t *g711 = (g711_codec_t *)ptr;
  uint32_t freq_ts = pts->audio_freq_timestamp;

  if (pts->audio_freq != 8000) {
    freq_ts = convert_timescale(freq_ts, pts->audio_freq, 8000);
  }

  //LOGIT(LOG_DEBUG, "g711", "ts %llu buffer len %d", ts, buflen);
  if (g711->m_initialized == 0) {
    g711->m_vft->audio_configure(g711->m_ifptr,
				 g711->m_freq, 
				 g711->m_chans, 
				 AUDIO_FMT_S16,
				 0);
    g711->m_initialized = 1;
  }
  if (g711->m_temp_buffsize < buflen * 2) {
    g711->m_temp_buff = (uint8_t *)realloc(g711->m_temp_buff, buflen * 2);
    g711->m_temp_buffsize = buflen * 2;
  } 

  int16_t *outbuf = (int16_t *)g711->m_temp_buff;
  for (uint32_t ix = 0; ix < buflen; ix++) {
    if (g711->m_alaw) {
      outbuf[ix] = alaw2linear(buffer[ix]);
    } else {
      outbuf[ix] = ulaw2linear(buffer[ix]);
    }
  }

  g711->m_vft->audio_load_buffer(g711->m_ifptr, 
				 g711->m_temp_buff, 
				 buflen * 2,
				 freq_ts,
				 pts->msec_timestamp);

  return (buflen);
}


static int g711_codec_check (lib_message_func_t message,
			     const char *stream_type,
			    const char *compressor,
			    int type,
			    int profile,
			    format_list_t *fptr, 
			    const uint8_t *userdata,
			    uint32_t userdata_size,
			     CConfigSet *pConfig)
{
  if ((strcasecmp(stream_type, STREAM_TYPE_MP4_FILE) == 0) &&
      compressor != NULL &&
      (strcasecmp(compressor, "mp4a") == 0)) {
    // private type
    if (type == MP4_ALAW_AUDIO_TYPE ||
	type == MP4_ULAW_AUDIO_TYPE) {
      return 1;
    }
  }
  if (strcasecmp(stream_type, STREAM_TYPE_RTP) == 0 && 
      fptr != NULL) {
    if (strcmp(fptr->fmt, "8") == 0 ||
	strcmp(fptr->fmt, "0") == 0) {
      return 1;
    }
  }
  if (compressor && 
      ((strcasecmp(compressor, "ulaw") == 0) ||
       (strcasecmp(compressor, "alaw") == 0))) {
    return 1;
  }
  return -1;
}

codec_data_t *g711_file_check (lib_message_func_t message,
			      const char *name, 
			      double *max,
			      char *desc[4],
			       CConfigSet *pConfig)
{
  int len;
  g711_codec_t *g711;

  len = strlen(name);
  if (strcasecmp(name + len - 5, ".g711") != 0) {
    return (NULL);
  }
  g711 = MALLOC_STRUCTURE(g711_codec_t);
  memset(g711, 0, sizeof(*g711));
 
  *max = 0;
  g711->m_buffer = (uint8_t *)(malloc(8000));
  g711->m_buffer_size = 8000;
  g711->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (g711->m_ifile == NULL) {
    free(g711);
    return NULL;
  }

  g711->m_freq = 8000;
  g711->m_chans = 1;
  g711->m_bitsperchan = 16;
  g711->m_alaw = 0;
  return (codec_data_t *)g711;
}

int g711_file_next_frame (codec_data_t *your, 
			  uint8_t **buffer, 
			  frame_timestamp_t *ts)
{
  g711_codec_t *g711 = (g711_codec_t *)your;

  if (g711->m_buffer_on > 0) {
    memmove(g711->m_buffer, 
	    &g711->m_buffer[g711->m_buffer_on],
	    g711->m_buffer_size - g711->m_buffer_on);
  }
  g711->m_buffer_size -= g711->m_buffer_on;
  g711->m_buffer_size += fread(g711->m_buffer + g711->m_buffer_size, 
			      1, 
			      8000 - g711->m_buffer_size,
			      g711->m_ifile);
  g711->m_buffer_on = 0;
  if (g711->m_buffer_size == 0) return 0;
  uint64_t calc;
  calc = g711->m_bytecount * M_64;
  calc /= g711->m_freq;
  ts->msec_timestamp = calc;
  ts->audio_freq_timestamp = g711->m_bytecount;
  ts->audio_freq = 8000;
  ts->timestamp_is_pts = false;
  *buffer = g711->m_buffer;
  return (g711->m_buffer_size);
}

	
void g711_file_used_for_frame (codec_data_t *ifptr, 
			       uint32_t bytes)
{
  g711_codec_t *g711 = (g711_codec_t *)ifptr;
  g711->m_buffer_on += MAX(bytes, 8000);
  g711->m_bytecount += bytes;  // increment time by # of bytes serviced
  if (g711->m_buffer_on > g711->m_buffer_size) 
    g711->m_buffer_on = g711->m_buffer_size;
}

int g711_file_eof (codec_data_t *ifptr)
{
  g711_codec_t *g711 = (g711_codec_t *)ifptr;
  return g711->m_buffer_on == g711->m_buffer_size && feof(g711->m_ifile);
}

int g711_raw_file_seek_to (codec_data_t *ifptr, uint64_t ts)
{
  if (ts != 0) return -1;

  g711_codec_t *g711 = (g711_codec_t *)ifptr;
  rewind(g711->m_ifile);
  g711->m_buffer_size = g711->m_buffer_on = 0;
  g711->m_bytecount = 0;
  return 0;
}

AUDIO_CODEC_WITH_RAW_FILE_PLUGIN("g711",
				 g711_codec_create,
				 g711_do_pause,
				 g711_decode,
				 NULL,
				 g711_close,
				 g711_codec_check,
				 g711_file_check,
				 g711_file_next_frame,
				 g711_file_used_for_frame,
				 g711_raw_file_seek_to,
				 g711_file_eof,
				 NULL, 
				 0);
/* end file g711.cpp */


/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * g711.c
 *
 * u-law, A-law and linear PCM conversions.
 */
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
static int16_t alaw2linear(uint8_t a_val)
{
    int16_t	t;
    uint8_t seg;

    a_val ^= 0x55;
    t = (a_val & QUANT_MASK) << 4;
    seg = (a_val & SEG_MASK) >> SEG_SHIFT;

    
    switch (seg)
    {
        case 0:
        t += 8;
        break;
        case 1:
        t += 0x108;
        break;
        default:
        t += 0x108;
        t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}

#define	BIAS		(0x84)		/* Bias for linear code. */

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
static int16_t ulaw2linear(uint8_t u_val)
{
    int16_t	t;

    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
     * Extract and bias the quantization bits. Then
     * shift up by the segment number and subtract out the bias.
     */
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= (u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

