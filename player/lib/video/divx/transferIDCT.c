/**************************************************************************
 *                                                                        *
 * This code has been developed by John Funnell. This software is an      *
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
 * John Funnell
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// transferIDCT.c //

/* C version to be optimised */

/* Functions to add or copy the result of the iDCT into the output       */
/* frame buffer.  The "clear block" function could be absorbed into this */
/* loop (set *ouputPtr = 0 after the copy/add).                          */


#include "portab.h"

void transferIDCT_add(int16_t *sourceS16, uint8_t *destU8, int stride) {
	int x, y;

	stride -= 8;	
	for (y=0; y<8; y++) {
		for (x=0; x<8; x++) {
			#define SUM16 (*(destU8) + *(sourceS16))
			if      (SUM16 > 255) *(destU8) = 255;
			else if (SUM16 <   0) *(destU8) =   0;
			else                  *(destU8) = SUM16;
			sourceS16++;
			destU8++;
		}
		destU8 += stride;
	}
}

void transferIDCT_copy(int16_t *sourceS16, uint8_t *destU8, int stride) {
	int x, y;

	stride -= 8;	
	for (y=0; y<8; y++) {
		for (x=0; x<8; x++) {
			if      (*(sourceS16) > 255) *(destU8) = 255;
			else if (*(sourceS16) <   0) *(destU8) =   0;
			else                         *(destU8) = (unsigned char) *(sourceS16);
			sourceS16++;
			destU8++;
		}
		destU8 += stride;
	}
}
