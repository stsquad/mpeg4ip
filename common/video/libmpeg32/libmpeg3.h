#ifndef LIBMPEG3_H
#define LIBMPEG3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpeg3private.h"


/* Supported color models for mpeg3_read_frame */
#define MPEG3_RGB565 2
#define MPEG3_BGR888 0
#define MPEG3_BGRA8888 1
#define MPEG3_RGB888 3
#define MPEG3_RGBA8888 4
#define MPEG3_RGBA16161616 5

/* Color models for the 601 to RGB conversion */
/* 601 not implemented for scalar code */
#define MPEG3_601_RGB565 11
#define MPEG3_601_BGR888 7
#define MPEG3_601_BGRA8888 8
#define MPEG3_601_RGB888 9
#define MPEG3_601_RGBA8888 10

/* Supported color models for mpeg3_read_yuvframe */ 
#define MPEG3_YUV420P 12
#define MPEG3_YUV422P 13

/* Check for file compatibility.  Return 1 if compatible. */
int mpeg3_check_sig(const char *path);

/* Open the MPEG3 stream. */
mpeg3_t* mpeg3_open(const char *path);

/* Open the MPEG3 stream and copy the tables from an already open stream. */
/* Eliminates the initial timecode search. */
mpeg3_t* mpeg3_open_copy(const char *path, mpeg3_t *old_file);
int mpeg3_close(mpeg3_t *file);




/* Performance */

/* Query the MPEG3 stream about audio. */
int mpeg3_has_audio(mpeg3_t *file);
int mpeg3_total_astreams(mpeg3_t *file);             /* Number of multiplexed audio streams */
int mpeg3_audio_channels(mpeg3_t *file, int stream);
int mpeg3_sample_rate(mpeg3_t *file, int stream);
char* mpeg3_audio_format(mpeg3_t *file, int stream);
  int mpeg3_audio_samples_per_frame(mpeg3_t *file, int stream);
  uint32_t mpeg3_audio_get_number_of_frames(mpeg3_t *file, int stream);
/* Total length obtained from the timecode. */
/* For DVD files, this is unreliable. */

/* Read a PCM buffer of audio from 1 channel and advance the position. */
/* Return a 1 if error. */
/* Stream defines the number of the multiplexed stream to read. */
/* If both output arguments are null the audio is not rendered. */
#if 0
int mpeg3_read_audio(mpeg3_t *file, 
		float *output_f,      /* Pointer to pre-allocated buffer of floats */
		short *output_i,      /* Pointer to pre-allocated buffer of int16's */
		int channel,          /* Channel to decode */
		long samples,         /* Number of samples to decode */
		int stream);          /* Stream containing the channel */

/* Reread the last PCM buffer from a different channel and advance the position */
int mpeg3_reread_audio(mpeg3_t *file, 
		float *output_f,      /* Pointer to pre-allocated buffer of floats */
		short *output_i,      /* Pointer to pre-allocated buffer of int16's */
		int channel,          /* Channel to decode */
		long samples,         /* Number of samples to decode */
		int stream);          /* Stream containing the channel */

/* Read the next compressed audio chunk.  Store the size in size and return a  */
/* 1 if error. */
/* Stream defines the number of the multiplexed stream to read. */
int mpeg3_read_audio_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream);
#endif

  int mpeg3_read_audio_frame(mpeg3_t *file,
			     unsigned char **output,
			     uint32_t *size,
			     uint32_t *max_size,
			     int stream);
  int mpeg3_get_audio_format(mpeg3_t *file, int stream);


/* Query the stream about video. */
int mpeg3_has_video(mpeg3_t *file);
int mpeg3_total_vstreams(mpeg3_t *file);            /* Number of multiplexed video streams */
int mpeg3_video_width(mpeg3_t *file, int stream);
int mpeg3_video_height(mpeg3_t *file, int stream);
int mpeg3_video_layer(mpeg3_t *file, int stream);
float mpeg3_aspect_ratio(mpeg3_t *file, int stream); /* aspect ratio.  0 if none */
double mpeg3_frame_rate(mpeg3_t *file, int stream);  /* Frames/sec */
  double mpeg3_video_bitrate(mpeg3_t *file, int stream);

/* Total length.   */
/* For DVD files, this is 1 indicating only percentage seeking is available. */
long mpeg3_video_frames(mpeg3_t *file, int stream);
int mpeg3_set_frame(mpeg3_t *file, long frame, int stream); /* Seek to a frame */
int mpeg3_skip_frames();
long mpeg3_get_frame(mpeg3_t *file, int stream);            /* Tell current position */

/* Seek all the tracks based on a percentage of the total bytes in the  */
/* file or the total */
/* time in a toc if one exists.  Percentage is a 0 to 1 double. */
/* This eliminates the need for tocs and 64 bit longs but doesn't  */
/* give frame accuracy. */
int mpeg3_seek_percentage(mpeg3_t *file, double percentage);
int mpeg3_seek_audio_percentage(mpeg3_t *file, int stream, double percentage);
int mpeg3_seek_video_percentage(mpeg3_t *file, int stream, double percentage);
double mpeg3_tell_percentage(mpeg3_t *file);
int mpeg3_previous_frame(mpeg3_t *file, int stream);
int mpeg3_end_of_audio(mpeg3_t *file, int stream);
int mpeg3_end_of_video(mpeg3_t *file, int stream);

/* Give the seconds time in the last packet read */

/* Read a frame.  The dimensions of the input area and output frame must be supplied. */
/* The frame is taken from the input area and scaled to fit the output frame in 1 step. */
/* Stream defines the number of the multiplexed stream to read. */
/* The last row of **output_rows must contain 4 extra bytes for scratch work. */
/* Read the next compressed frame including headers. */
/* Store the size in size and return a 1 if error. */
/* Stream defines the number of the multiplexed stream to read. */
int mpeg3_read_video_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream);

  int mpeg3_read_video_chunk_resize(mpeg3_t *file,
				    unsigned char **output,
				    long *size,
				    int stream);
  void mpeg3_read_video_chunk_cleanup(mpeg3_t *file, int stream);
/* Master control */
int mpeg3_total_programs();
int mpeg3_set_program(int program);

#ifdef __cplusplus
}
#endif

#endif
