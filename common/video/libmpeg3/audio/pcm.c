#include "mpeg3audio.h"
#include "../mpeg3private.inc"

int mpeg3audio_read_pcm_header(mpeg3audio_t *audio)
{
	unsigned int code;
	
	code = mpeg3bits_getbits(audio->astream, 16);
	while(!mpeg3bits_eof(audio->astream) && code != MPEG3_PCM_START_CODE)
	{
		code <<= 8;
		code &= 0xffff;
		code |= mpeg3bits_getbits(audio->astream, 8);
	}

	audio->avg_framesize = audio->framesize = 0x7db;
	audio->channels = 2;
	
	return mpeg3bits_eof(audio->astream);
}

int mpeg3audio_do_pcm(mpeg3audio_t *audio)
{
	int i, j, k;
	int16_t sample;
	int frame_samples = (audio->framesize - 3) / audio->channels / 2;

	if(mpeg3bits_read_buffer(audio->astream, audio->ac3_buffer, frame_samples * audio->channels * 2))
		return 1;

/* Need more room */
	if(audio->pcm_point / audio->channels >= audio->pcm_allocated - MPEG3AUDIO_PADDING * audio->channels)
	{
		mpeg3audio_replace_buffer(audio, audio->pcm_allocated + MPEG3AUDIO_PADDING * audio->channels);
	}

	k = 0;
	for(i = 0; i < frame_samples; i++)
	{
		for(j = 0; j < audio->channels; j++)
		{
			sample = ((int16_t)(audio->ac3_buffer[k++])) << 8;
			sample |= audio->ac3_buffer[k++];
			audio->pcm_sample[audio->pcm_point + i * audio->channels + j] = 
				(float)sample / 32767;
		}
	}
	audio->pcm_point += frame_samples * audio->channels;
	return 0;
}
