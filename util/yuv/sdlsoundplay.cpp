/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *              Bill May                wmay@cisco.com
 */
#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "mpeg4ip_sdl_includes.h"

static const char *usage = ": <options> <pcm file>\n"
"--freq <freq> -      set sample frequency (default 44100)\n"
"--channels <chans> - set number of channels (default 2)\n"
"--8-bit (-b) - use 8 bit data for PCM samples\n"
"--swap (-x) - swap 16 bit PCM data bytes\n"
"--version - display this message\n";

typedef struct userdata_t {
  FILE *m_infile;
  bool m_swap_bytes;
  bool m_have16bits;
  Uint8 *m_buffer;
  int m_volume;
  uint64_t m_bytes_played;
};

static void audio_callback (void *uptr, Uint8 *stream, int len)
{
  userdata_t *ud = (userdata_t *)uptr;
  int bytes_read;

  bytes_read = fread(ud->m_buffer, 1, len, ud->m_infile);
  if (bytes_read == 0) {
    SDL_PauseAudio(1);
    SDL_Event e;
    e.type = SDL_USEREVENT;
    SDL_PushEvent(&e);
    return;
  }
  if (ud->m_have16bits) {
    if (bytes_read & 0x1) {
      printf("error - read from file returned odd byte - truncating\n");
      bytes_read--;
    }
    if (ud->m_swap_bytes) {
      Uint8 *ptr = ud->m_buffer;
      for (int ix = 0; ix < bytes_read; ix += sizeof(uint16_t)) {
	Uint8 temp = ptr[0];
	ptr[0] = ptr[1];
	ptr[1] = temp;
	ptr += sizeof(uint16_t);
      }
    }
  }
  SDL_MixAudio(stream, ud->m_buffer, bytes_read, ud->m_volume);
  ud->m_bytes_played += bytes_read;
}

int main (int argc, char **argv)
{
  const char *ProgName = argv[0];
  uint32_t freq = 44100;
  uint32_t chans = 2;
  bool have_16bits = true;
  bool do_swap = false;
  int volume = 75;
  
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "freq", 1, 0, 'f' },
      { "channels", 1, 0, 'c' },
      { "8-bit", 0, 0, 'b' },
      { "swap", 0, 0, 'x' },
      { "version", 0, 0, 'v' },
      { "volume", 1, 0, 'V' },
      { NULL, 0, 0, 0 },
    };

    c = getopt_long_only(argc, argv, "f:c:bxv",
			 long_options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 'f':
      if (sscanf(optarg, "%u", &freq) != 1) {
	fprintf(stderr, "%s: can't read frequency from %s", ProgName, 
		optarg);
	exit(-1);
      }
      break;
    case 'c':
      if (sscanf(optarg, "%u", &chans) != 1) {
	fprintf(stderr, "%s: can't read channels from %s", ProgName, 
		optarg);
	exit(-1);
      }
      break;
    case 'V':
      if (sscanf(optarg, "%u", &volume) != 1) {
	fprintf(stderr, "%s: can't read volume from %s", ProgName, 
		optarg);
	exit(-1);
      }
      break;
    case 'b':
      have_16bits = false;
      break;
    case 'x':
      do_swap = true;
      break;
    case 'v':
      break;
    default:
      fprintf(stderr, "%s: unknown option specified: ignoring: %c\n",
	      ProgName, c);
    }
  }

  if ((argc - optind) != 1) {
    fprintf(stderr, "usage: %s %s", ProgName, usage);
    exit(1);
  }

  userdata_t ud;
  memset(&ud, 0, sizeof(ud));
  ud.m_volume = (volume * SDL_MIX_MAXVOLUME) / 100;

  ud.m_infile = fopen(argv[optind], FOPEN_READ_BINARY);
  if (ud.m_infile == NULL) {
    fprintf(stderr, "Couldn't open file %s\n", argv[optind]);
    exit(-1);
  }
  printf("%s - %s version %s\n", *argv, MPEG4IP_PACKAGE, MPEG4IP_VERSION);


  // we'll have a blank video window to capture events..
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
    printf("Could not init SDL audio: %s\n", SDL_GetError());
  }

  const char *temp = getenv("SDL_AUDIODRIVER");
  if (temp != NULL) {
    if (SDL_AudioInit(temp) < 0) {
      printf("Could not initialize %s audio driver\n", temp);
      exit(-1);
    }
  }
  char buffer[80];
  if (SDL_AudioDriverName(buffer, sizeof(buffer)) == NULL) {
    printf("Could not find SDL audio driver\n");
    exit(-1);
  }
  
  SDL_AudioSpec wanted, obtained;
  wanted.freq = freq;
  wanted.channels = chans;
  wanted.format = have_16bits ? AUDIO_S16 : AUDIO_U8;
  wanted.samples = 1024;
  wanted.callback = audio_callback;
  wanted.userdata = &ud;

  int ret = SDL_OpenAudio(&wanted, &obtained);
  if (ret < 0) {
    printf("Couldn't open audio: %s\n", SDL_GetError());
    SDL_Quit();
    exit(-1);
  }
  printf("wanted freq %u chans %u format %x samples %u\n", 
	 freq, chans, wanted.format, wanted.samples);
  printf("got    freq %u chans %u format %x samples %u size %u\n", 
	 obtained.freq, obtained.channels, obtained.format, 
	 obtained.samples, obtained.size);

  ud.m_buffer = (Uint8 *)malloc(obtained.size);
  ud.m_swap_bytes = do_swap;
  ud.m_have16bits = have_16bits;

  SDL_Surface *screen;
  SDL_Overlay *image;
  screen = SDL_SetVideoMode(320, 240, 24, SDL_SWSURFACE);
  image = SDL_CreateYUVOverlay(320, 240, SDL_YV12_OVERLAY, screen);

  SDL_PauseAudio(0);
  bool finished = false;
  while (finished == false) {
    SDL_Event our_event;
    if (SDL_WaitEvent(&our_event) > 0) {
      switch (our_event.type) {
      case SDL_QUIT:
	SDL_PauseAudio(1);
	finished = true;
	break;
      case SDL_USEREVENT:
	finished = true;
	break;
      }
    } else {
      printf("error while waiting for events\n");
    }
  }
  
  SDL_CloseAudio();
  if (image) {
    SDL_FreeYUVOverlay(image);
  }
  if (screen) SDL_FreeSurface(screen);
  double time_played = ud.m_bytes_played;
  time_played /= chans;
  if (ud.m_have16bits) {
    time_played /= 2;
  }

  time_played /= freq;

  printf("played "U64" bytes %g seconds\n", ud.m_bytes_played, time_played);
  free(ud.m_buffer);
  SDL_Quit();
  return (0);
}
