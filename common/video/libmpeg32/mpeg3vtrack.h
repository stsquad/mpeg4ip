#ifndef MPEG3_VTRACK_H
#define MPEG3_VTRACK_H

#include "mpeg3demux.h"
//#include "video/mpeg3video.h"

typedef struct
{
  void *file;
	int width;
	int height;
	float frame_rate;
	float aspect_ratio;
	mpeg3_demuxer_t *demuxer;
	long current_position;  /* Number of next frame to be played */
	long total_frames;     /* Total frames in the file */
  unsigned char *track_frame_buffer;
  long track_frame_buffer_size;
  long track_frame_buffer_maxsize;
  double percentage_seek;
  long frame_seek;
/* Pointer to master table of contents */
	int64_t *frame_offsets;
	int total_frame_offsets;
	int64_t *keyframe_numbers;
	int total_keyframe_numbers;
} mpeg3_vtrack_t;

#endif
