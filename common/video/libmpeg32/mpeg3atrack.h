#ifndef MPEG3ATRACK_H
#define MPEG3ATRACK_H

#include "mpeg3demux.h"
#include "audio/mpeg3audio.h"

typedef struct
{
	int channels;
	int sample_rate;
	mpeg3_demuxer_t *demuxer;
	mpeg3audio_t *audio;
	long current_position;
	long total_samples;
	int format;               /* format of audio */




/* Pointer to master table of contents */
	int64_t *sample_offsets;
	int total_sample_offsets;
} mpeg3_atrack_t;

#endif
