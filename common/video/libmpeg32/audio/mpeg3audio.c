#include "../libmpeg3.h"
#include "../mpeg3private.h"
#include "../mpeg3protos.h"
#include "mpeg3audio.h"
#include "tables.h"

#include <math.h>
#include <stdlib.h>

static mpeg3audio_t* allocate_struct(mpeg3_atrack_t *track)
{
  mpeg3audio_t *audio = calloc(1, sizeof(mpeg3audio_t));
  audio->track = track;
  audio->astream = mpeg3bits_new_stream(track->demuxer);
  audio->outscale = 1;
  audio->bsbuf = audio->bsspace[1];
  audio->init = 1;
  audio->bo = 1;
  audio->channels = 1;
  return audio;
}


static int delete_struct(mpeg3audio_t *audio)
{
  mpeg3bits_delete_stream(audio->astream);
  if(audio->pcm_sample) free(audio->pcm_sample);
  free(audio);
  return 0;
}






// Allocate more room for the decoders.
int mpeg3audio_replace_buffer(mpeg3audio_t *audio, long new_allocation)
{
  long i;

  audio->pcm_sample = realloc(audio->pcm_sample, 
			      sizeof(float) * new_allocation * audio->channels);
  audio->pcm_allocated = new_allocation;

  return 0;
}


static int read_frame(mpeg3audio_t *audio, int render)
{
  int result = 0;

  result = mpeg3audio_read_header(audio);
  if(!result)
    {
      switch(audio->format)
	{
	case AUDIO_AC3:
	  result = mpeg3audio_do_ac3(audio, render);
	  break;	
				
	case AUDIO_MPEG:
	  switch(audio->layer)
	    {
	    case 1:
	      break;

	    case 2:
	      result = mpeg3audio_dolayer2(audio, render);
	      break;

	    case 3:
	      result = mpeg3audio_dolayer3(audio, render);
	      break;

	    default:
	      result = 1;
	      break;
	    }
	  break;
			
	case AUDIO_PCM:
	  result = mpeg3audio_do_pcm(audio);
	  break;
	}
    }





  if(!result)
    {
      /* Byte align the stream */
      mpeg3bits_byte_align(audio->astream);
    }
  return result;
}
/* Get the length but also initialize the frame sizes. */
static int get_length(mpeg3audio_t *audio, mpeg3_atrack_t *track)
{
  long result = 0;
  long framesize1 = 0, total1 = 0;
  long framesize2 = 0, total2 = 0;
  long total_framesize = 0, total_frames = 0;
  long byte_limit = 131072;  /* Total bytes to gather information from */
  long total_bytes = 0;
  long major_framesize;     /* Bigger framesize + header */
  long minor_framesize;     /* Smaller framesize + header */
  long major_total;
  long minor_total;
  mpeg3_t *file = track->file;

  /* Get the frame sizes */
  mpeg3bits_seek_start(audio->astream);
  audio->pcm_point = 0;
  result = read_frame(audio, 0); /* Stores the framesize */
  audio->samples_per_frame = audio->pcm_point / audio->channels;

  // Try to deduce from frame size
  if(!track->sample_offsets)
    {
      switch(track->format)
	{
	case AUDIO_AC3:
	  audio->avg_framesize = audio->framesize;
	  break;

	case AUDIO_MPEG:
	  framesize1 = audio->framesize;
	  total_bytes += audio->framesize;
	  total1 = 1;

	  while(!result && total_bytes < byte_limit)
	    {
	      audio->pcm_point = 0;
	      result = read_frame(audio, 0);
	      total_bytes += audio->framesize;
	      if(audio->framesize != framesize1)
		{
		  framesize2 = audio->framesize;
		  total2 = 1;
		  break;
		}
	      else
		{
		  total1++;
		}
	    }

	  while(!result && total_bytes < byte_limit)
	    {
	      audio->pcm_point = 0;
	      result = read_frame(audio, 0);
	      total_bytes += audio->framesize;
	      if(audio->framesize != framesize2)
		{
		  break;
		}
	      else
		{
		  total2++;
		}
	    }

	  audio->pcm_point = 0;
	  result = read_frame(audio, 0);
	  if(audio->framesize != framesize1 && audio->framesize != framesize2)
	    {
	      /* Variable bit rate.  Get the average frame size. */
	      while(!result && total_bytes < byte_limit)
		{
		  audio->pcm_point = 0;
		  result = read_frame(audio, 0);
		  total_bytes += audio->framesize;
		  if(!result)
		    {
		      total_framesize += audio->framesize;
		      total_frames++;
		    }
		}
	      audio->avg_framesize = 4 + (float)(total_framesize + framesize1 + framesize2) / (total_frames + total1 + total2);
	    }
	  else
	    {
	      major_framesize = framesize2 > framesize1 ? framesize2 : framesize1;
	      major_total = framesize2 > framesize1 ? total2 : total1;
	      minor_framesize = framesize2 > framesize1 ? framesize1 : framesize2;
	      minor_total = framesize2 > framesize1 ? total1 : total2;
	      /* Add the headers to the framesizes */
	      audio->avg_framesize = 4 + (float)(major_framesize * major_total + minor_framesize * minor_total) / (major_total + minor_total);
	    }
	  break;

	case AUDIO_PCM:
	  break;
	}

      /* Estimate the total samples */
      if(file->is_audio_stream)
	{
	  /* From the raw file */
	  result = (long)((float)mpeg3demuxer_total_bytes(audio->astream->demuxer) / audio->avg_framesize * audio->samples_per_frame);
	}
      else
	{
	  /* Gross approximation from a multiplexed file. */
	  result = (long)(mpeg3demux_length(audio->astream->demuxer) * track->sample_rate);
	  /*		result = (long)((float)mpeg3_video_frames(file, 0) / mpeg3_frame_rate(file, 0) * track->sample_rate); */
	  /* We would scan the multiplexed packets here for the right timecode if only */
	  /* they had meaningful timecode. */
	}

    }
  else
    // Estimate image length based on table of contents
    {
      result = track->total_sample_offsets * MPEG3_AUDIO_CHUNKSIZE;
    }

  audio->pcm_point = 0;
  mpeg3audio_reset_synths(audio);
  return result;
}

