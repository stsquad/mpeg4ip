#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
int main (int argc, char **argv)
{
  FILE *yuvfile;
  uint8_t *ybuf, *ubuf, *vbuf;
  uint32_t height = 240, width = 320, ysize, uvsize, readbytes;
  char buf[32];
  argc--;
  argv++;

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
    default:
      printf("Unknown option %s", *argv);
      exit(-1);
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0 || !SDL_VideoDriverName(buf, 1)) {
    printf("Could not init SDL video: %s\n", SDL_GetError());
  }

  if (*argv == NULL) 
    return(0);
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

  yuvfile = fopen(*argv, "r");
  ysize = width*height;
  uvsize = ysize / 4;
  printf("ysize %u uvsize %u\n", ysize, uvsize);
  ybuf = (uint8_t *)malloc(ysize);
  ubuf = (uint8_t *)malloc(ysize);
  vbuf = (uint8_t *)malloc(ysize);

  uint32_t fcount = 0;
  do {
    fcount++;
    readbytes = fread(ybuf, ysize,  sizeof(uint8_t), yuvfile);
    if (readbytes != 1) {
      printf("frame %u - y buf read error\n", fcount);
      continue;
    }
    readbytes = fread(ubuf, uvsize,  sizeof(uint8_t), yuvfile);
    if (readbytes != 1) {
      printf("frame %u - u buf read error\n", fcount);
      continue;
    }
    readbytes = fread(vbuf, uvsize,  sizeof(uint8_t), yuvfile);
    if (readbytes != 1) {
      printf("frame %u - v buf read error\n", fcount);
      continue;
    }
    SDL_LockYUVOverlay(m_image);
    memcpy(m_image->pixels[0], ybuf, ysize);
    memcpy(m_image->pixels[1], vbuf, uvsize);
    memcpy(m_image->pixels[2], ubuf, uvsize);

    SDL_DisplayYUVOverlay(m_image, &m_dstrect);
    SDL_UnlockYUVOverlay(m_image);
    //SDL_Delay(33);
    //printf("%u\n", fcount);
  } while (!feof(yuvfile));

  SDL_Delay(5000);
  printf("Frames read %u\n", fcount);
  SDL_FreeYUVOverlay(m_image);
  SDL_FreeSurface(m_screen);
  SDL_Quit();
  fclose(yuvfile);
  return (0);
}
