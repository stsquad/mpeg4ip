#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "mp4_decoder.h"
#include "global.h"
#include "divxif.h"

void newdec_init (get_t get, bookmark_t book, void *userdata)
{
  ld = &base;
  coeff_pred = &ac_dc;
  initbits(get, book, userdata);
}

int newdec_read_volvop (void)
{
  if (getvolhdr() == 0)
    return 0; // get vol header
  return 1;
}

void post_volprocessing (void)
{
  mp4_hdr.picnum = 0;
  mp4_hdr.mb_xsize = mp4_hdr.width / 16;
  mp4_hdr.mb_ysize = mp4_hdr.height / 16;
  mp4_hdr.mba_size = mp4_hdr.mb_xsize * mp4_hdr.mb_ysize;
	// set picture dimension global vars
	{
		horizontal_size = mp4_hdr.width;
		vertical_size = mp4_hdr.height;

		mb_width = horizontal_size / 16;
		mb_height = vertical_size / 16;

		coded_picture_width = horizontal_size + 64;
		coded_picture_height = vertical_size + 64;
		chrom_width = coded_picture_width >> 1;
		chrom_height = coded_picture_height >> 1;
	}

	// init decoder
	initdecoder();

}

int newdec_frame (unsigned char *y,
		  unsigned char *u,
		  unsigned char *v,
		  int wait_for_i)
{
  unsigned char *yuv[3];
  yuv[0] = y;
  yuv[1] = u;
  yuv[2] = v;
  if (getvophdr(wait_for_i) == 0)
    return (0);
  #if 0
  if (wait_for_i && mp4_hdr.prediction_type != I_VOP)
    return (0);
  #endif
  get_mp4picture((unsigned char *)yuv, 1);
  return (1);
}
  
