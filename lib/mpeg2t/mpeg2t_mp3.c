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
 *		Bill May (wmay@cisco.com)
 */
/* mpeg2t_mp3.c - process mpeg audio frames */

#include "mpeg4ip.h"
#include "mpeg2_transport.h"
#include <assert.h>
#include "mp4av.h"
#include "mpeg2t_private.h"

/*
 * mpeg2t_find_mp3_frame_start - look through the buffer and find
 * the mpeg audio header
 */
static uint32_t mpeg2t_find_mp3_frame_start (mpeg2t_es_t *es_pid, 
					     const uint8_t *esptr, 
					     uint32_t buflen,
					     int *ret)
{
  const uint8_t *fptr;
  int found = 0;
  int dropped = 0;
  int offset, tocopy;
  uint32_t framesize;

  if (es_pid->left != 0) {
    // Indicates that we have up to 3 bytes left from previous frame
    // copy so we have 4 bytes in the buffer, then see if it matches.
    memcpy(es_pid->left_buff + es_pid->left,
	   esptr, 
	   3);
    found = MP4AV_Mp3GetNextFrame(es_pid->left_buff, 4, &fptr, 
				  &framesize, FALSE, TRUE);
    if (found) {
      dropped = fptr - (const uint8_t *)&es_pid->left_buff[0];
#if 0
      mpeg2t_message("MP3 - Found in left - %d dropped %d", es_pid->left, dropped);
#endif
    } else {
      es_pid->left = 0;
    }
  }

  if (found == 0) {
    // Not found with leftover bytes - see if it's in the buffer
    found = MP4AV_Mp3GetNextFrame(esptr, buflen, &fptr, &framesize, 
				  FALSE, TRUE);
    if (found == 0) {
      memcpy(es_pid->left_buff,
	     esptr + buflen - 3, 
	     3);
      es_pid->left = 3;
      return buflen;
    }
    dropped = fptr - esptr;
  }

  if (found) {
    // We've found the header - load up the info if we haven't already
    if (es_pid->info_loaded == 0) {
      MP4AV_Mp3Header hdr;
      hdr = MP4AV_Mp3HeaderFromBytes(fptr);
      es_pid->audio_chans = MP4AV_Mp3GetChannels(hdr);
      es_pid->sample_freq = MP4AV_Mp3GetHdrSamplingRate(hdr);
      es_pid->sample_per_frame = MP4AV_Mp3GetHdrSamplingWindow(hdr);
      es_pid->bitrate = MP4AV_Mp3GetBitRate(hdr) * 1000.0;
      es_pid->mpeg_layer = MP4AV_Mp3GetHdrLayer(hdr);
      mpeg2t_message(LOG_NOTICE, "MP3 - chans %d freq %d spf %d", 
		     es_pid->audio_chans, 
		     es_pid->sample_freq,
		     es_pid->sample_per_frame);
      es_pid->info_loaded = 1;
      *ret = 1;
    }
    // We know how big the frame will be, so malloc it
    mpeg2t_malloc_es_work(es_pid, framesize);
    if (es_pid->work == NULL) return buflen;
      
    offset = 0;

    // Copy all the data we can to the buffer
    if (es_pid->left) {
      offset = es_pid->left - dropped;
      memcpy(es_pid->work->frame, es_pid->left_buff + dropped, offset);
      es_pid->work_loaded = offset;
      es_pid->left = 0;
      dropped = 0;
    }

    tocopy = MIN(buflen - dropped, framesize);
    memcpy(es_pid->work->frame + offset, 
	   esptr + dropped,
	   tocopy);
    es_pid->work_loaded += tocopy;
#if 0
    printf("start framesize %d copied %d\n", es_pid->work->frame_len,
	   es_pid->work_loaded);
#endif
    return tocopy + dropped;
  }
  return buflen;
}

int process_mpeg2t_mpeg_audio (mpeg2t_es_t *es_pid, 
			       const uint8_t *esptr, 
			       uint32_t buflen)
{
  uint32_t used;
  int ret;
  uint32_t tocopy;

  if ((es_pid->peshdr_loaded != 0) && ((es_pid->stream_id & 0xe0) != 0xc0)) {
    mpeg2t_message(LOG_ERR, 
		   "Illegal stream id %x in mpeg audio stream - PID %x",
		   es_pid->stream_id, es_pid->pid.pid);
    return -1;
  }
  ret = 0;
  while (buflen > 0) {
    if (es_pid->work == NULL) {
      // we haven't found the header - look for it.
      if (buflen < 4) {
	memcpy(es_pid->left_buff, esptr, buflen);
	es_pid->left = buflen;
	return ret;
      }

      used = mpeg2t_find_mp3_frame_start(es_pid, esptr, buflen, &ret);

      esptr += buflen;
      buflen -= used;
    } else {
      // We've got the header - keep going until we've got the frame loaded
      tocopy = MIN(buflen, (es_pid->work->frame_len - es_pid->work_loaded));
      memcpy(es_pid->work->frame + es_pid->work_loaded, esptr, tocopy);
      buflen -= tocopy;
      es_pid->work_loaded += tocopy;
      esptr += tocopy;
#if 0
      printf("added %d bytes to %d\n", tocopy, es_pid->work_loaded);
#endif
    }
    if (es_pid->work != NULL &&
	es_pid->work_loaded == es_pid->work->frame_len) {

      mpeg2t_finished_es_work(es_pid, es_pid->work_loaded);
      ret = 1;
    }
  }
  return ret;
}

int mpeg2t_mpeg_audio_info (mpeg2t_es_t *es_pid, char *buffer, size_t len)
{
  int rate;
  if (es_pid->info_loaded == 0) return -1;
  rate = es_pid->bitrate / 1000;
  snprintf(buffer, len, "MPEG Audio layer %d, %d kbps, %d %s", 
	   es_pid->mpeg_layer, rate, es_pid->sample_freq, 
	   es_pid->audio_chans == 1 ? "mono" : "stereo");
  return 0;
}
	   
/* mpeg2t_mp3.c */
