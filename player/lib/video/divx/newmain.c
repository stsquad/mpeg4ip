#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "mp4_decoder.h"
#include "global.h"
#include "divxif.h"

void newdec_init (get_more_t get, void *userdata)
{
  ld = &base;
  coeff_pred = &ac_dc;
  initbits(get, userdata);
}

int newdec_read_volvop (unsigned char *buffer, unsigned int buflen)
{
  init_frame_bits(buffer, buflen);
  while (getvolhdr() != 1); // should exception out if not found...
  return 1; // get vol header
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
		  int wait_for_i,
		  unsigned char *buffer,
		  unsigned int buflen)
{
  unsigned char *yuv[3];
  yuv[0] = y;
  yuv[1] = u;
  yuv[2] = v;
  init_frame_bits(buffer, buflen);
  if (getvophdr() == 0)
    return (0 - ld->incnt);
  #if 0
  if (wait_for_i && mp4_hdr.prediction_type != I_VOP) {
    printf("wfi !IVOP\n");
    return (0 - ld->incnt);
  }
  #endif
  get_mp4picture((unsigned char *)yuv, 1);
  if (ld->bitcnt != 0) {
    ld->incnt++;
    ld->bitcnt = 0;
  }
  return (ld->incnt);
}

int newget_bytes_used (void)
{
  if (ld->bitcnt != 0) {
    ld->bitcnt = 0;
    ld->incnt++;
  }
  return (ld->incnt);
}
