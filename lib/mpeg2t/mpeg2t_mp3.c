
#include "mpeg4ip.h"
#include "mpeg2_transport.h"
#include <assert.h>
#include "mp4av.h"
#include "mpeg2t_private.h"

static uint32_t mpeg2t_find_mp3_frame_start (mpeg2t_es_t *es_pid, 
					     const uint8_t *esptr, 
					     uint32_t buflen)
{
  const uint8_t *fptr;
  int found = 0;
  int dropped = 0;
  int offset, tocopy;
  uint32_t framesize;

  if (es_pid->left != 0) {
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
    mpeg2t_malloc_es_work(es_pid, framesize);
    if (es_pid->work == NULL) return buflen;
      
    offset = 0;
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

  if ((es_pid->stream_id & 0xe0) != 0xc0) {
    mpeg2t_message(LOG_ERR, 
		   "Illegal stream id %x in mpeg audio stream - PID %x",
		   es_pid->stream_id, es_pid->pid.pid);
    return -1;
  }
  ret = 0;
  while (buflen > 0) {
    if (es_pid->work == NULL) {
      if (buflen < 4) {
	memcpy(es_pid->left_buff, esptr, buflen);
	es_pid->left = buflen;
	return ret;
      }

      used = mpeg2t_find_mp3_frame_start(es_pid, esptr, buflen);

      esptr += buflen;
      buflen -= used;
    } else {
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