int mpeg3audio_seek(mpeg3audio_t *audio, long position)
{
  int result = 0;
  mpeg3_atrack_t *track = audio->track;
  mpeg3_t *file = track->file;
  long frame_number;
  long byte_position;
  double time_position;

  /* Sample seek wasn't requested */
  if(audio->sample_seek < 0)
    {
      audio->pcm_position = position;
      audio->pcm_size = 0;
      return 0;
    }

  /* Seek to a sample. */
  /* Use table of contents */
  if(track->sample_offsets)
    {
      int index;
      int title_number;
      int64_t byte;

      index = position / MPEG3_AUDIO_CHUNKSIZE;
      if(index >= track->total_sample_offsets) index = track->total_sample_offsets - 1;
      title_number = (track->sample_offsets[index] & 
		      0xff00000000000000) >> 56;
      byte = track->sample_offsets[index] &
	0xffffffffffffff;

      mpeg3bits_open_title(audio->astream, title_number);
      mpeg3bits_seek_byte(audio->astream, byte);

      audio->pcm_position = index * MPEG3_AUDIO_CHUNKSIZE;
    }
  else
    /* Try to use multiplexer. */
    if(!file->is_audio_stream)
      {
	time_position = (double)position / track->sample_rate;
	result |= mpeg3bits_seek_time(audio->astream, time_position);
	audio->pcm_position = mpeg3bits_packet_time(audio->astream) * track->sample_rate;
	/*printf("wanted %f got %f\n", time_position, mpeg3bits_packet_time(audio->astream)); */
      }
    else
      {
	/* Seek in an elemental stream.  This algorithm achieves sample accuracy on fixed bitrates. */
	/* Forget about variable bitrates or program streams. */
	frame_number = position / audio->samples_per_frame;
	byte_position = (long)(audio->avg_framesize * frame_number);
	audio->pcm_position = frame_number * audio->samples_per_frame;

	if(byte_position < audio->avg_framesize * 2)
	  {
	    result |= mpeg3bits_seek_start(audio->astream);
	    audio->pcm_position = 0;
	  }
	else
	  {
	    result |= mpeg3bits_seek_byte(audio->astream, byte_position);
	  }
      }






  /* Arm the backstep buffer for layer 3 if not at the beginning already. */
  if(byte_position >= audio->avg_framesize * 2 && audio->layer == 3 && !result)
    {
      result |= mpeg3audio_prev_header(audio);
      result |= mpeg3audio_read_layer3_frame(audio);
    }

  /* Reset the tables. */
  mpeg3audio_reset_synths(audio);
  audio->pcm_size = 0;
  audio->pcm_point = 0;
  return result;
}

