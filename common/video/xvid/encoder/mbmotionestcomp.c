/**************************************************************************
 *                                                                        *
 * This code is developed by Eugene Kuznetsov.  This software is an       *
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
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/


/**************************************************************************
 *
 *  MBMotionEstComp.c, motion estimation/compensation module
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Eugene Kuznetsov
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/ 

/**************************************************************************
 *
 *  Modifications:
 *
 *  31.12.2001 PMVfastSearch16 and PMVfastSearch8 (gruel)
 *	30.12.2001 get_range/MotionSearchX simplified; blue/green bug fix
 *	22.12.2001 commented best_point==99 check
 *	19.12.2001 modified get_range (purple bug fix)
 *  15.12.2001 moved pmv displacement from mbprediction
 *  02.12.2001 motion estimation/compensation split (Isibaar)
 *	16.11.2001 rewrote/tweaked search algorithms; pross@cs.rmit.edu.au
 *  10.11.2001 support for sad16/sad8 functions
 *  28.08.2001 reactivated MODE_INTER4V for EXT_MODE
 *  24.08.2001 removed MODE_INTER4V_Q, disabled MODE_INTER4V for EXT_MODE
 *	22.08.2001 added MODE_INTER4V_Q			
 *  20.08.2001 added pragma to get rid of internal compiler error with VC6
 *             idea by Cyril. Thanks.
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include <assert.h>
#include <stdio.h>

#include "encoder.h"
#include "mbfunctions.h"
#include "../common/common.h"
#include "../common/timer.h"
#include "sad.h"

// very large value
#define MV_MAX_ERROR	(4096 * 256)

// stop search if sdelta < THRESHOLD
#define MV16_THRESHOLD	192
#define MV8_THRESHOLD	56

/* sad16(0,0) bias; mpeg4 spec suggests nb/2+1 */
/* nb  = vop pixels * 2^(bpp-8) */
#define MV16_00_BIAS	(128+1)

/* INTER bias for INTER/INTRA decision; mpeg4 spec suggests 2*nb */
#define INTER_BIAS	432

/* Parameters which control inter/inter4v decision */
#define IMV16X16			5

/* vector map (vlc delta size) smoother parameters */
#define NEIGH_TEND_16X16	2
#define NEIGH_TEND_8X8		2


// fast ((A)/2)*2
#define EVEN(A)		(((A)<0?(A)+1:(A)) & ~1)


#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define ABS(X) (((X)>0)?(X):-(X))
#define SIGN(X) (((X)>0)?1:-1)


int32_t PMVfastSearch8(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int start_x, int start_y,
					const uint32_t iQuality, 
					MBParam * const pParam,
					MACROBLOCK * const pMBs,				
					VECTOR * const currMV,
					VECTOR * const currPMV);

int32_t PMVfastSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t iQuality, 
					MBParam * const pParam,
					MACROBLOCK * const pMBs,				
					VECTOR * const currMV,
					VECTOR * const currPMV);




/* diamond search stuff
   keep the the sequence in circular order (so optimization works)
*/

typedef struct
{
	int32_t dx;
	int32_t dy;
}
DPOINT;


static const DPOINT diamond_small[4] = 
{
	{0, 1}, {1, 0}, {0, -1}, {-1, 0}
};


static const DPOINT diamond_large[8] =
{
	{0, 2}, {1, 1}, {2, 0}, {1, -1}, {0, -2}, {-1, -1}, {-2, 0}, {-1, 1}
};


// mv.length table
static const uint32_t mvtab[33] = {
    1,  2,  3,  4,  6,  7,  7,  7,
    9,  9,  9,  10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 11, 11, 11, 11, 11, 11, 12, 12
};


static __inline uint32_t mv_bits(int32_t component, const uint32_t iFcode)
{
    if (component == 0)
		return 1;

    if (component < 0)
		component = -component;

    if (iFcode == 1)
    {
		if (component > 32)
		    component = 32;

		return mvtab[component] + 1;
    }

    component += (1 << (iFcode - 1)) - 1;
    component >>= (iFcode - 1);

    if (component > 32)
		component = 32;

    return mvtab[component] + 1 + iFcode - 1;
}


static __inline uint32_t calc_delta_16(const int32_t dx, const int32_t dy, const uint32_t iFcode)
{
	return NEIGH_TEND_16X16 * (mv_bits(dx, iFcode) + mv_bits(dy, iFcode));
}

static __inline uint32_t calc_delta_8(const int32_t dx, const int32_t dy, const uint32_t iFcode)

{
    return NEIGH_TEND_8X8 * (mv_bits(dx, iFcode) + mv_bits(dy, iFcode));
}




/* calculate the min/max range (in halfpixels)
	relative to the _MACROBLOCK_ position
*/

static void __inline get_range(
			int32_t * const min_dx, int32_t * const max_dx,
			int32_t * const min_dy, int32_t * const max_dy,
			const uint32_t x, const uint32_t y, 
			const uint32_t block_sz,					// block dimension, 8 or 16
			const uint32_t width, const uint32_t height,
			const uint32_t fcode)
{
	const int search_range = 32 << (fcode - 1);
    const int high = search_range - 1;
    const int low = -search_range;

	// convert full-pixel measurements to half pixel
	const int hp_width = 2 * width;
	const int hp_height = 2 * height;
	const int hp_edge = 2 * block_sz;
	const int hp_x = 2 * (x) * block_sz;		// we need _right end_ of block, not x-coordinate
	const int hp_y = 2 * (y) * block_sz;		// same for _bottom end_

    *max_dx = MIN(high,	hp_width - hp_x);
    *max_dy = MIN(high,	hp_height - hp_y);
    *min_dx = MAX(low,	-(hp_edge + hp_x));
    *min_dy = MAX(low,	-(hp_edge + hp_y));
}


/* getref: calculate reference image pointer 
the decision to use interpolation h/v/hv or the normal image is
based on dx & dy.
*/

static __inline const uint8_t * get_ref(
				const uint8_t * const refn,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, const uint32_t y,
				const uint32_t block,					// block dimension, 8 or 16
				const int32_t dx, const int32_t dy,
				const uint32_t stride)
{
	switch ( ((dx&1)<<1) + (dy&1) )		// ((dx%2)?2:0)+((dy%2)?1:0)
    {
	case 0 : return refn + (x*block+dx/2) + (y*block+dy/2)*stride;
    case 1 : return refv + (x*block+dx/2) + (y*block+(dy-1)/2)*stride;
	case 2 : return refh + (x*block+(dx-1)/2) + (y*block+dy/2)*stride;
	default : 
	case 3 : return refhv + (x*block+(dx-1)/2) + (y*block+(dy-1)/2)*stride;
	}
}



