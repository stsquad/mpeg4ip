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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 *
 * Contributor(s):
 *              Bill May                wmay@cisco.com
 */
#include "mpeg4ip.h"
#include "mpeg4ip_sdl_includes.h"

int main (int argc, char **argv)
{
  FILE *yuvfile;
  uint8_t *ybuf, *ubuf, *vbuf;
  uint32_t height = 240, width = 320, ysize, uvsize, readbytes, rate = 30*100;
  char buf[32];

  printf("%s - %s version %s\n", *argv, MPEG4IP_PACKAGE, MPEG4IP_VERSION);

  argc--;
  argv++;
  if (argc <= 1) {
    printf("usage: yuvdump -h [height] -w [width] -r [frameRate/(perSecond/100)] fileName\n");
    return 0;
  }

  while (argv[0][0] == '-') {
    switch (argv[0][1]) {
    case 'h':
      argv++;
      argc--;
      height = atoi(*argv);
      argv++;
      argc--;
      break;
    case 'w':
      argv++;
      argc--;
      width = atoi(*argv);
      argv++;
      argc--;
      break;
    case 'r':
      argv++;
      argc--;
      rate = atoi(*argv);
      argv++;
      argc--;
      break;
    default:
      printf("Unknown option %s", *argv);
      exit(-1);
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0 || !SDL_VideoDriverName(buf, 1)) {
    printf("Could not init SDL video: %s\n", SDL_GetError());
  }

  if (*argv == NULL) return(0);

  const SDL_VideoInfo *video_info;
  int video_bpp;
  video_info = SDL_GetVideoInfo();

  switch (video_info->vfmt->BitsPerPixel) {
  case 16:
  case 32:
    video_bpp = video_info->vfmt->BitsPerPixel;
    break;
  default:
    video_bpp = 16;
    break;
  }
  SDL_Surface *m_screen = SDL_SetVideoMode(width,
					   height,
					   32,
					   SDL_SWSURFACE | SDL_ASYNCBLIT);
  SDL_Rect m_dstrect;

  m_dstrect.x = 0;
  m_dstrect.y = 0;
  m_dstrect.w = m_screen->w;
  m_dstrect.h = m_screen->h;
  SDL_Overlay *m_image = SDL_CreateYUVOverlay(width,
					      height,
					      SDL_YV12_OVERLAY, 
					      m_screen);

  yuvfile = fopen(*argv, FOPEN_READ_BINARY);
  ysize = width*height;
  uvsize = ysize / 4;
  printf("ysize %u uvsize %u\n", ysize, uvsize);
  ybuf = (uint8_t *)malloc(ysize);
  ubuf = (uint8_t *)malloc(ysize);
  vbuf = (uint8_t *)malloc(ysize);

  unsigned int fcount = 0;
  int qevent = 0;
  do {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	qevent = 1;
      }
    }
    readbytes = fread(ybuf, ysize,  sizeof(uint8_t), yuvfile);
    if (ferror(yuvfile) || readbytes != 1) {
      if (feof(yuvfile) && !ferror(yuvfile)) {
	continue;
      }
      printf("frame %u - y buf read error\n", fcount);
      continue;
    }
    readbytes = fread(ubuf, uvsize,  sizeof(uint8_t), yuvfile);
    if (ferror(yuvfile) || readbytes != 1) {
      printf("frame %u - u buf read error\n", fcount);
      continue;
    }
    readbytes = fread(vbuf, uvsize,  sizeof(uint8_t), yuvfile);
    if (ferror(yuvfile) || readbytes != 1) {
      printf("frame %u - v buf read error\n", fcount);
      continue;
    }
    SDL_LockYUVOverlay(m_image);
    memcpy(m_image->pixels[0], ybuf, ysize);
    memcpy(m_image->pixels[1], vbuf, uvsize);
    memcpy(m_image->pixels[2], ubuf, uvsize);

    SDL_DisplayYUVOverlay(m_image, &m_dstrect);
    SDL_UnlockYUVOverlay(m_image);

    SDL_Delay( 1000 * 100 / rate );

    fcount++;

  } while (qevent == 0 && !feof(yuvfile));

  SDL_Delay(2000);
  printf("Frames read %u\n", fcount);
  SDL_FreeYUVOverlay(m_image);
  SDL_FreeSurface(m_screen);
  SDL_Quit();
  fclose(yuvfile);
  return (0);
}
