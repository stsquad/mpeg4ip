/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// mp4_decoder.c //

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#endif

#define GLOBAL
#include "mp4_decoder.h"
#include "global.h"

/**
 *
**/

void get_mp4picture (unsigned char *bmp, int render_flag); 
void initdecoder ();
void closedecoder ();

/***/

void initdecoder ()
{
  int i, j, cc, size;

  /* clip table */
	clp = (unsigned char *) malloc (sizeof(unsigned char) * 1024);
  if (! clp) {
    printf ("malloc failed\n");
		exit(0);
	}
  clp += 384;
  for (i = -384; i < 640; i++)
    clp[i] = (unsigned char) ( (i < 0) ? 0 : ((i > 255) ? 255 : i) );
 
	/* dc prediction border */
	for (i = 0; i < (2*MBC+1); i++)
		coeff_pred->dc_store_lum[0][i] = 1024;

	for (i = 1; i < (2*MBR+1); i++)
		coeff_pred->dc_store_lum[i][0] = 1024;

	for (i = 0; i < (MBC+1); i++) {
		coeff_pred->dc_store_chr[0][0][i] = 1024;
		coeff_pred->dc_store_chr[1][0][i] = 1024;
	}

	for (i = 1; i < (MBR+1); i++) {
		coeff_pred->dc_store_chr[0][i][0] = 1024;
		coeff_pred->dc_store_chr[1][i][0] = 1024;
	}

	/* ac prediction border */
	for (i = 0; i < (2*MBC+1); i++)
		for (j = 0; j < 7; j++)	{
			coeff_pred->ac_left_lum[0][i][j] = 0;
			coeff_pred->ac_top_lum[0][i][j] = 0;
		}

	for (i = 1; i < (2*MBR+1); i++)
		for (j = 0; j < 7; j++)	{
			coeff_pred->ac_left_lum[i][0][j] = 0;
			coeff_pred->ac_top_lum[i][0][j] = 0;
		}

	/* 
			[Review] too many computation to access to the 
			correct array value, better use two different
			pointer for Cb and Cr components
	*/
	for (i = 0; i < (MBC+1); i++)
		for (j = 0; j < 7; j++)	{
			coeff_pred->ac_left_chr[0][0][i][j] = 0; 
			coeff_pred->ac_top_chr[0][0][i][j] = 0;
			coeff_pred->ac_left_chr[1][0][i][j] = 0;
			coeff_pred->ac_top_chr[1][0][i][j] = 0;
		}

	for (i = 1; i < (MBR+1); i++)
		for (j = 0; j < 7; j++)	{
			coeff_pred->ac_left_chr[0][i][0][j] = 0;
			coeff_pred->ac_top_chr[0][i][0][j] = 0;
			coeff_pred->ac_left_chr[1][i][0][j] = 0;
			coeff_pred->ac_top_chr[1][i][0][j] = 0;
		}

	/* mode border */
	for (i = 0; i < mb_width + 1; i++)
		modemap[0][i] = INTRA;
	for (i = 0; i < mb_height + 1; i++) {
		modemap[i][0] = INTRA;
		modemap[i][mb_width+1] = INTRA;
	}

	// edged forward and reference frame
  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
    {
      size = coded_picture_width * coded_picture_height;

			edged_ref[cc] = (unsigned char *) malloc (size);
      if (! edged_ref[cc]) {
        printf ("malloc failed\n");
				exit(0);
			}
			edged_for[cc] = (unsigned char *) malloc (size);
      if (! edged_for[cc]) {
        printf ("malloc failed\n");
				exit(0);
			}
      frame_ref[cc] = edged_ref[cc] + coded_picture_width * 32 + 32;
      frame_for[cc] = edged_for[cc] + coded_picture_width * 32 + 32;
    } 
    else
    {
      size = chrom_width * chrom_height;

			edged_ref[cc] = (unsigned char *) malloc (size);
      if (! edged_ref[cc]) {
        printf ("malloc failed\n");
				exit(0);
			}
			edged_for[cc] = (unsigned char *) malloc (size);
      if (! edged_for[cc]) {
        printf ("malloc failed\n");
				exit(0);
			}
      frame_ref[cc] = edged_ref[cc] + chrom_width * 16 + 16;
      frame_for[cc] = edged_for[cc] + chrom_width * 16 + 16;
    }
  }

	// display frame
	for (cc = 0; cc < 3; cc++) 
	{
		if (cc == 0)
			size = horizontal_size * vertical_size;
		else
			size = (horizontal_size * vertical_size) >> 2;

		display_frame[cc] = (unsigned char *) malloc (size);
		if (! display_frame[cc]) {
			printf("malloc failed\n");
			exit(0);
		}
	}

#ifndef _MMX_IDCT
  init_idct ();
#endif // _DEBUG
}

/***/

void closedecoder ()
{
	int cc;

	clp -= 384;
	free(clp);

	for (cc = 0; cc < 3; cc++) {
		free(display_frame[cc]);
		free(edged_ref[cc]);
		free(edged_for[cc]);
	}
}