/* ================================================================ */
/*                                    ENTRY POINTS */
/* ================================================================ */




mpeg3audio_t* mpeg3audio_new(mpeg3_atrack_t *track, int format)
{
  mpeg3audio_t *audio = allocate_struct(track);
  int result = 0;

  /* Init tables */
  mpeg3audio_new_decode_tables(audio);
  audio->percentage_seek = -1;
  audio->sample_seek = -1;
  audio->format = format;

  //printf("mpeg3audio_new 1\n");
  /* Determine the format of the stream */
  if(audio->format == AUDIO_UNKNOWN)
    {
      if(((mpeg3bits_showbits(audio->astream, 32) & 0xffff0000) >> 16) == MPEG3_AC3_START_CODE)
	audio->format = AUDIO_AC3;
      else
	audio->format = AUDIO_MPEG;
    }

  //printf("mpeg3audio_new 1\n");

  /* get channel count */
  result = mpeg3audio_read_header(audio);

  //printf("mpeg3audio_new 1\n");
  /* Set up the sample buffer */
  mpeg3audio_replace_buffer(audio, 262144);

  //printf("mpeg3audio_new 1\n");

  /* Copy information to the mpeg struct */
  if(!result)
    {
      track->channels = audio->channels;

      switch(track->format)
	{
	case AUDIO_AC3:
	  track->sample_rate = mpeg3_ac3_samplerates[audio->sampling_frequency_code];
	  break;

	case AUDIO_MPEG:
	  track->sample_rate = mpeg3_freqs[audio->sampling_frequency_code];
	  break;

	case AUDIO_PCM:
	  track->sample_rate = 48000;
	  break;
	}

      track->total_samples = get_length(audio, track);
      result |= mpeg3bits_seek_start(audio->astream);
    }
  else
    {
      delete_struct(audio);
      audio = 0;
    }

  //printf("mpeg3audio_new 2\n");
  return audio;
}

int mpeg3audio_delete(mpeg3audio_t *audio)
{
  delete_struct(audio);
  return 0;
}

int mpeg3audio_seek_percentage(mpeg3audio_t *audio, double percentage)
{
  audio->percentage_seek = percentage;
  audio->pcm_size = 0;
  audio->pcm_point = 0;
  audio->pcm_position = 0;
  return 0;
}

int mpeg3audio_seek_sample(mpeg3audio_t *audio, long sample)
{
  mpeg3_atrack_t *track = audio->track;
  if(sample > track->total_samples) sample = track->total_samples;
  if(sample < 0) sample = 0;
  audio->sample_seek = sample;
  return 0;
}

/* Read raw frames for concatenation purposes */
int mpeg3audio_read_raw(mpeg3audio_t *audio, unsigned char *output, long *size, long max_size)
{
  int result = 0;
  int i;
  *size = 0;

  switch(audio->format)
    {
    case AUDIO_AC3:
      /* Just write the AC3 stream */
      result = mpeg3bits_read_buffer(audio->astream, output, audio->framesize);
      *size = audio->framesize;
      break;

    case AUDIO_MPEG:
      /* Fix the mpeg stream */
      result = mpeg3audio_read_header(audio);
      if(!result)
	{
	  if(max_size < 4) return 1;
	  *output++ = (audio->newhead & 0xff000000) >> 24;
	  *output++ = (audio->newhead & 0xff0000) >> 16;
	  *output++ = (audio->newhead & 0xff00) >> 8;
	  *output++ = (audio->newhead & 0xff);
	  *size += 4;

	  if(max_size < 4 + audio->framesize) return 1;
	  if(mpeg3bits_read_buffer(audio->astream, output, audio->framesize))
	    return 1;

	  *size += audio->framesize;
	}
      break;
		
    case AUDIO_PCM:
      if(mpeg3bits_read_buffer(audio->astream, output, audio->framesize))
	return 1;
      *size = audio->framesize;
      break;
    }
  return result;
}