// ********************************************************************

/**
    Search for a single motion vector corresponding to macroblock (x,y),
    starting from motion vector <pred_x, pred_y> ( in half-pixels ),
    with search window determined by search_range & iFcode.
    Store the result in '*pmv' and return optimal SAD.
     pRef - reconstructed image
     pRefH - reconstructed image, interpolated along H axis.
     pRefV, 
	 pRefHV - same as above
**/

static int32_t MotionSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int iQuality, 
					MBParam * const pParam,
					MACROBLOCK * const pMBs,				
					VECTOR * const currMV,
					VECTOR * const currPMV)

{
	const DPOINT * diamond;

	const int32_t iFcode = pParam->fixed_code;
	const int32_t iQuant = pParam->quant;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width; 

	const uint8_t * cur = pCur->y + x*16 + y*16*iEdgedWidth;

	int32_t min_dx, max_dx;
    int32_t min_dy, max_dy;
	int32_t dx, dy;
	int32_t center_dx, center_dy;
	int32_t pred_x,pred_y;

    int32_t best_sdelta;
    int32_t best_dx, best_dy;
	
	int32_t best_sdelta2;
	int32_t best_dx2, best_dy2;
	
    uint32_t point;
	uint32_t best_point;
    uint32_t count;
  
    uint32_t first_pass = (iQuality > 4);

	get_pmv(pMBs, x, y, pParam->mb_width, 0, &pred_x, &pred_y);    

	get_range(&min_dx, &max_dx, &min_dy, &max_dy,
			x, y, 16, iWidth, iHeight, iFcode);

	// for 1st pass search, center=(0,0)

	center_dx = 0;
	center_dy = 0;

	// set sdelta2 to max (since we may not do 2nd pass)

	best_sdelta2 = MV_MAX_ERROR;
    best_dx2 = center_dx;
    best_dy2 = center_dy;

  start:
  	if (center_dx > max_dx) center_dx = max_dx;
	else if (center_dx < min_dx) center_dx = min_dx;
	if (center_dy > max_dy) center_dy = max_dy;
	else if (center_dy < min_dy) center_dy = min_dy;

	// sad/delta for center

	best_sdelta = sad16(cur,
				get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, center_dx, center_dy, iEdgedWidth),
				iEdgedWidth, 
				MV_MAX_ERROR);
	if (center_dx == 0 && center_dy == 0 && best_sdelta <= iQuant * 96)
	{
		best_sdelta -= MV16_00_BIAS;
	}
	best_sdelta += calc_delta_16(center_dx - pred_x, center_dy - pred_y, iFcode) * iQuant;
	best_dx = center_dx;
	best_dy = center_dy;

	diamond = diamond_large;
    point = 0;
    count = 8;
    best_point = 0;

	if (!(best_sdelta < MV16_THRESHOLD))
    while (1)
    {
		while (count--)
		{
			int32_t sdelta;

		    dx = center_dx + 2*diamond[point].dx;
		    dy = center_dy + 2*diamond[point].dy;

			if (dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
			{
				point =  (point + 1) & 7;		// % 8
				continue;
			}

			if (dx - pred_x == 0 && dy - pred_y  == 0)
			{
				first_pass = 0;
			}

			sdelta = sad16(cur,
						get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, dx, dy, iEdgedWidth),
						iEdgedWidth,
						best_sdelta);

			sdelta += calc_delta_16(dx - pred_x, dy - pred_y, iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				if (best_sdelta < MV16_THRESHOLD)
				{
					break;
				}
				best_point = point;
			}

			point =  (point + 1) & 7;		// % 8
		}

		if (best_sdelta < MV16_THRESHOLD)
		{
			break;
		}
		if (diamond == diamond_small)
		{
			break;
		}
		
	    if ((best_dx == center_dx) && (best_dy == center_dy))
	    {
			diamond = diamond_small;
			point = 0;
			count = 4;
	    }
	    else
	    {
			if (best_dx == center_dx || best_dy == center_dy)
			{
				point = (best_point + 6) & 7;   // % 8
				count = 5;
			}
			else
			{
				point = (best_point + 7) & 7;   // % 8
				count = 3;
			}
			
			center_dx = best_dx;
			center_dy = best_dy;
	    }
	}
	
    if (first_pass)
    {
		best_sdelta2 = best_sdelta;
		best_dx2 = best_dx;
		best_dy2 = best_dy;

		// perform 2nd pass search, center=pmv
		first_pass = 0;
		center_dx = pred_x;
		center_dy = pred_y;
		goto start;
    }

	if (best_sdelta2 < best_sdelta)
	{
		best_sdelta = best_sdelta2;
		best_dx = best_dx2;
		best_dy = best_dy2;
    }


	/*
	refinement step
	(only neccessary when diamond points are multiplied by 2)
	*/

	center_dx = best_dx;
	center_dy = best_dy;

	if (!(best_sdelta < MV16_THRESHOLD))
	for (dx = center_dx - 1; dx <= center_dx + 1; dx++)
	{
		for (dy = center_dy - 1; dy <= center_dy + 1; dy++)
	    {
			int32_t sdelta;
			
		    if ((dx == center_dx && dy == center_dy) ||
			    dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
			{
				continue;
			}

			sdelta = sad16(cur,
					get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, dx, dy, iEdgedWidth),
					iEdgedWidth,
					best_sdelta);

			sdelta += calc_delta_16(dx - pred_x, dy - pred_y, iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				if (sdelta < MV16_THRESHOLD)
				{
					break;
				}
			}
		}
	}

	currMV->x = best_dx;
	currMV->y = best_dy;
	currPMV->x = best_dx - pred_x;
	currPMV->y = best_dy - pred_y;
    return best_sdelta;
}



