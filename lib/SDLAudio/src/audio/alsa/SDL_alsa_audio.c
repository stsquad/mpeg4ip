/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/



/* Allow access to a raw mixing buffer */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#include "Our_SDL_audio.h"
#include "SDL_audiomem.h"
#include "SDL_audio_c.h"
#include "SDL_alsa_audio.h"
#ifdef ALSA_DYNAMIC
#ifdef USE_DLVSYM
#define __USE_GNU
#endif
#include <dlfcn.h>
#include "SDL_name.h"
#include "SDL_loadso.h"
#else
#define SDL_NAME(X)	X
#endif

/* The tag name used by ALSA audio */
#define DRIVER_NAME         "alsa"

/* The default ALSA audio driver */
#define DEFAULT_DEVICE	"default"

/* Audio driver functions */
static int ALSA_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void ALSA_WaitAudio(_THIS);
static void ALSA_PlayAudio(_THIS);
static Uint8 *ALSA_GetAudioBuf(_THIS);
static void ALSA_CloseAudio(_THIS);
#ifdef ALSA_DYNAMIC

static const char *alsa_library = ALSA_DYNAMIC;
static void *alsa_handle = NULL;
static int alsa_loaded = 0;

static int (*SDL_snd_pcm_open)(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
static int (*SDL_NAME(snd_pcm_open))(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
static int (*SDL_NAME(snd_pcm_close))(snd_pcm_t *pcm);
static snd_pcm_sframes_t (*SDL_NAME(snd_pcm_writei))(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
static int (*SDL_NAME(snd_pcm_resume))(snd_pcm_t *pcm);
static int (*SDL_NAME(snd_pcm_prepare))(snd_pcm_t *pcm);
static int (*SDL_NAME(snd_pcm_drain))(snd_pcm_t *pcm);
static const char *(*SDL_NAME(snd_strerror))(int errnum);
static size_t (*SDL_NAME(snd_pcm_hw_params_sizeof))(void);
static int (*SDL_NAME(snd_pcm_hw_params_any))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
static int (*SDL_NAME(snd_pcm_hw_params_set_access))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t access);
static int (*SDL_NAME(snd_pcm_hw_params_set_format))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
static int (*SDL_NAME(snd_pcm_hw_params_set_channels))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
static int (*SDL_NAME(snd_pcm_hw_params_get_channels))(const snd_pcm_hw_params_t *params);
static unsigned int (*SDL_NAME(snd_pcm_hw_params_set_rate_near))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int *dir);
static snd_pcm_uframes_t (*SDL_NAME(snd_pcm_hw_params_set_period_size_near))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val, int *dir);
static unsigned int (*SDL_NAME(snd_pcm_hw_params_set_periods_near))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int *dir);
static int (*SDL_NAME(snd_pcm_hw_params))(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
static int (*SDL_NAME(snd_pcm_nonblock))(snd_pcm_t *pcm, int nonblock);
#define snd_pcm_hw_params_sizeof SDL_NAME(snd_pcm_hw_params_sizeof)
static int (*SDL_NAME(snd_pcm_delay))(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp);
static struct {
	const char *name;
	void **func;
} alsa_functions[] = {
	{ "snd_pcm_open",	(void**)&SDL_NAME(snd_pcm_open)		},
	{ "snd_pcm_close",	(void**)&SDL_NAME(snd_pcm_close)	},
	{ "snd_pcm_writei",	(void**)&SDL_NAME(snd_pcm_writei)	},
	{ "snd_pcm_resume",	(void**)&SDL_NAME(snd_pcm_resume)	},
	{ "snd_pcm_prepare",	(void**)&SDL_NAME(snd_pcm_prepare)	},
	{ "snd_pcm_drain",	(void**)&SDL_NAME(snd_pcm_drain)	},
	{ "snd_strerror",	(void**)&SDL_NAME(snd_strerror)		},
	{ "snd_pcm_hw_params_sizeof",		(void**)&SDL_NAME(snd_pcm_hw_params_sizeof)		},
	{ "snd_pcm_hw_params_any",		(void**)&SDL_NAME(snd_pcm_hw_params_any)		},
	{ "snd_pcm_hw_params_set_access",	(void**)&SDL_NAME(snd_pcm_hw_params_set_access)		},
	{ "snd_pcm_hw_params_set_format",	(void**)&SDL_NAME(snd_pcm_hw_params_set_format)		},
	{ "snd_pcm_hw_params_set_channels",	(void**)&SDL_NAME(snd_pcm_hw_params_set_channels)	},
	{ "snd_pcm_hw_params_get_channels",	(void**)&SDL_NAME(snd_pcm_hw_params_get_channels)	},
	{ "snd_pcm_hw_params_set_rate_near",	(void**)&SDL_NAME(snd_pcm_hw_params_set_rate_near)	},
	{ "snd_pcm_hw_params_set_period_size_near",	(void**)&SDL_NAME(snd_pcm_hw_params_set_period_size_near)	},
	{ "snd_pcm_hw_params_set_periods_near",	(void**)&SDL_NAME(snd_pcm_hw_params_set_periods_near)	},
	{ "snd_pcm_hw_params",	(void**)&SDL_NAME(snd_pcm_hw_params)	},
	{ "snd_pcm_nonblock",	(void**)&SDL_NAME(snd_pcm_nonblock)	},
	{ "snd_pcm_delay", (void **)&SDL_NAME(snd_pcm_delay) },
};

static void UnloadALSALibrary(void) {
	if (alsa_loaded) {
/*		SDL_UnloadObject(alsa_handle);*/
		dlclose(alsa_handle);
		alsa_handle = NULL;
		alsa_loaded = 0;
	}
}

static int LoadALSALibrary(void) {
	int i, retval = -1;

/*	alsa_handle = SDL_LoadObject(alsa_library);*/
	alsa_handle = dlopen(alsa_library,RTLD_NOW);
	if (alsa_handle) {
		alsa_loaded = 1;
		retval = 0;
		for (i = 0; i < SDL_TABLESIZE(alsa_functions); i++) {
/*			*alsa_functions[i].func = SDL_LoadFunction(alsa_handle,alsa_functions[i].name);*/
#ifdef USE_DLVSYM
			*alsa_functions[i].func = dlvsym(alsa_handle,alsa_functions[i].name,"ALSA_0.9");
			if (!*alsa_functions[i].func)
#endif
				*alsa_functions[i].func = dlsym(alsa_handle,alsa_functions[i].name);
			if (!*alsa_functions[i].func) {
				retval = -1;
				UnloadALSALibrary();
				break;
			}
		}
	}
	return retval;
}

#else

static void UnloadALSALibrary(void) {
	return;
}

static int LoadALSALibrary(void) {
	return 0;
}

#endif /* ALSA_DYNAMIC */
static int ALSA_AudioDelay(_THIS)
{
  snd_pcm_sframes_t delayp;

  delayp = 0;
  if (SDL_NAME(snd_pcm_delay)(pcm_handle, &delayp) > 0) {
    delayp /= this->spec.channels; // to get samples.
#if 0
    // below turns into msec
    if (delayp > 0) {
      delayp *= 1000;
      delayp /= this->spec.channels;
      delayp /= this->spec.freq;
    }
#endif
  }
  return delayp;
}
static const char *get_audio_device(int channels)
{
	const char *device;
	
	device = getenv("AUDIODEV");	/* Is there a standard variable name? */
	if ( device == NULL ) {
		if (channels == 6) device = "surround51";
		else if (channels == 4) device = "surround40";
		else device = DEFAULT_DEVICE;
	}
	return device;
}

/* Audio driver bootstrap functions */

static int Audio_Available(void)
{
	int available;
	int status;
	snd_pcm_t *handle;

	available = 0;
	if (LoadALSALibrary() < 0) {
		return available;
	}
	status = SDL_NAME(snd_pcm_open)(&handle, get_audio_device(2), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if ( status >= 0 ) {
		available = 1;
        	SDL_NAME(snd_pcm_close)(handle);
	}
	UnloadALSALibrary();
	return(available);
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	free(device->hidden);
	free(device);
	UnloadALSALibrary();
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	LoadALSALibrary();
	this = (SDL_AudioDevice *)malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			free(this);
		}
		return(0);
	}
	memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->OpenAudio = ALSA_OpenAudio;
	this->WaitAudio = ALSA_WaitAudio;
	this->PlayAudio = ALSA_PlayAudio;
	this->GetAudioBuf = ALSA_GetAudioBuf;
	this->CloseAudio = ALSA_CloseAudio;
	this->AudioDelay = ALSA_AudioDelay;
	this->free = Audio_DeleteDevice;

	return this;
}