/* Channel is 0 to channels - 1 */
int mpeg3audio_decode_audio(mpeg3audio_t *audio, 
			    float *output_f, 
			    short *output_i, 
			    int channel, 
			    long start_position, 
			    long len)
{
  long allocation_needed = len + MPEG3AUDIO_PADDING;
  long i, j, result = 0;
  mpeg3_atrack_t *atrack = audio->track;
  long attempts;
  int render = 0;

  if(output_f || output_i) render = 1;

  //printf("mpeg3audio_decode_audio 1\n");
  /* Create new buffer */
  if(audio->pcm_allocated < allocation_needed)
    {
      mpeg3audio_replace_buffer(audio, allocation_needed);
    }

  /* There was a percentage seek */
  if(audio->percentage_seek >= 0)
    {
      double audio_time, video_time;
      mpeg3bits_seek_percentage(audio->astream, audio->percentage_seek);
      /* Force the pcm buffer to be reread. */
      audio->pcm_position = start_position;
      audio->pcm_size = 0;
      audio->percentage_seek = -1;
      /* Get synchronization offset */
      /*
       * 		mpeg3audio_read_frame(audio);
       * 		audio_time = audio->astream->demuxer->pes_audio_time;
       * 		video_time = audio->astream->demuxer->pes_video_time;
       */
      //printf("mpeg3audio_decode_audio %f %f\n", audio_time, video_time);
    }
  else
    {
      /* Entire output is in buffer so don't do anything. */
      if(start_position >= audio->pcm_position && start_position < audio->pcm_position + audio->pcm_size &&
	 start_position + len <= audio->pcm_size)
	{
	  ;
	}
      else
	/* Output starts in buffer but ends later so slide it back. */
	if(start_position <= audio->pcm_position + audio->pcm_size &&
	   start_position >= audio->pcm_position)
	  {
	    for(i = 0, j = (start_position - audio->pcm_position) * audio->channels;
		j < audio->pcm_size * audio->channels;
		i++, j++)
	      {
		audio->pcm_sample[i] = audio->pcm_sample[j];
	      }

	    audio->pcm_point = i;
	    audio->pcm_size -= start_position - audio->pcm_position;
	    audio->pcm_position = start_position;
	  }
	else
	  {
	    /* Output is outside buffer completely. */
	    result = mpeg3audio_seek(audio, start_position);
	    audio->sample_seek = -1;
	    /* Check sanity */
	    if(start_position < audio->pcm_position) 
	      audio->pcm_position = start_position;
	  }
      audio->sample_seek = -1;
    }

  //printf("mpeg3audio_decode_audio 2\n");


  //printf("mpeg3audio_decode_audio %lx\n", mpeg3bits_tell(audio->astream));
  /* Read packets until the buffer is full. */
  if(!result)
    {
      attempts = 0;
      while(attempts < 6 &&
	    !mpeg3bits_eof(audio->astream) &&
	    audio->pcm_size + audio->pcm_position < start_position + len)
	{
	  result = read_frame(audio, render);
	  if(result) attempts++;
	  audio->pcm_size = audio->pcm_point / audio->channels;
	}
    }
  //printf("mpeg3audio_decode_audio 3\n");

  /* Copy the buffer to the output */
  if(output_f)
    {
      for(i = 0, j = (start_position - audio->pcm_position) * audio->channels + channel; 
	  i < len && j < audio->pcm_size * audio->channels; 
	  i++, j += audio->channels)
	{
	  output_f[i] = audio->pcm_sample[j];
	}
      for( ; i < len; i++)
	{
	  output_f[i] = 0;
	}
    }
  else
    if(output_i)
      {
	int sample;
	for(i = 0, j = (start_position - audio->pcm_position) * audio->channels + channel; 
	    i < len && j < audio->pcm_size * audio->channels; 
	    i++, j += audio->channels)
	  {
	    sample = (int)(audio->pcm_sample[j] * 32767);
	    if(sample > 32767) sample = 32767;
	    else 
	      if(sample < -32768) sample = -32768;
			
	    output_i[i] = sample;
	  }
	for( ; i < len; i++)
	  {
	    output_i[i] = 0;
	  }
      }
  //printf("mpeg3audio_decode_audio 4\n");

  if(audio->pcm_point > 0)
    return 0;
  else
    return result;
}