static uint32_t MotionSearch8(
				const uint8_t * const pRef,
				const uint8_t * const pRefH,
				const uint8_t * const pRefV,
				const uint8_t * const pRefHV,
				const IMAGE * const pCur,
				const uint32_t x, const uint32_t y,
				const int32_t start_x, const int32_t start_y,
				const uint32_t iQuality,
				MBParam * const pParam,
				MACROBLOCK * const pMBs,				
				VECTOR * const currMV,
				VECTOR * const currPMV)
{
	const int32_t iFcode = pParam->fixed_code;
	const int32_t iQuant = pParam->quant;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width; 

	const uint8_t * cur = pCur->y + x*8 + y*8*iEdgedWidth;

	const DPOINT * diamond;

    int32_t min_dx, max_dx;
    int32_t min_dy, max_dy;
    int32_t dx, dy;
    int32_t center_dx, center_dy;
    int32_t pred_x,pred_y;
	
    uint32_t best_sdelta;
	int32_t best_dx, best_dy;
	
    uint32_t point;
	uint32_t best_point;
    uint32_t count;

    int32_t iSubBlock = ((y&1)<<1) + (x&1);

    get_pmv(pMBs, (x>>1), (y>>1), pParam->mb_width, iSubBlock, &pred_x, &pred_y);   

    get_range(&min_dx, &max_dx, &min_dy, &max_dy,
			x, y, 8, iWidth, iHeight, iFcode);

	// center search on start_x/y (mv from previous frame)

	center_dx = start_x;
	center_dy = start_y;

	if (center_dx < min_dx) 
		center_dx = min_dx;
	else if (center_dx > max_dx) 
		center_dx = max_dx;

	if (center_dy < min_dy) 
		center_dy = min_dy;
	else if (center_dy > max_dy) 
		center_dy = max_dy;

	// sad/delta for center
	
	best_sdelta = sad8(cur,
			get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, center_dx, center_dy, iEdgedWidth),
			iEdgedWidth);
	best_sdelta += calc_delta_8(center_dx - pred_x, center_dy - pred_y, iFcode) * iQuant;

	if (best_sdelta < MV8_THRESHOLD) 
	{
		currMV->x = center_dx;
		currMV->y = center_dy;
		currPMV->x = center_dx - pred_x;
		currPMV->y = center_dy - pred_y;
	    return best_sdelta;
	}
	best_dx = center_dx;
	best_dy = center_dy;


	diamond = diamond_large;
    point = 0;
    count = 8;
    best_point = 0;

    while(1)
    {
		while(count--)
		{
			uint32_t sdelta;

			dx = center_dx + diamond[point].dx;
			dy = center_dy + diamond[point].dy;

		    if (dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
		    {
				point =  (point + 1) & 7;		// % 8
				continue;
		    }

			sdelta = sad8(cur,
					get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, dx, dy, iEdgedWidth),
					iEdgedWidth);

			sdelta += calc_delta_8(dx - pred_x, dy - pred_y, iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				if (sdelta < MV8_THRESHOLD) {
					currMV->x = dx;
					currMV->y = dy;
					currPMV->x = dx - pred_x;
					currPMV->y = dy - pred_y;
				    return sdelta;
				}
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				best_point = point;
			}

			point =  (point + 1) & 7;		// % 8
		}

		if (diamond == diamond_small)
		{
			break;
		}

		if (best_dx == center_dx && best_dy == center_dy)
		{
			diamond = diamond_small;
			point = 0;
			count = 4;
		}
		else
		{
			if (best_dx == center_dx || best_dy == center_dy)
			{
				point = (best_point + 6) & 7;   // % 8
				count = 5;
			}
			else
			{
				point = (best_point + 7) & 7;   // % 8
				count = 3;
			}
			
			center_dx = best_dx;
			center_dy = best_dy;
	    }
    }

	/*
	refinement step
	(only neccessary when diamond points are multiplied by 2)

	center_dx = best_dx;
	center_dy = best_dy;

	if (!(best_sdelta < MV8_THRESHOLD))
	for (dx = center_dx - 1; dx <= center_dx + 1; dx++)
	{
		for (dy = center_dy - 1; dy <= center_dy + 1; dy++)
	    {
			uint32_t sdelta;
			
		    if ((dx == center_dx && dy == center_dy) ||
			    dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
			{
				continue;
			}

			sdelta = sad8(cur,
					get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, dx, dy, iEdgedWidth),
					iEdgedWidth);

			sdelta += calc_delta_8(dx - pred_x, dy - pred_y, iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				if (sdelta < MV8_THRESHOLD)
				{
					break;
				}
			}
		}
	} */

	currMV->x = best_dx;
	currMV->y = best_dy;
	currPMV->x = best_dx - pred_x;
	currPMV->y = best_dy - pred_y;
    return best_sdelta;
}




#ifndef SEARCH16
#define SEARCH16	MotionSearch16
#endif

#ifndef SEARCH8
#define SEARCH8		MotionSearch8
#endif

bool MotionEstimation(
			MACROBLOCK * const pMBs,
			MBParam * const pParam,
		    const IMAGE * const pRef,
			const IMAGE * const pRefH,
		    const IMAGE * const pRefV,
			const IMAGE * const pRefHV,
		    IMAGE * const pCurrent, 
			const uint32_t iLimit)

{
    const uint32_t iWcount = pParam->mb_width;
    const uint32_t iHcount = pParam->mb_height;
 
	uint32_t i, j, iIntra = 0;

    VECTOR mv16;
    VECTOR pmv16;

    int32_t sad8 = 0;
    int32_t sad16;
    int32_t deviation;

	// note: i==horizontal, j==vertical
    for (i = 0; i < iHcount; i++)
		for (j = 0; j < iWcount; j++)
		{
			MACROBLOCK *pMB = &pMBs[j + i * iWcount];

			sad16 = SEARCH16(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
					  j, i, pParam->quality, 
					  pParam, pMBs, &mv16, &pmv16); 
			pMB->sad16=sad16;

		if (pParam->quality > 4)
		{
			pMB->sad8[0] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
					2 * j, 2 * i, mv16.x, mv16.y, pParam->quality, 
					pParam, pMBs, &pMB->mvs[0], &pMB->pmvs[0]); 

			pMB->sad8[1] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
					2 * j + 1, 2 * i, mv16.x, mv16.y, pParam->quality, 
					pParam, pMBs, &pMB->mvs[1], &pMB->pmvs[1]); 

			pMB->sad8[2] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
					2 * j, 2 * i + 1, mv16.x, mv16.y, pParam->quality, 
					pParam, pMBs, &pMB->mvs[2], &pMB->pmvs[2]); 
			
			pMB->sad8[3] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
					2 * j + 1, 2 * i + 1, mv16.x, mv16.y, pParam->quality, 
					pParam, pMBs, &pMB->mvs[3], &pMB->pmvs[3]); 

			sad8 = pMB->sad8[0] + pMB->sad8[1] + pMB->sad8[2] + pMB->sad8[3];
		}

    
		/* decide: MODE_INTER or MODE_INTER4V 
			mpeg4:   if (sad8 < sad16 - nb/2+1) use_inter4v
		*/

		if (pMB->dquant == NO_CHANGE) {
			if ((pParam->quality <= 4) || 
				(sad16 < (sad8 + (int32_t)(IMV16X16 * pParam->quant)))) { 
			
				sad8 = sad16;
				pMB->mode = MODE_INTER;
				pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = mv16.x;
				pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = mv16.y;
				pMB->pmvs[0].x = pmv16.x;
				pMB->pmvs[0].y = pmv16.y;
			}
			else
				pMB->mode = MODE_INTER4V;
		}
		else 
		{
			sad8 = sad16;
			pMB->mode = MODE_INTER;
			pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = mv16.x;
			pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = mv16.y;
			pMB->pmvs[0].x = pmv16.x;
			pMB->pmvs[0].y = pmv16.y;
		}

		/* decide: MODE_INTER/4V or MODE_INTRA 
			if (dev_intra < sad_inter - 2 * nb) use_intra
		*/

		deviation = dev16(pCurrent->y + j*16 + i*16*pParam->edged_width, pParam->edged_width);
	
		if (deviation < (sad8 - INTER_BIAS))
		{
			pMB->mode = MODE_INTRA;
			pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = 0;
			pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = 0;

			iIntra++;
			if(iIntra >= iLimit)	
				return 1;
		}
	}

	return 0;
}