AudioBootStrap ALSA_bootstrap_ours = {
	DRIVER_NAME, "ALSA 0.9 PCM audio",
	Audio_Available, Audio_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void ALSA_WaitAudio(_THIS)
{
	/* Check to see if the thread-parent process is still alive */
	{ static int cnt = 0;
		/* Note that this only works with thread implementations 
		   that use a different process id for each thread.
		*/
		if (parent && (((++cnt)%10) == 0)) { /* Check every 10 loops */
			if ( kill(parent, 0) < 0 ) {
				this->enabled = 0;
			}
		}
	}
}

static void ALSA_PlayAudio(_THIS)
{
	int           status;
	int           sample_len;
	signed short *sample_buf;

 	// okay, Veer.  this->spec.format may have AUDIO_FORMAT_HW_AC3,
 	// the length of the AC3 frame should be in this->mixbuffer_length
 	if (this->spec.format == AUDIO_FORMAT_HW_AC3) {
 	  sample_len = this->mixbuffer_length;
 	} else
	sample_len = this->spec.samples;
	sample_buf = (signed short *)mixbuf;
	while ( sample_len > 0 ) {
		status = SDL_NAME(snd_pcm_writei)(pcm_handle, sample_buf, sample_len);
		if ( status < 0 ) {
			if ( status == -EAGAIN ) {
				SDL_Delay(1);
				continue;
			}
			if ( status == -ESTRPIPE ) {
				do {
					SDL_Delay(1);
					status = SDL_NAME(snd_pcm_resume)(pcm_handle);
				} while ( status == -EAGAIN );
			}
			if ( status < 0 ) {
				status = SDL_NAME(snd_pcm_prepare)(pcm_handle);
			}
			if ( status < 0 ) {
				/* Hmm, not much we can do - abort */
				this->enabled = 0;
				return;
			}
			continue;
		}
		sample_buf += status * this->spec.channels;
		sample_len -= status;
	}
}

static Uint8 *ALSA_GetAudioBuf(_THIS)
{
	return(mixbuf);
}

static void ALSA_CloseAudio(_THIS)
{
	if ( mixbuf != NULL ) {
		SDL_FreeAudioMem(mixbuf);
		mixbuf = NULL;
	}
	if ( pcm_handle ) {
		SDL_NAME(snd_pcm_drain)(pcm_handle);
		SDL_NAME(snd_pcm_close)(pcm_handle);
		pcm_handle = NULL;
	}
}

static int ALSA_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	int                  status;
	snd_pcm_hw_params_t *params;
	snd_pcm_format_t     format;
	snd_pcm_uframes_t    frames;
	Uint16               test_format;

	/* Open the audio device */
	status = SDL_NAME(snd_pcm_open)(&pcm_handle, get_audio_device(spec->channels), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if ( status < 0 ) {
		SDL_SetError("Couldn't open audio device: %s", SDL_NAME(snd_strerror)(status));
		return(-1);
	}

	/* Figure out what the hardware is capable of */
	snd_pcm_hw_params_alloca(&params);
	status = SDL_NAME(snd_pcm_hw_params_any)(pcm_handle, params);
	if ( status < 0 ) {
		SDL_SetError("Couldn't get hardware config: %s", SDL_NAME(snd_strerror)(status));
		ALSA_CloseAudio(this);
		return(-1);
	}

	/* SDL only uses interleaved sample output */
	status = SDL_NAME(snd_pcm_hw_params_set_access)(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if ( status < 0 ) {
		SDL_SetError("Couldn't set interleaved access: %s", SDL_NAME(snd_strerror)(status));
		ALSA_CloseAudio(this);
		return(-1);
	}

	/* Try for a closest match on audio format */
	status = -1;
	for ( test_format = SDL_FirstAudioFormat(spec->format);
	      test_format && (status < 0); ) {
		switch ( test_format ) {
			case AUDIO_U8:
				format = SND_PCM_FORMAT_U8;
				break;
			case AUDIO_S8:
				format = SND_PCM_FORMAT_S8;
				break;
			case AUDIO_S16LSB:
				format = SND_PCM_FORMAT_S16_LE;
				break;
			case AUDIO_S16MSB:
				format = SND_PCM_FORMAT_S16_BE;
				break;
			case AUDIO_U16LSB:
				format = SND_PCM_FORMAT_U16_LE;
				break;
			case AUDIO_U16MSB:
				format = SND_PCM_FORMAT_U16_BE;
				break;
			default:
				format = 0;
				break;
		}
		if ( format != 0 ) {
			status = SDL_NAME(snd_pcm_hw_params_set_format)(pcm_handle, params, format);
		}
		if ( status < 0 ) {
			test_format = SDL_NextAudioFormat();
		}
	}
	if ( status < 0 ) {
		SDL_SetError("Couldn't find any hardware audio formats");
		ALSA_CloseAudio(this);
		return(-1);
	}
	spec->format = test_format;

	/* Set the number of channels */
	status = SDL_NAME(snd_pcm_hw_params_set_channels)(pcm_handle, params, spec->channels);
	if ( status < 0 ) {
		status = SDL_NAME(snd_pcm_hw_params_get_channels)(params);
		if ( (status <= 0) || (status > 2) ) {
			SDL_SetError("Couldn't set audio channels");
			ALSA_CloseAudio(this);
			return(-1);
		}
		spec->channels = status;
	}

	/* Set the audio rate */
	status = SDL_NAME(snd_pcm_hw_params_set_rate_near)(pcm_handle, params, spec->freq, NULL);
	if ( status < 0 ) {
		SDL_SetError("Couldn't set audio frequency: %s", SDL_NAME(snd_strerror)(status));
		ALSA_CloseAudio(this);
		return(-1);
	}
	spec->freq = status;

	/* Set the buffer size, in samples */
	frames = spec->samples;
	frames = SDL_NAME(snd_pcm_hw_params_set_period_size_near)(pcm_handle, params, frames, NULL);
	spec->samples = frames;
	SDL_NAME(snd_pcm_hw_params_set_periods_near)(pcm_handle, params, 2, NULL);

	/* "set" the hardware with the desired parameters */
	status = SDL_NAME(snd_pcm_hw_params)(pcm_handle, params);
	if ( status < 0 ) {
		SDL_SetError("Couldn't set audio parameters: %s", SDL_NAME(snd_strerror)(status));
		ALSA_CloseAudio(this);
		return(-1);
	}

	/* Calculate the final parameters for this audio specification */
	SDL_CalculateAudioSpec(spec);

	/* Allocate mixing buffer */
	mixlen = spec->size;
	mixbuf = (Uint8 *)SDL_AllocAudioMem(mixlen);
	if ( mixbuf == NULL ) {
		ALSA_CloseAudio(this);
		return(-1);
	}
	memset(mixbuf, spec->silence, spec->size);

	/* Get the parent process id (we're the parent of the audio thread) */
	parent = getpid();

	/* Switch to blocking mode for playback */
	SDL_NAME(snd_pcm_nonblock)(pcm_handle, 0);

	/* We're ready to rock and roll. :-) */
	return(0);
}
