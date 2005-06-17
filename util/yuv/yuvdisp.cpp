#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpeg4ip_sdl_includes.h"
#include <unistd.h>
int main (int argc, char **argv)
{
  size_t height = 240, width = 320, ysize, uvsize;
  int yval, uval, vval;
  char buf[32];
  argc--;
  argv++;

  if (SDL_Init(SDL_INIT_VIDEO) < 0 || !SDL_VideoDriverName(buf, 1)) {
    printf("Could not init SDL video: %s\n", SDL_GetError());
  }

  if (argc != 3)
    return(0);

  yval = atoi(*argv);
  argv++;
  uval = atoi(*argv);
  argv++;
  vval = atoi(*argv);

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
					   video_bpp,
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


  ysize = width*height;
  uvsize = ysize / 4;
  SDL_LockYUVOverlay(m_image);
  memset(m_image->pixels[0], yval, ysize);
  memset(m_image->pixels[1], vval, uvsize);
  memset(m_image->pixels[2], uval, uvsize);
  SDL_DisplayYUVOverlay(m_image, &m_dstrect);
  SDL_UnlockYUVOverlay(m_image);
  printf("%d %d %d\n", yval, uval, vval);
  sleep(1);

  SDL_FreeYUVOverlay(m_image);
  SDL_FreeSurface(m_screen);
  SDL_Quit();
  return 0;
}