// --- compensate ---


static __inline void compensate8x8_halfpel(
				int16_t * const dct_codes,
				uint8_t * const cur,
				const uint8_t * const ref,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, const uint32_t y,
				const int32_t dx,  const int dy,
				const uint32_t stride)
{
	int32_t ddx,ddy;

	switch ( ((dx&1)<<1) + (dy&1) )   // ((dx%2)?2:0)+((dy%2)?1:0)
    {
    case 0 :
		ddx = dx/2;
		ddy = dy/2;
		compensate(dct_codes, cur + y*stride + x, 
				ref + (y+ddy)*stride + x+ddx, stride);
		break;

    case 1 :
		ddx = dx/2;
		ddy = (dy-1)/2;
		compensate(dct_codes, cur + y*stride + x, 
				refv + (y+ddy)*stride + x+ddx, stride);
		break;

    case 2 :
		ddx = (dx-1)/2;
		ddy = dy/2;
		compensate(dct_codes, cur + y*stride + x, 
				refh + (y+ddy)*stride + x+ddx, stride);
		break;

	default :	// case 3:
		ddx = (dx-1)/2;
		ddy = (dy-1)/2;
		compensate(dct_codes, cur + y*stride + x, 
				refhv + (y+ddy)*stride + x+ddx, stride);
		break;
    }
}



void MBMotionCompensation(
			MACROBLOCK * const mb,
		    const uint32_t i,
			const uint32_t j,
		    const IMAGE * const ref,
			const IMAGE * const refh,
		    const IMAGE * const refv,
			const IMAGE * const refhv,
		    IMAGE * const cur,
		    int16_t dct_codes[][64],
			const uint32_t width, 
			const uint32_t height,
			const uint32_t edged_width)
{
	static const uint32_t roundtab[16] =
		{ 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };


	if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q)
	{
		int32_t dx = mb->mvs[0].x;
		int32_t dy = mb->mvs[0].y;

		compensate8x8_halfpel(dct_codes[0], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i,     16*j,     dx, dy, edged_width);
		compensate8x8_halfpel(dct_codes[1], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i + 8, 16*j,     dx, dy, edged_width);
		compensate8x8_halfpel(dct_codes[2], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i,     16*j + 8, dx, dy, edged_width);
		compensate8x8_halfpel(dct_codes[3], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i + 8, 16*j + 8, dx, dy, edged_width);

		dx = (dx & 3) ? (dx >> 1) | 1 : dx / 2;
		dy = (dy & 3) ? (dy >> 1) | 1 : dy / 2;

		compensate8x8_halfpel(dct_codes[4], cur->u, ref->u, refh->u, refv->u, refhv->u,
								8*i, 8*j, dx, dy, edged_width/2);

		compensate8x8_halfpel(dct_codes[5], cur->v, ref->v, refh->v, refv->v, refhv->v,
								8*i, 8*j, dx, dy, edged_width/2);

	}
	else	// mode == MODE_INTER4V
	{
		int32_t sum, dx, dy;

		compensate8x8_halfpel(dct_codes[0], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i,     16*j,     mb->mvs[0].x, mb->mvs[0].y, edged_width);
		compensate8x8_halfpel(dct_codes[1], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i + 8, 16*j,     mb->mvs[1].x, mb->mvs[1].y, edged_width);
		compensate8x8_halfpel(dct_codes[2], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i,     16*j + 8, mb->mvs[2].x, mb->mvs[2].y, edged_width);
		compensate8x8_halfpel(dct_codes[3], cur->y, ref->y, refh->y, refv->y, refhv->y,
								16*i + 8, 16*j + 8, mb->mvs[3].x, mb->mvs[3].y, edged_width);

		sum = mb->mvs[0].x + mb->mvs[1].x + mb->mvs[2].x + mb->mvs[3].x;
		dx = (sum ? SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2) : 0);

		sum = mb->mvs[0].y + mb->mvs[1].y + mb->mvs[2].y + mb->mvs[3].y;
		dy = (sum ? SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2) : 0);

		compensate8x8_halfpel(dct_codes[4], cur->u, ref->u, refh->u, refv->u, refhv->u,
								8*i, 8*j, dx, dy, edged_width/2);

		compensate8x8_halfpel(dct_codes[5], cur->v, ref->v, refh->v, refv->v, refhv->v,
								8*i, 8*j, dx, dy, edged_width/2);

	}
}







/******************************************************************************************/



#define MVequal(A,B) ( ((A).x)==((B).x) && ((A).y)==((B).y) )

#define CHECK_MV16_ZERO {\
  if ( (0 <= max_dx) && (0 >= min_dx) \
    && (0 <= max_dy) && (0 >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, 0, 0 , iEdgedWidth), iEdgedWidth, MV_MAX_ERROR); \
    iSAD += calc_delta_16(-pmv[0].x, -pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD <= iQuant * 96)	\
   	iSAD -= MV16_00_BIAS; \
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=0; currMV->y=0; }  }	\
}


#define CHECK_MV16_CANDIDATE(X,Y) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - pmv[0].x, (Y) - pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } } \
}

#define CHECK_MV16_CANDIDATE_DIR(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - pmv[0].x, (Y) - pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); } } \
}

#define CHECK_MV16_CANDIDATE_FOUND(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - pmv[0].x, (Y) - pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); iFound=0; } } \
}





#define CHECK_MV8_ZERO {\
  iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, 0, 0 , iEdgedWidth), iEdgedWidth); \
  iSAD += calc_delta_8(-pmv[0].x, -pmv[0].y, (uint8_t)iFcode) * iQuant;\
  if (iSAD < iMinSAD) \
  { iMinSAD=iSAD; currMV->x=0; currMV->y=0; } \
}
		

