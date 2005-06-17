#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpeg4ip_sdl_includes.h"
#include <unistd.h>
int main (int argc, char **argv)
{
  FILE *yuvfile;
  uint8_t *ybuf, *ubuf, *vbuf, *temp;
  size_t height = 240, width = 320, ysize, uvsize;
  char yval, uval, vval;
  argc--;
  argv++;

  if (argc != 5) 
    return(0);

  ysize = width*height;
  uvsize = ysize / 4;
  yuvfile = fopen(*argv, "w");
  argv++;

  int frames = atoi(*argv);
  yval = atoi(*argv);
  uval = atoi(*argv);
  vval = atoi(*argv);
  printf("Y %x not %x", yval, ~yval);
  ybuf = (uint8_t *)malloc(ysize);
  temp = ybuf;
  for (size_t ix = 0; ix < height; ix++) {
    memset(temp, yval, width / 2);
    memset(temp + (width/2), ~yval, width / 2);
    temp += width;
    if (ix == height / 2) yval = ~yval;
  }

  ubuf = (uint8_t *)malloc(ysize);
  temp = ubuf;
  for (size_t ix = 0; ix < height/2; ix++) {
    memset(temp, uval, width / 4);
    memset(temp + (width/4), ~uval, width / 4);
    if (ix == height / 4) uval = ~uval;
    temp += width/2;
  }
    

  vbuf = (uint8_t *)malloc(ysize);
  temp = vbuf;
  for (size_t ix = 0; ix < height/2; ix++) {
    memset(temp, vval, width / 4);
    memset(temp + (width/4), ~vval, width / 4);
    if (ix == height / 4) vval = ~vval;
    temp += width/2;
  }

  for (int ix = 0; ix < frames; ix++) {
    fwrite(ybuf, ysize, sizeof(uint8_t), yuvfile);
    fwrite(ubuf, uvsize, sizeof(uint8_t), yuvfile);
    fwrite(vbuf, uvsize, sizeof(uint8_t), yuvfile);

  }
  
  fclose(yuvfile);
  return 0;
}
