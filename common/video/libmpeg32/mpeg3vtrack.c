#include "libmpeg3.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "mpeg3vtrack.h"
//#include "video/mpeg3video.h"
//#include "video/mpeg3videoprotos.h"
#include "mp4av.h"
#include <stdlib.h>

void mpeg3vtrack_cleanup_frame (mpeg3_vtrack_t *track)
{
  long size;

  if (track->track_frame_buffer == NULL) return;

  size = track->track_frame_buffer_size;
  track->track_frame_buffer[0] = track->track_frame_buffer[size - 4];
  track->track_frame_buffer[1] = track->track_frame_buffer[size - 3];
  track->track_frame_buffer[2] = track->track_frame_buffer[size - 2];
  track->track_frame_buffer[3] = track->track_frame_buffer[size - 1];
#if 0
  printf("Clean up - code is %x %x %x %x\n",
	 track->track_frame_buffer[0],
	 track->track_frame_buffer[1],
	 track->track_frame_buffer[2],
	 track->track_frame_buffer[3]);
#endif
  track->track_frame_buffer_size = 4;
}

static 
int mpeg3vtrack_locate_frame (mpeg3_vtrack_t *track,
			    long frame)
{
  mpeg3_demuxer_t *demux;
  long start_frame;
  uint32_t offset;
  uint32_t code = 0;
  demux = track->demuxer;
  if (frame < track->current_position) {
    mpeg3demux_seek_start(demux);
    start_frame = 0;
  } else
    start_frame = track->current_position;

  // clean out last frame
  while (!mpeg3demux_eof(demux)) {
    mpeg3vtrack_cleanup_frame(track);
    offset = 4;
    track->track_frame_buffer_size = 
    mpeg3demux_read_data(demux,
			 track->track_frame_buffer,
			 track->track_frame_buffer_maxsize);
    for (offset = 4, code = 0; offset < track->track_frame_buffer_size - 3; offset++) {
      code <<= 8;
      code |= track->track_frame_buffer[offset];
      if (code == MPEG3_PICTURE_START_CODE) {
	start_frame++;
	if (start_frame >= frame) {
	  for (offset = offset - 3; 
	       offset < track->track_frame_buffer_size; 
	       offset++) {
	    mpeg3demux_read_prev_char(demux);
	  }
	  track->track_frame_buffer_size = 0;
	  track->current_position = frame;
	  return 0;
	}
      }
    }
  }
  return 1;
}
  
  
    
static
int mpeg3vtrack_get_frame (mpeg3_vtrack_t *track)

{
	uint32_t code = 0;
	int have_pict_start, done;
	mpeg3_demuxer_t *demux = track->demuxer;
	unsigned char *output;

	have_pict_start = 0;
	track->current_position++;
	output = track->track_frame_buffer;
	if ((track->track_frame_buffer_size != 0) &&
	    output[0] == 0 && output[1] == 0 && output[2] == 1) {
	  if (output[3] == 0)
	      have_pict_start = 1;
	    output += 4;
	} 

	done = 0;
	while(done == 0 && 
		code != MPEG3_SEQUENCE_END_CODE &&
		!mpeg3demux_eof(demux))
	{
	  if (track->track_frame_buffer_size + 3 >= 
	      track->track_frame_buffer_maxsize) {
	    int diff = output - track->track_frame_buffer;
	    track->track_frame_buffer_maxsize += 4096;
	    track->track_frame_buffer = 
	      realloc(track->track_frame_buffer, 
		      track->track_frame_buffer_maxsize);
	    if (track->track_frame_buffer == NULL) exit(1);
	    output = track->track_frame_buffer + diff;
	  }
	  

	  code <<= 8;
	  // can't do this yet - we're still reading from the demuxer...
	  *output = mpeg3demux_read_char(track->demuxer);
	  code |= *output++;
	  //	  printf("%d %08x\n", track->track_frame_buffer_size, code);
	  track->track_frame_buffer_size++;
	  if (code == MPEG3_PICTURE_START_CODE) {
	    if (have_pict_start == 1) done = 1;
	    have_pict_start = 1;
	  } else if ((code == MPEG3_GOP_START_CODE) ||
		     (code == MPEG3_SEQUENCE_START_CODE)) {
	    if (have_pict_start == 1) done = 1;
	  }
	}
	return mpeg3demux_eof(demux);
}



