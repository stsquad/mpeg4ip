#ifndef _WAV_H
#define _WAV_H

typedef struct {
	unsigned short	format_tag;
	unsigned short	channels;			/* 1 = mono, 2 = stereo */
	unsigned long	samplerate;			/* typically: 44100, 32000, 22050, 11025 or 8000*/
	unsigned long	bytes_per_second;	/* SamplesPerSec * BlockAlign*/
	unsigned short	blockalign;			/* Channels * (BitsPerSample / 8)*/
	unsigned short	bits_per_sample;	/* 16 or 8 */
} WAVEAUDIOFORMAT;

typedef struct {
	char info[4];
	unsigned long length;
} RIFF_CHUNK;

int CreateWavHeader(FILE *file, int channels, int samplerate, int resolution);
int UpdateWavHeader(FILE *file);

#endif