#define CHECK_MV8_CANDIDATE(X,Y) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-pmv[0].x, (Y)-pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } } \
}

#define CHECK_MV8_CANDIDATE_DIR(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-pmv[0].x, (Y)-pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); } } \
}

#define CHECK_MV8_CANDIDATE_FOUND(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-pmv[0].x, (Y)-pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); iFound=0; } } \
}






/* This is somehow a copy of get_pmv, but returning all MVs and Minimum SAD 
   instead of only Median MV */

static __inline int get_pmvdata(const MACROBLOCK * const pMBs,
							const uint32_t x, const uint32_t y,
							const uint32_t x_dim,
							const uint32_t block,
							VECTOR * const pmv, 
							int32_t * const psad)
{
/* pmv are filled with: 
	[0]: Median (or whatever is correct in a special case)
	[1]: left neighbour
	[2]: top neighbour, 
	[3]: topright neighbour,
   psad are filled with:
	[0]: minimum of [1] to [3]
	[1]: left neighbour's SAD	// [1] to [3] are actually not needed  
	[2]: top neighbour's SAD, 
	[3]: topright neighbour's SAD,
*/

    int xin1, xin2, xin3;
    int yin1, yin2, yin3;
    int vec1, vec2, vec3;

    static VECTOR zeroMV;
    uint32_t index = x + y * x_dim;
    zeroMV.x = zeroMV.y = 0;

	// first row (special case)
    if (y == 0 && (block == 0 || block == 1))
    {
		if ((x == 0) && (block == 0))		// first column, first block
		{ 
			pmv[0] = pmv[1] = pmv[2] = pmv[3] = zeroMV;
			psad[0] = psad[1] = psad[2] = psad[3] = MV_MAX_ERROR;
			return 0;
		}
		if (block == 1)		// second block; has only a left neighbour
		{
			pmv[0] = pmv[1] = pMBs[index].mvs[0];
			pmv[2] = pmv[3] = zeroMV;
			psad[0] = psad[1] = pMBs[index].sad8[0];
			psad[2] = psad[3] = MV_MAX_ERROR;
			return 0;
		}
		else /* block==0, but x!=0, so again, there is a left neighbour*/
		{
			pmv[0] = pmv[1] = pMBs[index-1].mvs[1];
			pmv[2] = pmv[3] = zeroMV;
			psad[0] = psad[1] = pMBs[index-1].sad8[1];
			psad[2] = psad[3] = MV_MAX_ERROR;
			return 0;
		}
    }

	/*
		MODE_INTER, vm18 page 48
		MODE_INTER4V vm18 page 51

					(x,y-1)		(x+1,y-1)
					[   |   ]	[	|   ]
					[ 2 | 3 ]	[ 2 |   ]

		(x-1,y)		(x,y)		(x+1,y)
		[   | 1 ]	[ 0 | 1 ]	[ 0 |   ]
		[   | 3 ]	[ 2 | 3 ]	[	|   ]
	*/

    switch (block)
    {
	case 0:
		xin1 = x - 1;	yin1 = y;	vec1 = 1;	/* left */
		xin2 = x;	yin2 = y - 1;	vec2 = 2;	/* top */
		xin3 = x + 1;	yin3 = y - 1;	vec3 = 2;	/* top right */
		break;
	case 1:
		xin1 = x;		yin1 = y;		vec1 = 0;	
		xin2 = x;		yin2 = y - 1;   vec2 = 3;
		xin3 = x + 1;	yin3 = y - 1;	vec3 = 2;
	    break;
	case 2:
		xin1 = x - 1;	yin1 = y;		vec1 = 3;
		xin2 = x;		yin2 = y;		vec2 = 0;
		xin3 = x;		yin3 = y;		vec3 = 1;
	    break;
	default:
		xin1 = x;		yin1 = y;		vec1 = 2;
		xin2 = x;		yin2 = y;		vec2 = 0;
		xin3 = x;		yin3 = y;		vec3 = 1;
    }


	if (xin1 < 0 || /* yin1 < 0  || */ xin1 >= (int32_t)x_dim)
	{
	    	pmv[1] = zeroMV;
		psad[1] = MV_MAX_ERROR;
	}
	else
	{
		pmv[1] = pMBs[xin1 + yin1 * x_dim].mvs[vec1]; 
		psad[1] = pMBs[xin1 + yin1 * x_dim].sad8[vec1]; 
	}

	if (xin2 < 0 || /* yin2 < 0 || */ xin2 >= (int32_t)x_dim)
	{
		pmv[2] = zeroMV;
		psad[2] = MV_MAX_ERROR;
	}
	else
	{
		pmv[2] = pMBs[xin2 + yin2 * x_dim].mvs[vec2]; 
		psad[2] = pMBs[xin2 + yin2 * x_dim].sad8[vec2]; 
	}

	if (xin3 < 0 || /* yin3 < 0 || */ xin3 >= (int32_t)x_dim)
	{
		pmv[3] = zeroMV;
		psad[3] = MV_MAX_ERROR;
	}
	else
	{
		pmv[3] = pMBs[xin3 + yin3 * x_dim].mvs[vec3];
		psad[3] = pMBs[xin2 + yin2 * x_dim].sad8[vec3]; 
	}

	if ( (MVequal(pmv[1],pmv[2])) && (MVequal(pmv[1],pmv[3])) )
	{	pmv[0]=pmv[1];
		psad[0]=psad[1];
		return 1;
	}

	// median,minimum
	
	pmv[0].x = MIN(MAX(pmv[1].x, pmv[2].x), MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
	pmv[0].y = MIN(MAX(pmv[1].y, pmv[2].y), MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
	psad[0]=MIN(MIN(psad[1],psad[2]),psad[3]);
	return 0;
}


/* This is somehow a copy of get_ref, but with MV instead of X,Y */

static __inline const uint8_t * get_ref_mv(
				const uint8_t * const refn,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, const uint32_t y,
				const uint32_t block,			// block dimension, 8 or 16
				const VECTOR* mv,	// measured in half-pel!
				const uint32_t stride)
{
	switch ( (((mv->x)&1)<<1) + ((mv->y)&1) )
    {
	case 0 : return refn + (x*block+(mv->x)/2) + (y*block+(mv->y)/2)*stride;
    	case 1 : return refv + (x*block+(mv->x)/2) + (y*block+((mv->y)-1)/2)*stride;
	case 2 : return refh + (x*block+((mv->x)-1)/2) + (y*block+(mv->y)/2)*stride;
	default : 
	case 3 : return refhv + (x*block+((mv->x)-1)/2) + (y*block+((mv->y)-1)/2)*stride;
	}
}


#define PMV_HALFPELDIAMOND 0x01000000
#define PMV_HALFPELREFINE  0x02000000
#define PMV_HALFPEL 	   (PMV_HALFPELDIAMOND|PMV_HALFPELREFINE)	/* if halfpel is used at all */

#define PMV_EARLYSTOP	   0x04000000
#define PMV_UNRESTRICTED   0x08000000		// not implemented
#define PMV_QUICKSTOP	   0x10000000 	// like early, but without any more refinement

int32_t PMVfastSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t iQuality, 				
					MBParam * const pParam,
					MACROBLOCK * const pMBs,				
					VECTOR * const currMV,
					VECTOR * const currPMV)
{
        const uint32_t iWcount = pParam->mb_width;
	const int32_t iFcode = pParam->fixed_code;
	const int32_t iQuant = pParam->quant;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width; 

	const uint8_t * cur = pCur->y + x*16 + y*16*iEdgedWidth;

	int32_t iDiamondSize;
	uint32_t iFlags;
	
	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;
		
	VECTOR pmv[4];
	int32_t psad[4];
	VECTOR backupMV;
	
	MACROBLOCK * const pMB = pMBs + x + y * iWcount;

	static int32_t threshA,threshB;
    	int32_t iDirection,iFound,bPredEq;
    	int32_t iMinSAD,iSAD;

	switch (iQuality)
	{
	case 1:	iFlags = PMV_QUICKSTOP; break;
	case 2: iFlags = PMV_EARLYSTOP; break;
	case 3: iFlags = PMV_EARLYSTOP; 
		break;
	case 4: iFlags = PMV_HALFPELREFINE;
		break;
	case 5: iFlags = PMV_HALFPELREFINE;
		break;
	default: iFlags = iQuality;
	}

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy,
			x, y, 16, iWidth, iHeight, iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if (!(iFlags & PMV_HALFPEL ))
	{ min_dx = EVEN(min_dx);
	  max_dx = EVEN(max_dx);
	  min_dy = EVEN(min_dy);
	  max_dy = EVEN(max_dy); 
	}		/* because we might use something like IF (dx>max_dx) THEN dx=max_dx; */
		

	bPredEq  = get_pmvdata(pMBs, x, y, iWcount, 0, pmv, psad);

	if ((x==0) && (y==0) )
	{
		threshA =  512;
		threshB = 1024;
	
	}
	else
	{
		threshA = psad[0];
		threshB = threshA+256;
		if (threshA< 512) threshA =  512;
                if (threshA>1024) threshA = 1024; 
        	if (threshB>1792) threshB = 1792; 
	}

	iFound=0;

	if (!(iFlags & PMV_HALFPEL ))
	{ 	/* This should NOT be necessary! */
		pmv[0].x = EVEN(pmv[0].x);
		pmv[0].y = EVEN(pmv[0].y);
	}
	
	if (pmv[0].x > max_dx) 	
		{	
			pmv[0].x=max_dx;
		}
	if (pmv[0].x < min_dx) 
		{	
			pmv[0].x=min_dx;
		}
	if (pmv[0].y > max_dy) 
		{	
			pmv[0].y=max_dy;
		}
	if (pmv[0].y < min_dy) 
		{	
			pmv[0].y=min_dy;
		}
	
/* Step 2: Calculate Distance= |MedianMVX| + |MedianMVY| where MedianMV is the motion 
	vector of the median. 
	If PredEq=1 and MVpredicted = Previous Frame MV, set Found=2  
*/

        if ((bPredEq) && (MVequal(pmv[0],pMB->mvs[0]) ) )
		iFound=2;

/* Step 3: If Distance>0 or thresb<1536 or PredEq=1 Select small Diamond Search. 
        Otherwise select large Diamond Search. 
*/

	if ( (pmv[0].x != 0) || (pmv[0].y != 0) || (threshB<1536) || (bPredEq) ) 
		iDiamondSize=1;	// halfpel!
	else
		iDiamondSize=2;	// halfpel!

	if (!(iFlags & PMV_HALFPELDIAMOND) )
		iDiamondSize*=2;

/* Step 4: Calculate SAD around the Median prediction. 
        MinSAD=SAD 
        If Motion Vector equal to Previous frame motion vector 
		and MinSAD<PrevFrmSAD goto Step 10. 
        If SAD<=256 goto Step 10. 
*/	


// Prepare for main loop 

	*currMV=pmv[0];		/* current best := prediction */
	
	iMinSAD = sad16( cur, 
		get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 16, currMV, iEdgedWidth),
		iEdgedWidth, MV_MAX_ERROR);
  	iMinSAD += calc_delta_16(currMV->x-pmv[0].x, currMV->y -pmv[0].y, (uint8_t)iFcode) * iQuant;
	
	if ( (iMinSAD < 256 ) || ( (MVequal(pmv[0],pMB->mvs[0])) && (iMinSAD < pMB->sad16) ) )
		{
		
			if (iFlags & PMV_QUICKSTOP) 
				goto step10b;
			if (iFlags & PMV_EARLYSTOP) 
				goto step10;
		}

/* 
Step 5: Calculate SAD for motion vectors taken from left block, top, top-right, and Previous frame block. 
        Also calculate (0,0) but do not subtract offset. 
        Let MinSAD be the smallest SAD up to this point. 
        If MV is (0,0) subtract offset. ******** WHAT'S THIS 'OFFSET' ??? ***********
*/

// (0,0) is always possible

	CHECK_MV16_ZERO;

// previous frame MV is always possible
	CHECK_MV16_CANDIDATE(pMB->mvs[0].x,pMB->mvs[0].y);
	
// left neighbour, if allowed
	if (x != 0) 
	{
		if (!(iFlags & PMV_HALFPEL ))
		{	pmv[1].x = EVEN(pmv[1].x);
			pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV16_CANDIDATE(pmv[1].x,pmv[1].y);		
	}

// top neighbour, if allowed
	if (y != 0)
	{	
		if (!(iFlags & PMV_HALFPEL ))
		{	pmv[2].x = EVEN(pmv[2].x);
			pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV16_CANDIDATE(pmv[2].x,pmv[2].y);
	
// top right neighbour, if allowed
		if (x != (iWcount-1))
		{
			if (!(iFlags & PMV_HALFPEL ))
			{	pmv[3].x = EVEN(pmv[3].x);
				pmv[3].y = EVEN(pmv[3].y);
			}
			CHECK_MV16_CANDIDATE(pmv[3].x,pmv[3].y);
		}
	}

/* Step 6: If MinSAD <= thresa goto Step 10. 
   If Motion Vector equal to Previous frame motion vector and MinSAD<PrevFrmSAD goto Step 10. 
*/

	if ( (iMinSAD <= threshA) || ( MVequal(*currMV,pMB->mvs[0]) && (iMinSAD < pMB->sad16) ) )
		{	
			if (iFlags & PMV_QUICKSTOP) 
				goto step10b;
			if (iFlags & PMV_EARLYSTOP) 
				goto step10;
		}

/************ (Diamond Search)  **************/
/* 
Step 7: Perform Diamond search, with either the small or large diamond. 
        If Found=2 only examine one Diamond pattern, and afterwards goto step 10 
Step 8: If small diamond, iterate small diamond search pattern until motion vector lies in the center of the diamond. 
        If center then goto step 10. 
Step 9: If large diamond, iterate large diamond search pattern until motion vector lies in the center. 
        Refine by using small diamond and goto step 10. 
*/

/* Do one search with full Diamond pattern, and only 3 of 4 for all following diamonds */

	iDirection = 0; 
	backupMV=*currMV;

	CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);

	while (!iFound)
	{	
		iFound = 1; 
		backupMV=*currMV;

		if ( iDirection != 2) 
			CHECK_MV16_CANDIDATE_FOUND(backupMV.x-iDiamondSize,backupMV.y,1);
		if ( iDirection != 1) 
			CHECK_MV16_CANDIDATE_FOUND(backupMV.x+iDiamondSize,backupMV.y,2);
		if ( iDirection != 4) 
			CHECK_MV16_CANDIDATE_FOUND(backupMV.x,backupMV.y-iDiamondSize,3);
		if ( iDirection != 3) 
			CHECK_MV16_CANDIDATE_FOUND(backupMV.x,backupMV.y+iDiamondSize,4);

	}

/* Step 10: The motion vector is chosen according to the block corresponding to MinSAD.
         By performing an optional local half-pixel search, we can refine this result even further.
*/
	
step10:	
	if (iFlags & PMV_HALFPELREFINE) 		// perform final half-pel step 
	{
		backupMV=*currMV;
		CHECK_MV16_CANDIDATE(backupMV.x-1,backupMV.y-1);
		CHECK_MV16_CANDIDATE(backupMV.x  ,backupMV.y-1);
		CHECK_MV16_CANDIDATE(backupMV.x+1,backupMV.y-1);
		CHECK_MV16_CANDIDATE(backupMV.x-1,backupMV.y);
		CHECK_MV16_CANDIDATE(backupMV.x+1,backupMV.y);
		CHECK_MV16_CANDIDATE(backupMV.x-1,backupMV.y+1);
		CHECK_MV16_CANDIDATE(backupMV.x  ,backupMV.y+1);
		CHECK_MV16_CANDIDATE(backupMV.x+1,backupMV.y+1);
	}

step10b:
	currPMV->x = currMV->x - pmv[0].x;
	currPMV->y = currMV->y - pmv[0].y;
	return iMinSAD;
}

int32_t PMVfastSearch8(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int start_x, int start_y,
					const uint32_t iQuality, 
					MBParam * const pParam,
					MACROBLOCK * const pMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV)
{
        const uint32_t iWcount = pParam->mb_width;

	const int32_t iFcode = pParam->fixed_code;
	const int32_t iQuant = pParam->quant;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width; 

	const uint8_t * cur = pCur->y + x*8 + y*8*iEdgedWidth;

	int32_t iDiamondSize;

	uint32_t iFlags;
	
	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;
		
	VECTOR pmv[4];
	int32_t psad[4];
	VECTOR backupMV;
	
	MACROBLOCK * const pMB = pMBs + (x>>1) + (y>>1) * iWcount;

	static int32_t threshA,threshB;
    	int32_t iDirection,iFound,bPredEq;
    	int32_t iMinSAD,iSAD;

	int32_t iSubBlock = ((y&1)<<1) + (x&1);

	switch (iQuality)	/* only case 5 (and maybe 4) will be used */
	{
	case 1:	;
	case 2: ;
	case 3: iFlags = PMV_EARLYSTOP;  
		break;
	case 4: iFlags = PMV_EARLYSTOP|PMV_HALFPELREFINE|PMV_HALFPELDIAMOND;  
		break;
	case 5: iFlags = PMV_EARLYSTOP|PMV_HALFPELREFINE|PMV_HALFPELDIAMOND;  
		break;
	default: iFlags = iQuality;
	}

/* Get maximum range */
    get_range(&min_dx, &max_dx, &min_dy, &max_dy,
			x, y, 8, iWidth, iHeight, iFcode);

/* we work with abs. MVs, not relative to prediction, so range is relative to 0,0 */

	if (!(iFlags & PMV_HALFPELDIAMOND ))
	{ min_dx = EVEN(min_dx);
	  max_dx = EVEN(max_dx);
	  min_dy = EVEN(min_dy);
	  max_dy = EVEN(max_dy); 
	}		/* because we might use IF (dx>max_dx) THEN dx=max_dx; */
		

	bPredEq  = get_pmvdata(pMBs, (x>>1), (y>>1), iWcount, iSubBlock, pmv, psad);

	if ((x==0) && (y==0) )
	{
		threshA =  512/4;
		threshB = 1024/4;
	
	}
	else
	{
		threshA = psad[0]/4;			/* good estimate */
		threshB = threshA+256/4;
		if (threshA< 512/4) threshA =  512/4;
                if (threshA>1024/4) threshA = 1024/4; 
        	if (threshB>1792/4) threshB = 1792/4; 
	}

	iFound=0;

	if (!(iFlags & PMV_HALFPEL ))
	{ 	/* This should NOT be necessary unless you change iFlags between MBs */
		pmv[0].x = EVEN(pmv[0].x);
		pmv[0].y = EVEN(pmv[0].y);
	}

	if (pmv[0].x > max_dx) 	
		{	
			pmv[0].x=max_dx;
		}
	if (pmv[0].x < min_dx) 
		{	
			pmv[0].x=min_dx;
		}
	if (pmv[0].y > max_dy) 
		{	
			pmv[0].y=max_dy;
		}
	if (pmv[0].y < min_dy) 
		{	
			pmv[0].y=min_dy;
		}
	
/* Step 2: Calculate Distance= |MedianMVX| + |MedianMVY| where MedianMV is the motion 
	vector of the median. 
	If PredEq=1 and MVpredicted = Previous Frame MV, set Found=2  
*/

        if ((bPredEq) && (MVequal(pmv[0],pMB->mvs[iSubBlock]) ) )
		iFound=2;

/* Step 3: If Distance>0 or thresb<1536 or PredEq=1 Select small Diamond Search. 
        Otherwise select large Diamond Search. 
*/

	if ( (pmv[0].x != 0) || (pmv[0].y != 0) || (threshB<1536/4) || (bPredEq) ) 
		iDiamondSize=1;	// 1 halfpel!
	else
		iDiamondSize=2;	// 2 halfpel = 1 full pixel!

	if (!(iFlags & PMV_HALFPELDIAMOND) )
		iDiamondSize*=2;

/* Step 4: Calculate SAD around the Median prediction. 
        MinSAD=SAD 
        If Motion Vector equal to Previous frame motion vector 
		and MinSAD<PrevFrmSAD goto Step 10. 
        If SAD<=256 goto Step 10. 
*/	


// Prepare for main loop 

	currMV->x=start_x;		/* start with mv16 */
	currMV->y=start_y;		
/*	pmv[0];	*/		
	
	iMinSAD = sad8( cur, 
		get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 8, currMV, iEdgedWidth),
		iEdgedWidth);
  	iMinSAD += calc_delta_8(currMV->x - pmv[0].x, currMV->y - pmv[0].y, (uint8_t)iFcode) * iQuant;
	
	if ( (iMinSAD < 256/4 ) || ( (MVequal(pmv[0],pMB->mvs[iSubBlock])) && (iMinSAD < pMB->sad8[iSubBlock]) ) )
		{
			if (iFlags & PMV_QUICKSTOP) 
				goto step10_8b;
			if (iFlags & PMV_EARLYSTOP) 
				goto step10_8;
		}

/* 
Step 5: Calculate SAD for motion vectors taken from left block, top, top-right, and Previous frame block. 
        Also calculate (0,0) but do not subtract offset. 
        Let MinSAD be the smallest SAD up to this point. 
        If MV is (0,0) subtract offset. ******** WHAT'S THIS 'OFFSET' ??? ***********
*/

// the prediction might be even better than mv16
	CHECK_MV8_CANDIDATE(pmv[0].x,pmv[0].y);

// (0,0) is always possible
	CHECK_MV8_ZERO;

// previous frame MV is always possible
	CHECK_MV8_CANDIDATE(pMB->mvs[iSubBlock].x,pMB->mvs[iSubBlock].y);
	
// left neighbour, if allowed
	if (psad[1] != MV_MAX_ERROR) 
	{
		if (!(iFlags & PMV_HALFPEL ))	/* remove these checks */
		{	pmv[1].x = EVEN(pmv[1].x);				/* if INTER4V=>halfpel */
			pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV8_CANDIDATE(pmv[1].x,pmv[1].y);		
	}

// top neighbour, if allowed
	if (psad[2] != MV_MAX_ERROR) 
	{	
		if (!(iFlags & PMV_HALFPEL ))
		{	pmv[2].x = EVEN(pmv[2].x);
			pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV8_CANDIDATE(pmv[2].x,pmv[2].y);
	
// top right neighbour, if allowed
		if (psad[3] != MV_MAX_ERROR) 
		{
		if (!(iFlags & PMV_HALFPEL ))
		{	pmv[3].x = EVEN(pmv[3].x);
			pmv[3].y = EVEN(pmv[3].y);
		}
			CHECK_MV8_CANDIDATE(pmv[3].x,pmv[3].y);
		}
	}

/* Step 6: If MinSAD <= thresa goto Step 10. 
   If Motion Vector equal to Previous frame motion vector and MinSAD<PrevFrmSAD goto Step 10. 
*/

	if ( (iMinSAD <= threshA) || ( MVequal(*currMV,pMB->mvs[iSubBlock]) && (iMinSAD < pMB->sad8[iSubBlock]) ) )
		{	
		  	if (iFlags & PMV_QUICKSTOP)
				goto step10_8b;
		  	if (iFlags & PMV_EARLYSTOP)
				goto step10_8;
		}

/************ (Diamond Search)  **************/
/* 
Step 7: Perform Diamond search, with either the small or large diamond. 
        If Found=2 only examine one Diamond pattern, and afterwards goto step 10 
Step 8: If small diamond, iterate small diamond search pattern until motion vector lies in the center of the diamond. 
        If center then goto step 10. 
Step 9: If large diamond, iterate large diamond search pattern until motion vector lies in the center. 
        Refine by using small diamond and goto step 10. 
*/

/* Do one search with full Diamond, and only 3 of 4 for all following diamonds */

	iDirection = 0; 
	backupMV=*currMV;

	CHECK_MV8_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);

	while (!iFound)
	{	
		iFound = 1; 
		backupMV=*currMV;

		if ( iDirection != 2) 
			CHECK_MV8_CANDIDATE_FOUND(backupMV.x-iDiamondSize,backupMV.y,1);
		if ( iDirection != 1) 
			CHECK_MV8_CANDIDATE_FOUND(backupMV.x+iDiamondSize,backupMV.y,2);
		if ( iDirection != 4) 
			CHECK_MV8_CANDIDATE_FOUND(backupMV.x,backupMV.y-iDiamondSize,3);
		if ( iDirection != 3) 
			CHECK_MV8_CANDIDATE_FOUND(backupMV.x,backupMV.y+iDiamondSize,4);

	}

/* Step 10: The motion vector is chosen according to the block corresponding to MinSAD.
         By performing an optional local half-pixel search, we can refine this result even further.
*/
	
step10_8:	
	if (iFlags & PMV_HALFPELREFINE) 		// perform final half-pel step 
	{
		backupMV=*currMV;
		CHECK_MV8_CANDIDATE(backupMV.x-1,backupMV.y-1);
		CHECK_MV8_CANDIDATE(backupMV.x  ,backupMV.y-1);
		CHECK_MV8_CANDIDATE(backupMV.x+1,backupMV.y-1);
		CHECK_MV8_CANDIDATE(backupMV.x-1,backupMV.y);
		CHECK_MV8_CANDIDATE(backupMV.x+1,backupMV.y);
		CHECK_MV8_CANDIDATE(backupMV.x-1,backupMV.y+1);
		CHECK_MV8_CANDIDATE(backupMV.x  ,backupMV.y+1);
		CHECK_MV8_CANDIDATE(backupMV.x+1,backupMV.y+1);
	}
step10_8b:

	currPMV->x = currMV->x - pmv[0].x;
	currPMV->y = currMV->y - pmv[0].y;
	
	return iMinSAD;
}