/* Line up on the beginning of the previous code. */
int mpeg3vtrack_prev_code(mpeg3_demuxer_t* demux, uint32_t code)
{
  uint32_t testcode = 0;
  int bytes = 0;

  if (mpeg3demux_bof(demux)) return mpeg3demux_bof(demux);

  do {
    testcode >>= 8;
    testcode |= (mpeg3demux_read_prev_char(demux) << 24);
    bytes++;
  } while(!mpeg3demux_bof(demux) && testcode != code);

  return mpeg3demux_bof(demux);
}

// Must perform seeking now to get timecode for audio
int mpeg3vtrack_seek_percentage(mpeg3_vtrack_t *vtrack, double percentage)
{
  vtrack->percentage_seek = percentage;
  return 0;
}

int mpeg3vtrack_seek_frame(mpeg3_vtrack_t *vtrack, long frame)
{
  vtrack->frame_seek = frame;
  return 0;
}

int mpeg3vtrack_seek(mpeg3_vtrack_t *track, int dont_decode)
{
  long this_gop_start;
  int result = 0;
  int back_step;
  int attempts;
  mpeg3_t *file = track->file;
  int is_video_stream = file->is_video_stream;
  double percentage;
  long frame_number;
  int match_refframes = 1;
  mpeg3_demuxer_t *demux = track->demuxer;
  int ix;


  // Must do seeking here so files which don't use video don't seek.
  /* Seek to percentage */
  if(track->percentage_seek >= 0)
    {
      //printf("mpeg3video_seek 1 %f\n", video->percentage_seek);
      track->track_frame_buffer_size = 0;

      percentage = track->percentage_seek;
      track->percentage_seek = -1;
      mpeg3demux_seek_percentage(track->demuxer, percentage);
      printf("Seek time is %g\n", mpeg3demux_get_time(track->demuxer));
    }
  else
    /* Seek to a frame */
    if(track->frame_seek >= 0)
      {
	frame_number = track->frame_seek;
	track->frame_seek = -1;
	if(frame_number < 0) frame_number = 0;
	if(frame_number > track->total_frames) 
	  frame_number = track->total_frames;

	//printf("mpeg3video_seek 1 %ld %ld\n", frame_number, video->framenum);

	/* Seek to I frame in table of contents */
	if(track->frame_offsets)
	  {
	    int i;
	    for(i = track->total_keyframe_numbers - 1; i >= 0; i--)
	      {
		if(track->keyframe_numbers[i] <= frame_number)
		  {
		    int frame;
		    int title_number;
		    int64_t byte;

		    // Go 2 I-frames before current position
		    frame = track->keyframe_numbers[i];
		    title_number = (track->frame_offsets[frame] & 
				    0xff00000000000000) >> 56;
		    byte = track->frame_offsets[frame] & 
		      0xffffffffffffff;


		    mpeg3demux_open_title(demux, title_number);
		    mpeg3demux_seek_byte(demux, byte);

		    for (ix = frame; ix < frame_number; ix++) {
		      mpeg3vtrack_get_frame(track);
		    }
		    break;
		  }
	      }
	  }
	else
	  /* Seek to start of file */
	  {
	  if(frame_number < 16)
	    {
	      mpeg3demux_seek_start(demux);
	      for (ix = 0; ix < frame_number; ix++) {
		mpeg3vtrack_get_frame(track);
	      }
#if 0
	      track->video->repeat_count = track->video->current_repeat = 0;
	      track->video->framenum = 0;
	      result = mpeg3vtrack_drop_frames(track, 
					       frame_number - video->framenum);
#endif
	    }
	  else
	    {
	      /* Seek to an I frame. */
		  /* Elementary stream */
		  if(is_video_stream)
		    {
#if 0
		      //mpeg3_t *file = video->file;
		      mpeg3_vtrack_t *track = video->track;
		      int64_t byte = (int64_t)((double)(mpeg3demuxer_total_bytes(vstream->demuxer) / 
							track->total_frames) * 
					       frame_number);
		      long minimum = 65535;
		      int done = 0;

		      //printf("seek elementary %d\n", frame_number);
		      /* Get GOP just before frame */
		      do
			{
			  result = mpeg3demux_seek_byte(demux, byte);

			  if(!result) 
			    result = mpeg3vtrack_prev_code(demux, MPEG3_GOP_START_CODE);
			  mpeg3demux_read_char(demux);
			  if(!result) result = mpeg3video_getgophdr(video);
			  this_gop_start = mpeg3video_goptimecode_to_frame(video);

			  //printf("wanted %ld guessed %ld byte %ld result %d\n", frame_number, this_gop_start, byte, result);
			  if(labs(this_gop_start - frame_number) >= labs(minimum)) 
			    done = 1;
			  else
			    {
			      minimum = this_gop_start - frame_number;
			      byte += (long)((float)(frame_number - this_gop_start) * 
					     (float)(mpeg3demuxer_total_bytes(vstream->demuxer) / 
						     track->total_frames));
			      if(byte < 0) byte = 0;
			    }
			}while(!result && !done);

		      //printf("wanted %d guessed %d\n", frame_number, this_gop_start);
		      if(!result)
			{
			  video->framenum = this_gop_start;
			  result = mpeg3video_drop_frames(video, frame_number - video->framenum);
			}
#endif
		    }
		  else



		    /* System stream */
		    {
		      mpeg3vtrack_locate_frame(track, frame_number);
#if 0
		      mpeg3demux_seek_time(demux, ((double)frame_number) / track->frame_rate);
		      percentage = mpeg3demux_tell_percentage(demux);
#endif
		      //printf("seek frame %ld percentage %f byte %ld\n", frame_number, percentage, mpeg3bits_tell(vstream));
		    }
		}

	    }
      }

  return result;
}

