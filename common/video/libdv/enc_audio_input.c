/* 
 *  enc_input.c
 *
 *     Copyright (C) Peter Schlaile - Feb 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>

#include "enc_audio_input.h"

#if HAVE_SYS_SOUNDCARD_H
#define HAVE_DEV_DSP  1
#endif

#if HAVE_DEV_DSP
#include <sys/types.h>
#include <sys/soundcard.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

// #define ARCH_X86 0

static jmp_buf error_jmp_env;

static void (*audio_converter)(unsigned char* in_buf, unsigned char* out_buf,
			       int num_samples) = NULL;

static void convert_u8(unsigned char* in_buf, unsigned char* out_buf,
		       int num_samples)
{
	int i;
	for (i = 0; i < num_samples; i++) {
		int val = *in_buf++ - 128;
		*out_buf++ = val >> 8;
		*out_buf++ = val & 0xff;
	}
}


static void convert_s16_le(unsigned char* in_buf, unsigned char* out_buf,
			   int num_samples)
{
	int i;
	for (i = 0; i < num_samples; i++) {
		*out_buf++ = in_buf[1];
		*out_buf++ = in_buf[0];
		in_buf += 2;
	}
}

static void convert_s16_be(unsigned char* in_buf, unsigned char* out_buf,
			   int num_samples)
{
	memcpy(out_buf, in_buf, 2*num_samples);
}

static void convert_u16_le(unsigned char* in_buf, unsigned char* out_buf,
			   int num_samples)
{
	int i;
	for (i = 0; i < num_samples; i++) {
		int val = (in_buf[0] + (in_buf[1] << 8)) - 32768;
		*out_buf++ = val >> 8;
		*out_buf++ = val & 0xff;
		in_buf += 2;
	}
}

static void convert_u16_be(unsigned char* in_buf, unsigned char* out_buf,
			   int num_samples)
{
	int i;
	for (i = 0; i < num_samples; i++) {
		int val = (in_buf[1] + (in_buf[0] << 8)) - 32768;
		*out_buf++ = val >> 8;
		*out_buf++ = val & 0xff;
		in_buf += 2;
	}
}


unsigned long read_long(FILE* in_wav)
{
	unsigned char buf[4];

	if (fread(buf, 1, 4, in_wav) != 4) {
		fprintf(stderr, "WAV: Short read!\n");
		longjmp(error_jmp_env, 1);
	}

	return buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
}

unsigned long read_short(FILE* in_wav)
{
	unsigned char buf[2];
	if (fread(buf, 1, 2, in_wav) != 2) {
		fprintf(stderr, "WAV: Short read!\n");
		longjmp(error_jmp_env, 1);
	}

	return buf[0] + (buf[1] << 8);
}

void read_header(FILE* in_wav, char* header)
{
	unsigned char buf[4];

	if (fread(buf, 1, 4, in_wav) != 4) {
		fprintf(stderr, "WAV: Short read!\n");
		longjmp(error_jmp_env, 1);
	}

	if (memcmp(buf, header, 4) != 0) {
		fprintf(stderr, "WAV: No %s header!\n", header);
		longjmp(error_jmp_env, 1);
	}
}

int parse_wave_header(FILE* in_wav, dv_enc_audio_info_t * res)
{
	unsigned char fmt_header_junk[1024];
	int header_len;

	if (setjmp(error_jmp_env)) {
		return -1;
	}

	read_header(in_wav, "RIFF");
	read_long(in_wav); /* ignore length, this is important,
			      since stream generated wavs do not have a
			      valid length! */

	read_header(in_wav, "WAVE");
	read_header(in_wav, "fmt ");
	header_len = read_long(in_wav);
	if (header_len > 1024) {
		fprintf(stderr, "WAV: Header too large!\n");
		return -1;
	}
	
	read_short(in_wav); /* format tag */
	res->channels = read_short(in_wav);
	res->frequency = read_long(in_wav);
	res->bytespersecond = read_long(in_wav); /* bytes per second */
	res->bytealignment = read_short(in_wav); /* byte alignment */
	res->bitspersample = read_short(in_wav);
	if (header_len - 16) {
		fread(fmt_header_junk, 1, header_len - 16, in_wav);
	}
	read_header(in_wav, "data");
	read_long(in_wav); /* ignore length, this is important,
			      since stream generated wavs do not have a
			      valid length! */

	switch (res->frequency) {
	case 48000:
	case 44100:
		if (res->channels != 2) {
			fprintf(stderr, 
				"WAV: Unsupported channel count (%d) for "
				"frequency %d!\n", res->channels,
				res->frequency);
			return(-1);
		}
		break;
	case 32000:
		if (res->channels != 4 && res->channels != 2) {
			fprintf(stderr, 
				"WAV: Unsupported channel count (%d) for "
				"frequency %d!\n", res->channels,
				res->frequency);
			return(-1);
		}
		break;
	default:
		fprintf(stderr, "WAV: Unsupported frequency: %d\n",
			res->frequency);
		return(-1);
	}
	if (res->bitspersample != 16) {
		fprintf(stderr, "WAV: Unsupported bitspersample: %d Only 16 "
			"bits are supported right now. (FIXME: just look in "
			"audio.c and copy the code if you "
			"really need this!)\n", res->bitspersample);
		return(-1);
	}

	return 0;
}

