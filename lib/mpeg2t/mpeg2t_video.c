#include "mpeg4ip.h"
#include "mpeg2_transport.h"
#include "mpeg2t_private.h"
#include "mp4av.h"

#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_SEQUENCE_END_CODE          0x000001b7


int process_mpeg2t_mpeg_video (mpeg2t_es_t *es_pid, 
			       const uint8_t *esptr, 
			       uint32_t buflen)
{
  int framesfinished = 0;

  if ((es_pid->stream_id & 0xf0) != 0xe0) {
    mpeg2t_message(LOG_ERR, "Video stream PID %x with bad stream_id %x", 
		   es_pid->pid.pid,
		   es_pid->stream_id);
    return 0;
  }

  while (buflen > 0) {
    es_pid->header <<= 8;
    es_pid->header |= *esptr;

    if (es_pid->work_state == 0) {
      /*
       * Work state 0 - looking for any header
       */
      if ((es_pid->header & 0xffffff00) == 0x00000100) {
	// have first header.
	if (es_pid->work == NULL) {
	  if (es_pid->work_max_size < 4096) es_pid->work_max_size = 4096;

	  mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	  if (es_pid->work == NULL) return framesfinished;
	}
	es_pid->work->frame[0] = 0;
	es_pid->work->frame[1] = 0;
	es_pid->work->frame[2] = 1;
	es_pid->work->frame[3] = *esptr;
	es_pid->work_loaded = 4;
	if (es_pid->header == MPEG3_PICTURE_START_CODE) {
	  es_pid->work_state = 2;
	  es_pid->pict_header_offset = 0;
	} else 
	  es_pid->work_state = 1;
#if 0
	printf("video - state 0 header %x state %d\n", es_pid->header, 
	       es_pid->work_state);
#endif
      } 
      buflen--;
      esptr++;
    } else {
      /*
       * All other work states - load the current byte into the
       * buffer - reallocate buffer if needed
       */
      if (es_pid->work_loaded >= es_pid->work_max_size - 1) {
	uint8_t *frameptr;
	es_pid->work_max_size += 1024;
	frameptr = 
	  (uint8_t *)realloc(es_pid->work,
			     sizeof(mpeg2t_frame_t) + 
			     es_pid->work_max_size);
#if 0
	printf("Es pid work reallocing to %d\n", es_pid->work_max_size);
#endif
	if (frameptr == NULL) {
	  es_pid->work = NULL;
	  es_pid->work_state = 0;
	  es_pid->header = 0;
	  buflen--;
	  esptr++;
	  break;
	} else {
	  es_pid->work = (mpeg2t_frame_t *)frameptr;
	  frameptr += sizeof(mpeg2t_frame_t);
	  es_pid->work->frame = frameptr;
	}
      }
	
      es_pid->work->frame[es_pid->work_loaded] = *esptr;
      es_pid->work_loaded++;
      if (es_pid->work_state == 1) {
	/*
	 * Work State 1 - looking for first picture start code
	 */
	// Looking for first picture start code
	if (es_pid->header == MPEG3_PICTURE_START_CODE) {
	  es_pid->pict_header_offset = es_pid->work_loaded - 4;
	  es_pid->work_state = 2;
#if 0
	  printf("Now at work state 2 - len %d\n", es_pid->work_loaded);
#endif
	}
      } else {
	/*
	 * Work state 2 - have picture start code in buffer - looking for
	 * next picture start code
	 */
	// Might want to enhance this to stop at GOP, also
	if (es_pid->header == MPEG3_PICTURE_START_CODE ||
	    es_pid->header == MPEG3_SEQUENCE_START_CODE ||
	    es_pid->header == MPEG3_GOP_START_CODE ||
	    es_pid->header == MPEG3_SEQUENCE_END_CODE) {
	  framesfinished = 1;
	  if (es_pid->have_seq_header) {
	    uint32_t h, w;
	    double frame_rate;

	    if (MP4AV_Mpeg3ParseSeqHdr(es_pid->work->frame + es_pid->seq_header_offset,
				       es_pid->work_loaded - es_pid->seq_header_offset,
				       &h, 
				       &w, 
				       &frame_rate) >= 0) {
	      mpeg2t_message(LOG_DEBUG, "Found seq header - h %d w %d fr %g", 
			     h, w, frame_rate);
	    }
	  }

	  mpeg2t_message(LOG_CRIT, "Video seq type is %d", 
			 MP4AV_Mpeg3PictHdrType(es_pid->work->frame + es_pid->pict_header_offset));

	  mpeg2t_finished_es_work(es_pid, es_pid->work_loaded);

	  es_pid->have_seq_header = 0;
	  mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	  if (es_pid->work != NULL) {
	    es_pid->work->frame[0] = 0;
	    es_pid->work->frame[1] = 0;
	    es_pid->work->frame[2] = 1;
	    es_pid->work->frame[3] = *esptr;
	    es_pid->work_loaded = 4;
	    if (es_pid->header == MPEG3_PICTURE_START_CODE) {
	      es_pid->work_state = 2;
	      es_pid->pict_header_offset = 0;
	    } else {
	      es_pid->work_state = 1;
	    }
	  } else {
	    es_pid->work_state = 0;
	    es_pid->header = 0;
	    return framesfinished;
	  }
	}
      }
      esptr++;
      buflen--;
    }
    // Check for sequence header here...
    if (es_pid->header == MPEG3_SEQUENCE_START_CODE) {
      es_pid->have_seq_header = 1;
      es_pid->seq_header_offset = es_pid->work_loaded - 4;
    }
  }
  return framesfinished;
}