int mpeg3vtrack_previous_frame(mpeg3_vtrack_t *vtrack)
{
  int ix;
  if(mpeg3demux_tell_percentage(vtrack->demuxer) <= 0) return 1;

  // Get one picture
  mpeg3vtrack_prev_code(vtrack->demuxer, MPEG3_PICTURE_START_CODE);
  for (ix = 0; ix < 4; ix++) {
    mpeg3demux_read_prev_char(vtrack->demuxer);
  }

  if(mpeg3demux_bof(vtrack->demuxer)) 
    mpeg3demux_seek_percentage(vtrack->demuxer, 0);
  //  vtrack->video->repeat_count = 0;
  return 0;
}

#if 0
int mpeg3vtrack_drop_frames(mpeg3_vtrack_t *vtrack, long frames)
{
  int result = 0;
  long frame_number = video->framenum + frames;

  /* Read the selected number of frames and skip b-frames */
  while(!result && frame_number > video->framenum)
    {
      result = mpeg3video_read_frame_backend(video, frame_number - video->framenum);
    }
  return result;
}
#endif
/* ======================================================================= */
/*                                    ENTRY POINTS */
/* ======================================================================= */


mpeg3_vtrack_t* mpeg3_new_vtrack(mpeg3_t *file, 
	int stream_id, 
	mpeg3_demuxer_t *demuxer,
	int number)
{
  uint32_t h,w;
  int have_mpeg2;
  double frame_rate, bitrate;

	int result = 0;
	mpeg3_vtrack_t *new_vtrack;
	new_vtrack = calloc(1, sizeof(mpeg3_vtrack_t));
	new_vtrack->file = file;
	new_vtrack->demuxer = mpeg3_new_demuxer(file, 0, 1, stream_id);
	if(new_vtrack->demuxer) mpeg3demux_copy_titles(new_vtrack->demuxer, demuxer);
	new_vtrack->current_position = 0;
	new_vtrack->percentage_seek = -1;
	new_vtrack->frame_seek = -1;
	new_vtrack->track_frame_buffer = NULL;
	new_vtrack->track_frame_buffer_maxsize = 0;

//printf("mpeg3_new_vtrack 1\n");
// Copy pointers
	if(file->frame_offsets)
	{
		new_vtrack->frame_offsets = file->frame_offsets[number];
		new_vtrack->total_frame_offsets = file->total_frame_offsets[number];
		new_vtrack->keyframe_numbers = file->keyframe_numbers[number];
		new_vtrack->total_keyframe_numbers = file->total_keyframe_numbers[number];
	}

//printf("mpeg3_new_vtrack 1\n");
//printf("mpeg3_new_vtrack "X64"\n", mpeg3demux_tell(new_vtrack->demuxer));
/* Get information about the track here. */

	if (mpeg3vtrack_get_frame(new_vtrack)) {
	  mpeg3_delete_vtrack(file, new_vtrack);
	  return NULL;
	}
	
	if (MP4AV_Mpeg3ParseSeqHdr(new_vtrack->track_frame_buffer, 
				   new_vtrack->track_frame_buffer_size, 
				   &have_mpeg2,
				   &h,
				   &w, 
				   &frame_rate,
				   &bitrate) < 0) {
	  mpeg3_delete_vtrack(file, new_vtrack);
	  return NULL;
	}

	new_vtrack->frame_rate = frame_rate;
	new_vtrack->width = w;
	new_vtrack->height = h;
	new_vtrack->mpeg_layer = have_mpeg2 ? 2 : 1;
	mpeg3demux_seek_start(new_vtrack->demuxer);
	new_vtrack->track_frame_buffer_size = 0;

	if (!new_vtrack->frame_offsets) {
	  if (file->is_video_stream) {
	  } else {
	    new_vtrack->total_frames = 
	      (long)(mpeg3demux_length(new_vtrack->demuxer) * frame_rate);
	  }
	} else {
	  new_vtrack->total_frames = new_vtrack->total_frame_offsets;
	}

//printf("mpeg3_new_vtrack 2\n");
	return new_vtrack;
}

