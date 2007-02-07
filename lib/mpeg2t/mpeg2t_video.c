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

/* mpeg2t_video.c - parse ES stream for MPEG1/MPEG2 video */

#include "mpeg4ip.h"
#include "mpeg2_transport.h"
#include "mpeg2t_private.h"
#include "mp4av.h"
#include "mp4av_h264.h"

#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_SEQUENCE_END_CODE          0x000001b7


int process_mpeg2t_mpeg_video (mpeg2t_es_t *es_pid, 
			       const uint8_t *esptr, 
			       uint32_t buflen)
{
  int framesfinished = 0;

  if (es_pid->peshdr_loaded != 0 && ((es_pid->stream_id & 0xf0) != 0xe0)) {
    mpeg2t_message(LOG_ERR, "Video stream PID %x with bad stream_id %x", 
		   es_pid->pid.pid,
		   es_pid->stream_id);
    return 0;
  }

  while (buflen > 0) {
    es_pid->header <<= 8;
    es_pid->header |= *esptr;

    if ((es_pid->header & 0xffffff00) == 0x00000100) {
      mpeg2t_message(LOG_DEBUG, "header %x", es_pid->header);
    }

    if (es_pid->work_state == 0) {
      /*
       * Work state 0 - looking for any header
       */
      if ((es_pid->header & 0xffffff00) == 0x00000100) {
	// have first header.
	if (es_pid->work_max_size < 4096) es_pid->work_max_size = 4096;

	// always do this in state 0 to get the psts at the start
	mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	if (es_pid->work == NULL) return framesfinished;

	// Store header
	es_pid->work->frame[0] = 0;
	es_pid->work->frame[1] = 0;
	es_pid->work->frame[2] = 1;
	es_pid->work->frame[3] = *esptr;
	es_pid->work_loaded = 4;
	// If we have a PICTURE_START - go to work state 2
	// Otherwise, we're looking for the 1st PICTURE_START
	if (es_pid->header == MPEG3_PICTURE_START_CODE) {
	  es_pid->work_state = 2;
	  es_pid->pict_header_offset = 0;
	} else 
	  es_pid->work_state = 1;
#if 0
	mpeg2t_message(LOG_DEBUG, 
		       "video - state 0 header %x state %d", es_pid->header, 
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
      if (es_pid->work_loaded >= es_pid->work_max_size - 5) {
	uint8_t *frameptr;
	es_pid->work_max_size += 1024;
	frameptr = 
	  (uint8_t *)realloc(es_pid->work,
			     sizeof(mpeg2t_frame_t) + 
			     es_pid->work_max_size);
#if 0
	mpeg2t_message(LOG_DEBUG, "Es pid work reallocing to %d", 
		       es_pid->work_max_size);
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
	 * next picture start code, or one of SEQUENCE or GOP_START.
	 * If SEQUENCE or GOP START, make sure the header at the end
	 * is a PICTURE_START header.
	 */
	// Might want to enhance this to stop at GOP, also
	if (es_pid->header == MPEG3_PICTURE_START_CODE ||
	    es_pid->header == MPEG3_SEQUENCE_START_CODE ||
	    es_pid->header == MPEG3_GOP_START_CODE ||
	    es_pid->header == MPEG3_SEQUENCE_END_CODE) {
	  // last frame code should be 0 to finish off picture code.
	  es_pid->work->frame[es_pid->work_loaded - 1] = 0;
	  es_pid->work->seq_header_offset = es_pid->seq_header_offset;
	  framesfinished = 1;
	  if (es_pid->info_loaded == 0 && es_pid->have_seq_header) {
	    uint32_t h, w;
	    double frame_rate, bitrate;
	    int have_mpeg2;
	    uint8_t profile;
	    if (MP4AV_Mpeg3ParseSeqHdr(es_pid->work->frame + es_pid->seq_header_offset,
				       es_pid->work_loaded - es_pid->seq_header_offset,
				       &have_mpeg2,
				       &h, 
				       &w, 
				       &frame_rate,
				       &bitrate, 
				       NULL,
				       &profile) >= 0) {
	      mpeg2t_message(LOG_NOTICE, "Found seq header - h %d w %d fr %g offset %d len %d", 
			     h, w, frame_rate, es_pid->seq_header_offset,
			     es_pid->work_loaded);
	      es_pid->info_loaded = 1;
	      es_pid->h = h;
	      es_pid->w = w;
	      es_pid->bitrate = bitrate;
	      es_pid->frame_rate = frame_rate;
	      es_pid->tick_per_frame = (uint32_t)(90000.0 / frame_rate);
	      es_pid->mpeg_layer = have_mpeg2 ? 2 : 1;
	    }
	  }

	  // store the frame type so we can figure out the timestamps
	  es_pid->work->frame_type = 
	    MP4AV_Mpeg3PictHdrType(es_pid->work->frame + 
				   es_pid->pict_header_offset);
	  es_pid->work->pict_header_offset = es_pid->pict_header_offset;
	  mpeg2t_finished_es_work(es_pid, es_pid->work_loaded);

	  es_pid->have_seq_header = 0;
	  es_pid->seq_header_offset = 0;
	  mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	  if (es_pid->work != NULL) {
	    // Put the header we just found at the start of the frame,
	    // then set the work state accordingly.
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

int mpeg2t_mpeg_video_info (mpeg2t_es_t *es_pid, char *buffer, size_t len)
{
  int rate, offset;
  if (es_pid->info_loaded == 0) return -1;
  offset = snprintf(buffer, len, "MPEG-%d Video, %d x %d, %g",
		    es_pid->mpeg_layer, es_pid->w, es_pid->h, es_pid->frame_rate);
  if (es_pid->bitrate > 0.0) {
    rate = es_pid->bitrate / 1000;
    snprintf(buffer + offset, len - offset, ", %d kbps", rate);
  }
  return 0;
}



int mpeg2t_h264_video_info (mpeg2t_es_t *es_pid, char *buffer, size_t len)
{
  int rate, offset;
  if (es_pid->info_loaded == 0) return -1;
  offset = snprintf(buffer, len, "H.264 Video, %d x %d",
		    es_pid->w, es_pid->h);
  if (es_pid->bitrate > 0.0) {
    rate = es_pid->bitrate / 1000;
    snprintf(buffer + offset, len - offset, ", %d kbps", rate);
  }
  return 0;
}

int process_mpeg2t_h264_video (mpeg2t_es_t *es_pid, 
			       const uint8_t *esptr, 
			       uint32_t buflen)
{

  int have_header = 0;
  uint8_t nal_value = 0;
  int framesfinished = 0;
#if 0
  mpeg2t_message(LOG_DEBUG, "enter h264 process");
  if (es_pid->peshdr_loaded != 0 && ((es_pid->stream_id & 0xf0) != 0xe0)) {
    mpeg2t_message(LOG_ERR, "Video stream PID %x with bad stream_id %x", 
		   es_pid->pid.pid,
		   es_pid->stream_id);
    return 0;
  }
#endif
  // note - one thing that we're not handling correctly is the
  // extra 0 byte before the access header.  That's okay for now, but
  // may run into problems later.
  while (buflen > 0) {
    es_pid->header <<= 8;
    es_pid->header |= *esptr;
    have_header = 0;
    if ((es_pid->header & 0xffffff00) == 0x00000100) {
      have_header = 1;
      nal_value = es_pid->header & 0x1f;
      mpeg2t_message(LOG_DEBUG, "header %x %x %d", es_pid->header, nal_value,
		     es_pid->work_state);
    }

    if (es_pid->work_state == 0) {
      /*
       * Work state 0 - looking for access unit header
       */
      if (have_header != 0 && nal_value == H264_NAL_TYPE_ACCESS_UNIT) {
	// have first header.
	if (es_pid->work_max_size < 4096) es_pid->work_max_size = 4096;

	// always do this in state 0 to get the psts at the start
	mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	if (es_pid->work == NULL) return framesfinished;

	// Store header
	es_pid->work->flags = 0;
	es_pid->work->frame[0] = 0;
	es_pid->work->frame[1] = 0;
	es_pid->work->frame[2] = 1;
	es_pid->work->frame[3] = *esptr;
	es_pid->work_loaded = 4;
	es_pid->work_state = 2;
#if 1
	mpeg2t_message(LOG_DEBUG, "video - state 0 header %x state %d", es_pid->header, 
	       es_pid->work_state);
#endif
      }
    } else {
      /*
       * All other work states - load the current byte into the
       * buffer - reallocate buffer if needed
       */

      if (es_pid->work_loaded >= es_pid->work_max_size - 5) {
	uint8_t *frameptr;
	es_pid->work_max_size += 1024;
	frameptr = 
	  (uint8_t *)realloc(es_pid->work,
			     sizeof(mpeg2t_frame_t) + 
			     es_pid->work_max_size);

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


      if (have_header != 0) {
	if (h264_nal_unit_type_is_slice(nal_value)) {
	  if ((es_pid->work->flags & HAVE_PICT_HEADER) == 0) {
	    es_pid->work->pict_header_offset = es_pid->work_loaded - 4;
	    es_pid->work->flags |= HAVE_PICT_HEADER;
	  }
	} else {
	  switch (nal_value) {
	  case H264_NAL_TYPE_ACCESS_UNIT:
	    es_pid->work->frame[es_pid->work_loaded - 1] = 0;
	    framesfinished = 1;
	    if (es_pid->info_loaded == 0) {
	      // look to fill in es_pid information such as frame rate, etc
	      if ((es_pid->work->flags & HAVE_SEQ_HEADER) == HAVE_SEQ_HEADER) {
		h264_decode_t dec;
		memset(&dec, 0, sizeof(dec));
		if (h264_read_seq_info(es_pid->work->frame + es_pid->work->seq_header_offset,
				       es_pid->work_loaded - es_pid->work->seq_header_offset,
				       &dec) == 0) {
		  es_pid->info_loaded = 1;
		  es_pid->h = dec.pic_height;
		  es_pid->w = dec.pic_width;
		  mpeg2t_message(LOG_DEBUG, "h264 read seq header - %ux%u",
				 es_pid->w, es_pid->h);
		  // don't have timing info yet.
		  //es_pid->bitrate = bitrate;
		  //es_pid->frame_rate = frame_rate;
		  //es_pid->tick_per_frame = (uint32_t)(90000.0 / frame_rate);
		  //es_pid->mpeg_layer = have_mpeg2 ? 2 : 1;
		}
	      }
	    }

	    // store the frame type so we can figure out the timestamps
	    // we don't know how the pts and dts will work, so assume we'll
	    // have the values.
#if 0
	    es_pid->work->frame_type = 
	      MP4AV_Mpeg3PictHdrType(es_pid->work->frame + 
				     es_pid->pict_header_offset);
#endif


	    // xxx hack for temporary
	    es_pid->work->frame_type = 1;
	    //printf("hi %d\n", es_pid->work->frame_type);

	    mpeg2t_message(LOG_DEBUG, "finished work %d", es_pid->work_loaded);
	    // -4 might have to be -5 in the case of zero byte
	    mpeg2t_finished_es_work(es_pid, es_pid->work_loaded - 4);

	    es_pid->have_seq_header = 0;
	    mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	    if (es_pid->work != NULL) {
	      // Put the header we just found at the start of the frame,
	      // then set the work state accordingly.
	      es_pid->work->frame[0] = 0;
	      es_pid->work->frame[1] = 0;
	      es_pid->work->frame[2] = 1;
	      es_pid->work->frame[3] = *esptr;
	      es_pid->work_loaded = 4;
	      es_pid->work->flags = 0;
	      es_pid->work_state = 2;
	    } else {
	      es_pid->work_state = 0;
	      es_pid->header = 0;
	      return framesfinished;
	    }
	    break;
	  case H264_NAL_TYPE_SEQ_PARAM:
	    es_pid->work->seq_header_offset = es_pid->work_loaded - 4;
	    es_pid->work->flags |= HAVE_SEQ_HEADER;
	    es_pid->have_seq_header = 1;
	    break;
	  case H264_NAL_TYPE_PIC_PARAM:
	    es_pid->work->nal_pic_param_offset = es_pid->work_loaded - 4;
	    es_pid->work->flags |= HAVE_PIC_PARAM_HEADER;
	    break;
	  
	  }
	}
      }
    }
    esptr++;
    buflen--;
  }
  return framesfinished;

}

int mpeg2t_mpeg4_video_info (mpeg2t_es_t *es_pid, char *buffer, size_t len)
{
  int offset;
  if (es_pid->info_loaded == 0) return -1;
  offset = snprintf(buffer, len, "Mpeg-4 Video, %d x %d",
		    es_pid->w, es_pid->h);
  return 0;
}

int process_mpeg2t_mpeg4_video (mpeg2t_es_t *es_pid, 
				const uint8_t *esptr, 
				uint32_t buflen)
{

  bool have_header = false;
  uint8_t header_value = 0;
  int framesfinished = 0;
#if 0
  mpeg2t_message(LOG_DEBUG, "enter mpeg4 process");
  if (es_pid->peshdr_loaded != 0 && ((es_pid->stream_id & 0xf0) != 0xe0)) {
    mpeg2t_message(LOG_ERR, "Video stream PID %x with bad stream_id %x", 
		   es_pid->pid.pid,
		   es_pid->stream_id);
    return 0;
  }
#endif
  // note - one thing that we're not handling correctly is the
  // extra 0 byte before the access header.  That's okay for now, but
  // may run into problems later.
  while (buflen > 0) {
    es_pid->header <<= 8;
    es_pid->header |= *esptr;
    have_header = false;
    if ((es_pid->header & 0xffffff00) == 0x00000100) {
      have_header = true;
      header_value = es_pid->header;
      mpeg2t_message(LOG_DEBUG, "header %x %x %d", es_pid->header, 
		     header_value,
		     es_pid->work_state);
    }

    switch (es_pid->work_state) {
    case 0:
      if (have_header == false) break;
      /*
       * Work state 0 - looking for any header
       */
      // have a header.
      if (es_pid->work_max_size < 4096) es_pid->work_max_size = 4096;

      // always do this in state 0 to get the psts at the start
      mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
      if (es_pid->work == NULL) return framesfinished;
      
      // Store header
      es_pid->work->flags = 0;
      es_pid->work->frame[0] = 0;
      es_pid->work->frame[1] = 0;
      es_pid->work->frame[2] = 1;
      es_pid->work_loaded = 3;
      es_pid->work_state = 1; // looking for VOP
#if 1
      mpeg2t_message(LOG_DEBUG, "video - state 0 header %x state %d", es_pid->header, 
		     es_pid->work_state);
#endif
      // fall into:
    case 1:
    case 2:
      /*
       * Work state 1 - looking for VOP
       */
      if (es_pid->work_loaded >= es_pid->work_max_size - 5) {
	uint8_t *frameptr;
	es_pid->work_max_size += 1024;
	frameptr = 
	  (uint8_t *)realloc(es_pid->work,
			     sizeof(mpeg2t_frame_t) + 
			     es_pid->work_max_size);

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

      if (have_header) {
	if (es_pid->work_state == 2) {
	  // we're finished with this frame
	  es_pid->work->frame_type = 1;
	  //printf("hi %d\n", es_pid->work->frame_type);

	  mpeg2t_message(LOG_DEBUG, "finished work %d", es_pid->work_loaded);
	  // -4 might have to be -5 in the case of zero byte
	  mpeg2t_finished_es_work(es_pid, es_pid->work_loaded - 4);

	  es_pid->have_seq_header = 0;
	  mpeg2t_malloc_es_work(es_pid, es_pid->work_max_size);
	  if (es_pid->work != NULL) {
	    // Put the header we just found at the start of the frame,
	    // then set the work state accordingly.
	    es_pid->work->frame[0] = 0;
	    es_pid->work->frame[1] = 0;
	    es_pid->work->frame[2] = 1;
	    es_pid->work->frame[3] = *esptr;
	    es_pid->work_loaded = 4;
	    es_pid->work->flags = 0;
	    es_pid->work_state = 1;
	  } else {
	    es_pid->work_state = 0;
	    es_pid->header = 0;
	    return framesfinished;
	  }
	} 
	// now, figure out what state to do based on header
	if (header_value == MP4AV_MPEG4_VOP_START){ 
	  // now, we're reading until the next header
	  es_pid->work_state = 2;
	  if (es_pid->info_loaded == 0) {
	    // read the VOL
	    if ((es_pid->work->flags & HAVE_SEQ_HEADER) == HAVE_SEQ_HEADER) {
	      u_int8_t TimeBits;
	      u_int16_t TimeTicks;
	      u_int16_t FrameDuration;
	      u_int16_t FrameWidth;
	      u_int16_t FrameHeight;
	      
	      if (MP4AV_Mpeg4ParseVol(es_pid->work->frame + es_pid->work->seq_header_offset,
				      es_pid->work_loaded - es_pid->work->seq_header_offset,
				      &TimeBits, 
				      &TimeTicks, 
				      &FrameDuration, 
				      &FrameWidth,
				      &FrameHeight, 
				      NULL, 
				      NULL, 
				      NULL)) { 
		es_pid->info_loaded = 1;
		es_pid->h = FrameHeight;
		es_pid->w = FrameWidth;
		mpeg2t_message(LOG_DEBUG, "parsed mpeg4 vol");
	      }
	    }
	  } 
	} else {
	  // we only really care about VOL headers
	  if (header_value >= MP4AV_MPEG4_VOL_START &&
		header_value < MP4AV_MPEG4_VOL_START + 0xf) {
	      // have VOL
	      es_pid->work->seq_header_offset = es_pid->work_loaded - 4;
	      es_pid->work->flags |= HAVE_SEQ_HEADER;
	      mpeg2t_message(LOG_DEBUG, "have mpeg4 vol");
	    }
	}
      }
      break;
    }
    esptr++;
    buflen--;
  }
  return framesfinished;

}
