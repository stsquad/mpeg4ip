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
// mp4_recon.c //

#include <stdio.h>
#include <stdlib.h>

#include "mp4_decoder.h"
#include "global.h"

#include "basic_prediction.h"

/**
 *
**/

static __inline void recon_comp (unsigned char *src, unsigned char *dst, int lx, int w, int h, int x, int y, int dx, int dy, int flag);

/***/

void reconstruct (int bx, int by, int mode)
{
  int w, h, lx, dx, dy, xp, yp, comp, sum;
  int x, y, px, py;
  unsigned char * src[3];
	
  x = bx + 1;
  y = by + 1;

  lx = coded_picture_width;

  src[0] = frame_for[0];
  src[1] = frame_for[1];
  src[2] = frame_for[2];

	w = 8;
	h = 8;

	// Luma
	px = bx << 4;
	py = by << 4;
	if (mode == INTER4V)
	{
		for (comp = 0; comp < 4; comp++)
		{
			dx = MV[0][comp][y][x];
			dy = MV[1][comp][y][x];
			
			xp = px + ((comp & 1) << 3);
			yp = py + ((comp & 2) << 2);

			recon_comp (src[0], frame_ref[0], lx, w, h, xp, yp, dx, dy, 0);
		}
	} else
	{
		dx = MV[0][0][y][x];
		dy = MV[1][0][y][x];

		recon_comp (src[0], frame_ref[0], lx, w << 1, h << 1, px, py, dx, dy, 0);
	}
	
	// Chr
	px = bx << 3;
	py = by << 3;
	if (mode == INTER4V)
	{
		sum = MV[0][0][y][x] + MV[0][1][y][x] + MV[0][2][y][x] + MV[0][3][y][x];
		if (sum == 0) 
			dx = 0;
		else
			dx = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);
		
		sum = MV[1][0][y][x] + MV[1][1][y][x] + MV[1][2][y][x] + MV[1][3][y][x];
		if (sum == 0)
			dy = 0;
		else
			dy = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);
		
	} else
	{
		dx = MV[0][0][y][x];
		dy = MV[1][0][y][x];
		/* chroma rounding */
		dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
		dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);
	}
	lx >>= 1;

	recon_comp (src[1], frame_ref[1], lx, w, h, px, py, dx, dy, 1);
	recon_comp (src[2], frame_ref[2], lx, w, h, px, py, dx, dy, 2);
}

/***/

static __inline void recon_comp (unsigned char *src, unsigned char *dst, 
                        int lx, int w, int h, int x, 
                        int y, int dx, int dy, int chroma)
{
  int xint, xh, yint, yh;
  unsigned char *s, *d;

  xint = dx >> 1;
  xh = dx & 1;
  yint = dy >> 1;
  yh = dy & 1;

  /* origins */
  s = src + lx * (y + yint) + x + xint;
  d = dst + lx * y + x;

	{
		int mc_driver = ((w!=8)<<3) | (mp4_hdr.rounding_type<<2) | (yh<<1) | (xh);
		switch (mc_driver)
		{
		// block
			 // no round
			case 0: case 4:
 			  CopyBlock(s, d, lx);
			  break;
			case 1:
				CopyBlockHor(s, d, lx);
				break;
			case 2:
				CopyBlockVer(s, d, lx);
				break;
			case 3:
				CopyBlockHorVer(s, d, lx);
				break;
				// round
			case 5:
				CopyBlockHorRound(s, d, lx);
				break;
			case 6:
				CopyBlockVerRound(s, d, lx);
				break;
			case 7:
				CopyBlockHorVerRound(s, d, lx);
				break;
		// macroblock
			// no round
			case 8: case 12:
				CopyMBlock(s, d, lx);
				break;
			case 9:
				CopyMBlockHor(s, d, lx);
				break;
			case 10:
				CopyMBlockVer(s, d, lx);
				break;
			case 11:
				CopyMBlockHorVer(s, d, lx);
				break;
			// round
			case 13:
				CopyMBlockHorRound(s, d, lx);
				break;
			case 14:
				CopyMBlockVerRound(s, d, lx);
				break;
			case 15:
				CopyMBlockHorVerRound(s, d, lx);
				break;
		}
	}
}