static void bytesperframe(dv_enc_audio_info_t * audio_info, int isPAL)
{
	audio_info->bytesperframe = audio_info->bytespersecond/(isPAL?25 : 30);
}

static FILE* audio_fp = NULL;

int wav_init(const char* filename, dv_enc_audio_info_t * audio_info)
{
	audio_fp = fopen(filename, "r");

	if (!audio_fp) {
		fprintf(stderr, "Can't open WAV file: %s\n", filename);
		return(-1);
	}

	if (parse_wave_header(audio_fp, audio_info)) {
		return(-1);
	}

	audio_converter = convert_s16_le;

	return(0);
}

void wav_finish()
{
	fclose(audio_fp);
}

int wav_load(dv_enc_audio_info_t * audio_info, int isPAL)
{
	int rval;
	unsigned char data[1920 * 2 * 2];

	bytesperframe(audio_info, isPAL);

	rval = (fread(data, 1, audio_info->bytesperframe, audio_fp) 
		!= audio_info->bytesperframe);
	if (!rval) {
		audio_converter(data, audio_info->data, 
				audio_info->bytesperframe / 2);
	}
	return rval;
}

#if HAVE_DEV_DSP

#define ioerror(msg, res) \
        if (res == -1) { \
                perror(msg); \
                return(-1); \
        }

static int audio_fd = -1;
static int audio_fmt = 0;

static int dsp_bytes_per_sample;

static int dsp_init(const char* filename, dv_enc_audio_info_t * audio_info)
{
	int frequencies[] = {48000, 44100, 32000, 0};
	int * p;

	audio_fd = open(filename, O_RDONLY);

	if (audio_fd == -1) {
		perror("Can't open /dev/dsp");
		return(-1);
	}

	ioerror("DSP_GETFMTS", ioctl(audio_fd, SNDCTL_DSP_GETFMTS,&audio_fmt));

	dsp_bytes_per_sample = 2*2;

	if (audio_fmt & AFMT_S16_BE) {
		audio_converter = convert_s16_be;
		audio_fmt = AFMT_S16_BE;
	} else if (audio_fmt & AFMT_S16_LE) {
		audio_converter = convert_s16_le;
		audio_fmt = AFMT_S16_LE;
	} else if (audio_fmt & AFMT_U16_BE) {
		audio_converter = convert_u16_be;
		audio_fmt = AFMT_U16_BE;
	} else if (audio_fmt & AFMT_U16_LE) {
		audio_converter = convert_u16_le;
		audio_fmt = AFMT_U16_LE;
	} else if (audio_fmt & AFMT_U8) {
		audio_converter = convert_u8;
		audio_fmt = AFMT_U8;
		dsp_bytes_per_sample = 1*2;
	} else {
		fprintf(stderr, "DSP: No supported audio format found for "
			"device %s!\n", filename);
		return(-1);
	}

	ioerror("DSP_SETFMT", ioctl(audio_fd, SNDCTL_DSP_SETFMT, &audio_fmt));

	audio_info->channels = 2;

	ioerror("DSP_CHANNELS", ioctl(audio_fd, SNDCTL_DSP_CHANNELS,
				      &audio_info->channels));

	for (p = frequencies; *p; p++) {
		audio_info->frequency = *p;

		ioerror("DSP_SPEED", ioctl(audio_fd, SNDCTL_DSP_SPEED,
					   &audio_info->frequency));
		if (audio_info->frequency == *p) {
			break;
		}
	}

	if (!*p) {
		fprintf(stderr, "DSP: No supported sampling rate found for "
			"device %s (48000, 44100 or 32000)!\n", filename);
		return(-1);
	}

	audio_info->bitspersample = 16;
	audio_info->bytespersecond = audio_info->frequency * 4;
	audio_info->bytealignment = 4;

	return 0;
}

static void dsp_finish()
{
	close(audio_fd);
}

static int dsp_load(dv_enc_audio_info_t * audio_info, int isPAL)
{
	int rval;
	unsigned char data[1920 * 2 * 2];
	int wanted = audio_info->bytesperframe * dsp_bytes_per_sample / 4;

	bytesperframe(audio_info, isPAL);

	rval = (read(audio_fd, data, wanted) != wanted);

	if (!rval) {
		audio_converter(data, audio_info->data, 
				audio_info->bytesperframe / 2);
	}
	return rval;
}

#endif

static dv_enc_audio_input_filter_t filters[DV_ENC_MAX_AUDIO_INPUT_FILTERS] = {
	{ wav_init, wav_finish, wav_load, "wav" },
#if HAVE_DEV_DSP
	{ dsp_init, dsp_finish, dsp_load, "dsp" },
#endif
	{ NULL, NULL, NULL, NULL }
};

void dv_enc_register_audio_input_filter(dv_enc_audio_input_filter_t filter)
{
	dv_enc_audio_input_filter_t * p = filters;
	while (p->filter_name) p++;
	*p++ = filter;
	p->filter_name = NULL;
}

int get_dv_enc_audio_input_filters(dv_enc_audio_input_filter_t ** filters_,
				   int * count)
{
	dv_enc_audio_input_filter_t * p = filters;

	*count = 0;
	while (p->filter_name) p++, (*count)++;

	*filters_ = filters;
	return 0;
}
