#ifndef MPEG3ATRACK_H
#define MPEG3ATRACK_H

#include "mpeg3demux.h"

typedef struct
{
  void *file;
	int channels;
	int sample_rate;
	mpeg3_demuxer_t *demuxer;
	long current_position;
	long total_samples;
	int format;               /* format of audio */
  double percentage_seek;
  int framesize; // for fixed frame formats
  int samples_per_frame;
  uint32_t total_frames;
/* Pointer to master table of contents */
	int64_t *sample_offsets;
	int total_sample_offsets;
} mpeg3_atrack_t;

#endif