int mpeg3_delete_vtrack(mpeg3_t *file, mpeg3_vtrack_t *vtrack)
{
	if(vtrack->demuxer) mpeg3_delete_demuxer(vtrack->demuxer);
	if(vtrack->track_frame_buffer) free(vtrack->track_frame_buffer);
	free(vtrack);
	return 0;
}

/* Read all the way up to and including the next picture start code */
int mpeg3vtrack_read_raw (mpeg3_vtrack_t *vtrack, 
			  unsigned char *output, 
			  long *size, 
			  long max_size)
{
  uint32_t code = 0;

  mpeg3_demuxer_t *demux = vtrack->demuxer;
  *size = 0;
  while(code != MPEG3_PICTURE_START_CODE && 
	code != MPEG3_SEQUENCE_END_CODE &&
	*size < max_size && 
	!mpeg3demux_eof(demux))
    {
      code <<= 8;
      *output = mpeg3demux_read_char(demux);
      code |= *output++;
      (*size)++;
    }
  return mpeg3demux_eof(demux);
}
/* Read all the way up to and including the next picture start code */
int mpeg3vtrack_read_raw_resize (mpeg3_vtrack_t *vtrack, 
				 unsigned char **optr, 
				 long *size, 
				 int first)
{
  int result = 0;

  result = mpeg3vtrack_seek(vtrack, 1);
  
  if (!result) result = mpeg3vtrack_get_frame(vtrack);

  if (!result) {
    *optr = vtrack->track_frame_buffer;
    *size = vtrack->track_frame_buffer_size;
  }
  return result;
}

