/**************************************************************************
 *
 *  Modifications:
 *
 *  02.04.2002 add EPZS(^2) as ME algorithm, use PMV_USESQUARES to choose between 
 *             EPZS and EPZS^2
 *  08.02.2002 split up PMVfast into three routines: PMVFast, PMVFast_MainLoop
 *             PMVFast_Refine to support multiple searches with different start points
 *  07.01.2002 uv-block-based interpolation
 *  06.01.2002 INTER/INTRA-decision is now done before any SEARCH8 (speedup)
 *             changed INTER_BIAS to 150 (as suggested by suxen_drol)
 *             removed halfpel refinement step in PMVfastSearch8 + quality=5
 *             added new quality mode = 6 which performs halfpel refinement
 *             filesize difference between quality 5 and 6 is smaller than 1%
 *             (Isibaar)
 *  31.12.2001 PMVfastSearch16 and PMVfastSearch8 (gruel)
 *  30.12.2001 get_range/MotionSearchX simplified; blue/green bug fix
 *  22.12.2001 commented best_point==99 check
 *  19.12.2001 modified get_range (purple bug fix)
 *  15.12.2001 moved pmv displacement from mbprediction
 *  02.12.2001 motion estimation/compensation split (Isibaar)
 *  16.11.2001 rewrote/tweaked search algorithms; pross@cs.rmit.edu.au
 *  10.11.2001 support for sad16/sad8 functions
 *  28.08.2001 reactivated MODE_INTER4V for EXT_MODE
 *  24.08.2001 removed MODE_INTER4V_Q, disabled MODE_INTER4V for EXT_MODE
 *  22.08.2001 added MODE_INTER4V_Q			
 *  20.08.2001 added pragma to get rid of internal compiler error with VC6
 *             idea by Cyril. Thanks.
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../encoder.h"
#include "../utils/mbfunctions.h"
#include "../prediction/mbprediction.h"
#include "../global.h"
#include "../utils/timer.h"
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
#define INTER_BIAS	512

/* Parameters which control inter/inter4v decision */
#define IMV16X16			5

/* vector map (vlc delta size) smoother parameters */
#define NEIGH_TEND_16X16	2
#define NEIGH_TEND_8X8		2


// fast ((A)/2)*2
#define EVEN(A)		(((A)<0?(A)+1:(A)) & ~1)


#ifndef MIN
#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#endif
#ifndef MAX
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#endif
#define ABS(X)    (((X)>0)?(X):-(X))
#define SIGN(X)   (((X)>0)?1:-1)

int32_t PMVfastSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
					MACROBLOCK * const pMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV);

int32_t EPZSSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
					MACROBLOCK * const pMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV);


int32_t PMVfastSearch8(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int start_x, int start_y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
					MACROBLOCK * const pMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV);

int32_t EPZSSearch8(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int start_x, int start_y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
					MACROBLOCK * const pMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV);


typedef int32_t (MainSearch16Func)(
	const uint8_t * const pRef,
	const uint8_t * const pRefH,
	const uint8_t * const pRefV,
	const uint8_t * const pRefHV,
	const uint8_t * const cur,
	const int x, const int y,
	int32_t startx, int32_t starty,	
	int32_t iMinSAD,
	VECTOR * const currMV,
	const VECTOR * const pmv,
	const int32_t min_dx, const int32_t max_dx, 
	const int32_t min_dy, const int32_t max_dy,
	const int32_t iEdgedWidth, 
	const int32_t iDiamondSize, 
	const int32_t iFcode,
	const int32_t iQuant,
	int iFound);

typedef MainSearch16Func* MainSearch16FuncPtr;


typedef int32_t (MainSearch8Func)(
	const uint8_t * const pRef,
	const uint8_t * const pRefH,
	const uint8_t * const pRefV,
	const uint8_t * const pRefHV,
	const uint8_t * const cur,
	const int x, const int y,
	int32_t startx, int32_t starty,	
	int32_t iMinSAD,
	VECTOR * const currMV,
	const VECTOR * const pmv,
	const int32_t min_dx, const int32_t max_dx, 
	const int32_t min_dy, const int32_t max_dy,
	const int32_t iEdgedWidth, 
	const int32_t iDiamondSize, 
	const int32_t iFcode,
	const int32_t iQuant,
	int iFound);

typedef MainSearch8Func* MainSearch8FuncPtr;

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


/*
 * getref: calculate reference image pointer 
 * the decision to use interpolation h/v/hv or the normal image is
 * based on dx & dy.
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
	case 0  : return refn + (x*block+dx/2) + (y*block+dy/2)*stride;
	case 1  : return refv + (x*block+dx/2) + (y*block+(dy-1)/2)*stride;
	case 2  : return refh + (x*block+(dx-1)/2) + (y*block+dy/2)*stride;
	default : 
	case 3  : return refhv + (x*block+(dx-1)/2) + (y*block+(dy-1)/2)*stride;
	}

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
	case 0  : return refn + (x*block+(mv->x)/2) + (y*block+(mv->y)/2)*stride;
    	case 1  : return refv + (x*block+(mv->x)/2) + (y*block+((mv->y)-1)/2)*stride;
	case 2  : return refh + (x*block+((mv->x)-1)/2) + (y*block+(mv->y)/2)*stride;
	default : 
	case 3  : return refhv + (x*block+((mv->x)-1)/2) + (y*block+((mv->y)-1)/2)*stride;
	}

}

#ifndef SEARCH16
#define SEARCH16	PMVfastSearch16
//#define SEARCH16	FullSearch16
//#define SEARCH16	EPZSSearch16
#endif

#ifndef SEARCH8
#define SEARCH8		PMVfastSearch8
//#define SEARCH8	EPZSSearch8
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

	if (sadInit)
		(*sadInit)();
		
	// note: i==horizontal, j==vertical
	for (i = 0; i < iHcount; i++)
		for (j = 0; j < iWcount; j++)
		{
			MACROBLOCK *pMB = &pMBs[j + i * iWcount];

			sad16 = SEARCH16(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
					 j, i, pParam->motion_flags,
					 pParam, pMBs, &mv16, &pmv16); 
			pMB->sad16=sad16;


			/* decide: MODE_INTER or MODE_INTRA 
			   if (dev_intra < sad_inter - 2 * nb) use_intra
			*/

			deviation = dev16(pCurrent->y + j*16 + i*16*pParam->edged_width, pParam->edged_width);
	
			if (deviation < (sad16 - INTER_BIAS))
			{
				pMB->mode = MODE_INTRA;
				pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = 0;
				pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = 0;

				iIntra++;
				if(iIntra >= iLimit)	
					return 1;

				continue;
			}

			if (pParam->global_flags & XVID_INTER4V)
			{
				pMB->sad8[0] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
						       2 * j, 2 * i, mv16.x, mv16.y, pParam->motion_flags, 
						       pParam, pMBs, &pMB->mvs[0], &pMB->pmvs[0]); 

				pMB->sad8[1] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
						       2 * j + 1, 2 * i, mv16.x, mv16.y, pParam->motion_flags,
						       pParam, pMBs, &pMB->mvs[1], &pMB->pmvs[1]); 

				pMB->sad8[2] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
						       2 * j, 2 * i + 1, mv16.x, mv16.y, pParam->motion_flags,
						       pParam, pMBs, &pMB->mvs[2], &pMB->pmvs[2]); 
			
				pMB->sad8[3] = SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
						       2 * j + 1, 2 * i + 1, mv16.x, mv16.y, pParam->motion_flags,
						       pParam, pMBs, &pMB->mvs[3], &pMB->pmvs[3]); 

				sad8 = pMB->sad8[0] + pMB->sad8[1] + pMB->sad8[2] + pMB->sad8[3];
			}

    
			/* decide: MODE_INTER or MODE_INTER4V 
			   mpeg4:   if (sad8 < sad16 - nb/2+1) use_inter4v
			*/

			if (pMB->dquant == NO_CHANGE) {
				if (((pParam->global_flags & XVID_INTER4V)==0) || 
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
		}

	return 0;
}

#define MVzero(A) ( ((A).x)==(0) && ((A).y)==(0) )

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

#define NOCHECK_MV16_CANDIDATE(X,Y) { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - pmv[0].x, (Y) - pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } \
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
		
#define NOCHECK_MV8_CANDIDATE(X,Y) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-pmv[0].x, (Y)-pmv[0].y, (uint8_t)iFcode) * iQuant;\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } \
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

/* too slow and not fully functional at the moment */
/*
int32_t ZeroSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t MotionFlags, 				
					MBParam * const pParam,
					MACROBLOCK * const pMBs,				
					VECTOR * const currMV,
					VECTOR * const currPMV)
{
	const int32_t iEdgedWidth = pParam->edged_width; 
	const int32_t iQuant = pParam->quant;
	const uint8_t * cur = pCur->y + x*16 + y*16*iEdgedWidth;
	int32_t iSAD;
	int32_t pred_x,pred_y;

	get_pmv(pMBs, x, y, pParam->mb_width, 0, &pred_x, &pred_y);    

	iSAD = sad16( cur, 
		get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, 0,0, iEdgedWidth),
		iEdgedWidth, MV_MAX_ERROR);
	if (iSAD <= iQuant * 96)	
	   	iSAD -= MV16_00_BIAS; 

	currMV->x = 0;
	currMV->y = 0;
	currPMV->x = -pred_x;
	currPMV->y = -pred_y;

	return iSAD;

}
*/

int32_t Diamond16_MainSearch(
	const uint8_t * const pRef,
	const uint8_t * const pRefH,
	const uint8_t * const pRefV,
	const uint8_t * const pRefHV,
	const uint8_t * const cur,
	const int x, const int y,
	int32_t startx, int32_t starty,	
	int32_t iMinSAD,
	VECTOR * const currMV,
	const VECTOR * const pmv,
	const int32_t min_dx, const int32_t max_dx, 
	const int32_t min_dy, const int32_t max_dy,
	const int32_t iEdgedWidth, 
	const int32_t iDiamondSize, 
	const int32_t iFcode,
	const int32_t iQuant,
	int iFound)
{
/* Do a diamond search around given starting point, return SAD of best */

	int32_t iDirection=0;
	int32_t iSAD;
	VECTOR backupMV;
	backupMV.x = startx;
	backupMV.y = starty;
	
/* It's one search with full Diamond pattern, and only 3 of 4 for all following diamonds */

	CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);

	if (iDirection)
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
	else
	{	
		currMV->x = startx;
		currMV->y = starty;
	}
	return iMinSAD;
}

int32_t Square16_MainSearch(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const uint8_t * const cur,
					const int x, const int y,
					int32_t startx, int32_t starty,	
					int32_t iMinSAD,
					VECTOR * const currMV,
					const VECTOR * const pmv,
					const int32_t min_dx, const int32_t max_dx, 
					const int32_t min_dy, const int32_t max_dy,
					const int32_t iEdgedWidth, 
					const int32_t iDiamondSize, 
					const int32_t iFcode,
					const int32_t iQuant,
					int iFound)
{
/* Do a square search around given starting point, return SAD of best */

	int32_t iDirection=0;
	int32_t iSAD;
	VECTOR backupMV;
	backupMV.x = startx;
	backupMV.y = starty;
	
/* It's one search with full square pattern, and new parts for all following diamonds */

/*   new direction are extra, so 1-4 is normal diamond
      537
      1*2
      648  
*/

	CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);

	CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
	

	if (iDirection)
		while (!iFound)
		{	
			iFound = 1; 
			backupMV=*currMV;
	
			switch (iDirection)
			{
				case 1:
					CHECK_MV16_CANDIDATE_FOUND(backupMV.x-iDiamondSize,backupMV.y,1);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
					break;
				case 2:
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);	
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
					break;
					
				case 3:
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
					break;
					
				case 4:
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
					break;

				case 5:	
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
					break; 
					
				case 6:
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
			
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
	
					break;
					
				case 7:
					CHECK_MV16_CANDIDATE_FOUND(backupMV.x-iDiamondSize,backupMV.y,1);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
					break;
					
				case 8:
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
					break; 
			default:
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);
				
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y-iDiamondSize,5);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y+iDiamondSize,6);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y-iDiamondSize,7);
					CHECK_MV16_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y+iDiamondSize,8);
					break;
			}
		}
	else
		{	
			currMV->x = startx;
			currMV->y = starty;
		}
	return iMinSAD;
}


int32_t Full16_MainSearch(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const uint8_t * const cur,
					const int x, const int y,
					int32_t startx, int32_t starty,	
					int32_t iMinSAD,
					VECTOR * const currMV,
					const VECTOR * const pmv,
					const int32_t min_dx, const int32_t max_dx, 
					const int32_t min_dy, const int32_t max_dy,
					const int32_t iEdgedWidth, 
					const int32_t iDiamondSize, 
					const int32_t iFcode,
					const int32_t iQuant,
					int iFound)
{
	int32_t iSAD;
	int32_t dx,dy;
	VECTOR backupMV;
	backupMV.x = startx;
	backupMV.y = starty;
	
	for (dx = min_dx; dx<=max_dx; dx+=iDiamondSize)
		for (dy = min_dy; dy<= max_dy; dy+=iDiamondSize)
			NOCHECK_MV16_CANDIDATE(dx,dy);

	return iMinSAD;
}

int32_t Full8_MainSearch(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const uint8_t * const cur,
					const int x, const int y,
					int32_t startx, int32_t starty,	
					int32_t iMinSAD,
					VECTOR * const currMV,
					const VECTOR * const pmv,
					const int32_t min_dx, const int32_t max_dx, 
					const int32_t min_dy, const int32_t max_dy,
					const int32_t iEdgedWidth, 
					const int32_t iDiamondSize, 
					const int32_t iFcode,
					const int32_t iQuant,
					int iFound)
{
	int32_t iSAD;
	int32_t dx,dy;
	VECTOR backupMV;
	backupMV.x = startx;
	backupMV.y = starty;
	
	for (dx = min_dx; dx<=max_dx; dx+=iDiamondSize)
		for (dy = min_dy; dy<= max_dy; dy+=iDiamondSize)
			NOCHECK_MV8_CANDIDATE(dx,dy);

	return iMinSAD;
}



int32_t Halfpel16_Refine(
	const uint8_t * const pRef,
	const uint8_t * const pRefH,
	const uint8_t * const pRefV,
	const uint8_t * const pRefHV,
	const uint8_t * const cur,
	const int x, const int y,
	VECTOR * const currMV,
	int32_t iMinSAD,
	const VECTOR * const pmv,
	const int32_t min_dx, const int32_t max_dx, 
	const int32_t min_dy, const int32_t max_dy,
	const int32_t iFcode,
	const int32_t iQuant,
	const int32_t iEdgedWidth)
{
/* Do a half-pel refinement (or rather a "smallest possible amount" refinement) */

	int32_t iSAD;
	VECTOR backupMV = *currMV;
	
	CHECK_MV16_CANDIDATE(backupMV.x-1,backupMV.y-1);
	CHECK_MV16_CANDIDATE(backupMV.x  ,backupMV.y-1);
	CHECK_MV16_CANDIDATE(backupMV.x+1,backupMV.y-1);
	CHECK_MV16_CANDIDATE(backupMV.x-1,backupMV.y);
	CHECK_MV16_CANDIDATE(backupMV.x+1,backupMV.y);
	CHECK_MV16_CANDIDATE(backupMV.x-1,backupMV.y+1);
	CHECK_MV16_CANDIDATE(backupMV.x  ,backupMV.y+1);
	CHECK_MV16_CANDIDATE(backupMV.x+1,backupMV.y+1);
	
	return iMinSAD;
}

#define PMV_HALFPEL16 (PMV_HALFPELDIAMOND16|PMV_HALFPELREFINE16)


int32_t PMVfastSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
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
	
	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;
		
	int32_t iFound;

	VECTOR newMV;
	VECTOR backupMV;	/* just for PMVFAST */
	
	VECTOR pmv[4];
	int32_t psad[4];
	
	MACROBLOCK * const pMB = pMBs + x + y * iWcount;

	static int32_t threshA,threshB;
    	int32_t bPredEq;
    	int32_t iMinSAD,iSAD;

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy,
		  x, y, 16, iWidth, iHeight, iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if (!(MotionFlags & PMV_HALFPEL16 ))
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

	if (!(MotionFlags & PMV_HALFPELDIAMOND16) )
		iDiamondSize*=2;

/* Step 4: Calculate SAD around the Median prediction. 
   MinSAD=SAD 
   If Motion Vector equal to Previous frame motion vector 
   and MinSAD<PrevFrmSAD goto Step 10. 
   If SAD<=256 goto Step 10. 
*/	


// Prepare for main loop 

	*currMV=pmv[0];		/* current best := prediction */
	if (!(MotionFlags & PMV_HALFPEL16 ))
	{ 	/* This should NOT be necessary! */
		currMV->x = EVEN(currMV->x);
		currMV->y = EVEN(currMV->y);
	}
	
	if (currMV->x > max_dx) 	
	{	
		currMV->x=max_dx;
	}
	if (currMV->x < min_dx) 
	{	
		currMV->x=min_dx;
	}
	if (currMV->y > max_dy) 
	{	
		currMV->y=max_dy;
	}
	if (currMV->y < min_dy) 
	{	
		currMV->y=min_dy;
	}
	
	iMinSAD = sad16( cur, 
			 get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 16, currMV, iEdgedWidth),
			 iEdgedWidth, MV_MAX_ERROR);
  	iMinSAD += calc_delta_16(currMV->x-pmv[0].x, currMV->y-pmv[0].y, (uint8_t)iFcode) * iQuant;
	
	if ( (iMinSAD < 256 ) || ( (MVequal(*currMV,pMB->mvs[0])) && (iMinSAD < pMB->sad16) ) )
	{
		
		if (MotionFlags & PMV_QUICKSTOP16) 
			goto PMVfast16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast16_Terminate_with_Refine;
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
		if (!(MotionFlags & PMV_HALFPEL16 ))
		{	pmv[1].x = EVEN(pmv[1].x);
		pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV16_CANDIDATE(pmv[1].x,pmv[1].y);		
	}

// top neighbour, if allowed
	if (y != 0)
	{	
		if (!(MotionFlags & PMV_HALFPEL16 ))
		{	pmv[2].x = EVEN(pmv[2].x);
		pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV16_CANDIDATE(pmv[2].x,pmv[2].y);
	
// top right neighbour, if allowed
		if (x != (iWcount-1))
		{
			if (!(MotionFlags & PMV_HALFPEL16 ))
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
		if (MotionFlags & PMV_QUICKSTOP16) 
			goto PMVfast16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast16_Terminate_with_Refine;
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

	backupMV = *currMV; /* save best prediction, actually only for EXTSEARCH */

/* default: use best prediction as starting point for one call of PMVfast_MainSearch */
	iSAD = Diamond16_MainSearch(pRef, pRefH, pRefV, pRefHV, cur,
					  x, y, 
					  currMV->x, currMV->y, iMinSAD, &newMV, 
					  pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, iFound);
	
	if (iSAD < iMinSAD) 
	{
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH16)
	{
/* extended: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0],backupMV)) )
		{ 	iSAD = Diamond16_MainSearch(pRef, pRefH, pRefV, pRefHV, cur,
							  x, y, 
							  pmv[0].x, pmv[0].y, iMinSAD, &newMV, 
							  pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, iFound);
		
		if (iSAD < iMinSAD) 
		{
			*currMV = newMV;
			iMinSAD = iSAD;
		}
		}

		if ( (!(MVzero(pmv[0]))) && (!(MVzero(backupMV))) )
		{ 	iSAD = Diamond16_MainSearch(pRef, pRefH, pRefV, pRefHV, cur,
							  x, y, 
							  0, 0, iMinSAD, &newMV, 
							  pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, iFound);
		
		if (iSAD < iMinSAD) 
		{
			*currMV = newMV;
			iMinSAD = iSAD;
		}
		}
	}

/* 
   Step 10:  The motion vector is chosen according to the block corresponding to MinSAD.
*/

PMVfast16_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE16) 		// perform final half-pel step 
		iMinSAD = Halfpel16_Refine( pRef, pRefH, pRefV, pRefHV, cur,
				  x, y,
				  currMV, iMinSAD, 
				  pmv, min_dx, max_dx, min_dy, max_dy, iFcode, iQuant, iEdgedWidth);

PMVfast16_Terminate_without_Refine:
	currPMV->x = currMV->x - pmv[0].x;
	currPMV->y = currMV->y - pmv[0].y;
	return iMinSAD;
}






int32_t Diamond8_MainSearch(
	const uint8_t * const pRef,
	const uint8_t * const pRefH,
	const uint8_t * const pRefV,
	const uint8_t * const pRefHV,
	const uint8_t * const cur,
	const int x, const int y,
	int32_t startx, int32_t starty,	
	int32_t iMinSAD,
	VECTOR * const currMV,
	const VECTOR * const pmv,
	const int32_t min_dx, const int32_t max_dx, 
	const int32_t min_dy, const int32_t max_dy,
	const int32_t iEdgedWidth, 
	const int32_t iDiamondSize, 
	const int32_t iFcode,
	const int32_t iQuant,
	int iFound)
{
/* Do a diamond search around given starting point, return SAD of best */

	int32_t iDirection=0;
	int32_t iSAD;
	VECTOR backupMV;
	backupMV.x = startx;
	backupMV.y = starty;
	
/* It's one search with full Diamond pattern, and only 3 of 4 for all following diamonds */

	CHECK_MV8_CANDIDATE_DIR(backupMV.x-iDiamondSize,backupMV.y,1);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x+iDiamondSize,backupMV.y,2);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x,backupMV.y-iDiamondSize,3);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x,backupMV.y+iDiamondSize,4);

	if (iDirection)
		while (!iFound)
		{	
			iFound = 1; 
			backupMV=*currMV;	// since iDirection!=0, this is well defined!
	
			if ( iDirection != 2) 
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x-iDiamondSize,backupMV.y,1);
			if ( iDirection != 1) 
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x+iDiamondSize,backupMV.y,2);
			if ( iDirection != 4) 
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x,backupMV.y-iDiamondSize,3);
			if ( iDirection != 3) 
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x,backupMV.y+iDiamondSize,4);
		}
	else
	{	
		currMV->x = startx;
		currMV->y = starty;
	}
	return iMinSAD;
}

int32_t Halfpel8_Refine(
	const uint8_t * const pRef,
	const uint8_t * const pRefH,
	const uint8_t * const pRefV,
	const uint8_t * const pRefHV,
	const uint8_t * const cur,
	const int x, const int y,
	VECTOR * const currMV,
	int32_t iMinSAD,
	const VECTOR * const pmv,
	const int32_t min_dx, const int32_t max_dx, 
	const int32_t min_dy, const int32_t max_dy,
	const int32_t iFcode,
	const int32_t iQuant,
	const int32_t iEdgedWidth)
{
/* Do a half-pel refinement (or rather a "smallest possible amount" refinement) */

	int32_t iSAD;
	VECTOR backupMV = *currMV;
	
	CHECK_MV8_CANDIDATE(backupMV.x-1,backupMV.y-1);
	CHECK_MV8_CANDIDATE(backupMV.x  ,backupMV.y-1);
	CHECK_MV8_CANDIDATE(backupMV.x+1,backupMV.y-1);
	CHECK_MV8_CANDIDATE(backupMV.x-1,backupMV.y);
	CHECK_MV8_CANDIDATE(backupMV.x+1,backupMV.y);
	CHECK_MV8_CANDIDATE(backupMV.x-1,backupMV.y+1);
	CHECK_MV8_CANDIDATE(backupMV.x  ,backupMV.y+1);
	CHECK_MV8_CANDIDATE(backupMV.x+1,backupMV.y+1);
	
	return iMinSAD;
}


#define PMV_HALFPEL8 (PMV_HALFPELDIAMOND8|PMV_HALFPELREFINE8)

int32_t PMVfastSearch8(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int start_x, int start_y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
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

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;
		
	VECTOR pmv[4];
	int32_t psad[4];
	VECTOR newMV;
	VECTOR backupMV;
	
	MACROBLOCK * const pMB = pMBs + (x>>1) + (y>>1) * iWcount;

	static int32_t threshA,threshB;
    	int32_t iFound,bPredEq;
    	int32_t iMinSAD,iSAD;

	int32_t iSubBlock = ((y&1)<<1) + (x&1);

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy,
		  x, y, 8, iWidth, iHeight, iFcode);

/* we work with abs. MVs, not relative to prediction, so range is relative to 0,0 */

	if (!(MotionFlags & PMV_HALFPELDIAMOND8 ))
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

	if (!(MotionFlags & PMV_HALFPELDIAMOND8) )
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
	
	iMinSAD = sad8( cur, 
			get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 8, currMV, iEdgedWidth),
			iEdgedWidth);
  	iMinSAD += calc_delta_8(currMV->x - pmv[0].x, currMV->y - pmv[0].y, (uint8_t)iFcode) * iQuant;
	
	if ( (iMinSAD < 256/4 ) || ( (MVequal(*currMV,pMB->mvs[iSubBlock])) && (iMinSAD < pMB->sad8[iSubBlock]) ) )
	{
		if (MotionFlags & PMV_QUICKSTOP16) 
			goto PMVfast8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast8_Terminate_with_Refine;
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
		if (!(MotionFlags & PMV_HALFPEL8 ))	
		{	pmv[1].x = EVEN(pmv[1].x);	
		pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV8_CANDIDATE(pmv[1].x,pmv[1].y);		
	}

// top neighbour, if allowed
	if (psad[2] != MV_MAX_ERROR) 
	{	
		if (!(MotionFlags & PMV_HALFPEL8 ))
		{	pmv[2].x = EVEN(pmv[2].x);
		pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV8_CANDIDATE(pmv[2].x,pmv[2].y);
	
// top right neighbour, if allowed
		if (psad[3] != MV_MAX_ERROR) 
		{
			if (!(MotionFlags & PMV_HALFPEL8 ))
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
		if (MotionFlags & PMV_QUICKSTOP16) 
			goto PMVfast8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast8_Terminate_with_Refine;
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

	backupMV = *currMV; /* save best prediction, actually only for EXTSEARCH */

/* default: use best prediction as starting point for one call of PMVfast_MainSearch */
	iSAD = Diamond8_MainSearch(pRef, pRefH, pRefV, pRefHV, cur,
					 x, y, 
					 currMV->x, currMV->y, iMinSAD, &newMV, 
					 pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, iFound);
	
	if (iSAD < iMinSAD) 
	{
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH8)
	{
/* extended: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0],backupMV)) )
		{ 	iSAD = Diamond16_MainSearch(pRef, pRefH, pRefV, pRefHV, cur,
							  x, y, 
							  pmv[0].x, pmv[0].y, iMinSAD, &newMV, 
							  pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, iFound);
		
		if (iSAD < iMinSAD) 
		{
			*currMV = newMV;
			iMinSAD = iSAD;
		}
		}

		if ( (!(MVzero(pmv[0]))) && (!(MVzero(backupMV))) )
		{ 	iSAD = Diamond16_MainSearch(pRef, pRefH, pRefV, pRefHV, cur,
							  x, y, 
							  0, 0, iMinSAD, &newMV, 
							  pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, iFound);
		
		if (iSAD < iMinSAD) 
		{
			*currMV = newMV;
			iMinSAD = iSAD;
		}
		}
	}

/* Step 10: The motion vector is chosen according to the block corresponding to MinSAD.
   By performing an optional local half-pixel search, we can refine this result even further.
*/
	
PMVfast8_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE8) 		// perform final half-pel step 
		iMinSAD = Halfpel8_Refine( pRef, pRefH, pRefV, pRefHV, cur,
						 x, y,
						 currMV, iMinSAD, 
						 pmv, min_dx, max_dx, min_dy, max_dy, iFcode, iQuant, iEdgedWidth);


PMVfast8_Terminate_without_Refine:
	currPMV->x = currMV->x - pmv[0].x;
	currPMV->y = currMV->y - pmv[0].y;
	
	return iMinSAD;
}

int32_t EPZSSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
					MACROBLOCK * const pMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV)
{
        const uint32_t iWcount = pParam->mb_width;
        const uint32_t iHcount = pParam->mb_height;
	const int32_t iFcode = pParam->fixed_code;
	const int32_t iQuant = pParam->quant;

	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width; 

	const uint8_t * cur = pCur->y + x*16 + y*16*iEdgedWidth;

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;
		
	VECTOR newMV;
	VECTOR backupMV;
	
	VECTOR pmv[4];
	int32_t psad[8];
	
	static MACROBLOCK * oldMBs = NULL; 
	MACROBLOCK * const pMB = pMBs + x + y * iWcount;
	MACROBLOCK * oldMB = NULL;

	static int32_t thresh2;
    	int32_t bPredEq;
    	int32_t iMinSAD,iSAD=9999;

	MainSearch16FuncPtr EPZSMainSearchPtr;

	if (oldMBs == NULL)
	{	oldMBs = (MACROBLOCK*) calloc(1,iWcount*iHcount*sizeof(MACROBLOCK));
		fprintf(stderr,"allocated %d bytes for oldMBs\n",iWcount*iHcount*sizeof(MACROBLOCK));
	}
	oldMB = oldMBs + x + y * iWcount;

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy,
			x, y, 16, iWidth, iHeight, iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if (!(MotionFlags & PMV_HALFPEL16 ))
	{ min_dx = EVEN(min_dx);
	  max_dx = EVEN(max_dx);
	  min_dy = EVEN(min_dy);
	  max_dy = EVEN(max_dy); 
	}		/* because we might use something like IF (dx>max_dx) THEN dx=max_dx; */
		
	bPredEq  = get_pmvdata(pMBs, x, y, iWcount, 0, pmv, psad);

/* Step 4: Calculate SAD around the Median prediction. 
        MinSAD=SAD 
        If Motion Vector equal to Previous frame motion vector 
		and MinSAD<PrevFrmSAD goto Step 10. 
        If SAD<=256 goto Step 10. 
*/	

// Prepare for main loop 

	*currMV=pmv[0];		/* current best := median prediction */
	if (!(MotionFlags & PMV_HALFPEL16))
	{ 	
		currMV->x = EVEN(currMV->x);
		currMV->y = EVEN(currMV->y);
	}
	
	if (currMV->x > max_dx) 	
		currMV->x=max_dx;
	if (currMV->x < min_dx) 
		currMV->x=min_dx;
	if (currMV->y > max_dy) 
		currMV->y=max_dy;
	if (currMV->y < min_dy) 
		currMV->y=min_dy;

/***************** This is predictor SET A: only median prediction ******************/ 
	
	iMinSAD = sad16( cur, 
		get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 16, currMV, iEdgedWidth),
		iEdgedWidth, MV_MAX_ERROR);
  	iMinSAD += calc_delta_16(currMV->x-pmv[0].x, currMV->y-pmv[0].y, (uint8_t)iFcode) * iQuant;
	
// thresh1 is fixed to 256 
	if ( (iMinSAD < 256 ) || ( (MVequal(*currMV,pMB->mvs[0])) && (iMinSAD < pMB->sad16) ) )
		{
			if (MotionFlags & PMV_QUICKSTOP16) 
				goto EPZS16_Terminate_without_Refine;
			if (MotionFlags & PMV_EARLYSTOP16) 
				goto EPZS16_Terminate_with_Refine;
		}

/************** This is predictor SET B: (0,0), prev.frame MV, neighbours **************/ 

// previous frame MV 
	CHECK_MV16_CANDIDATE(pMB->mvs[0].x,pMB->mvs[0].y);

// set threshhold based on Min of Prediction and SAD of collocated block
// CHECK_MV16 always uses iSAD for the SAD of last vector to check, so now iSAD is what we want

	if ((x==0) && (y==0) )
	{
		thresh2 =  512;
	}
	else
	{
/* T_k = 1.2 * MIN(SAD_top,SAD_left,SAD_topleft,SAD_coll) +128;   [Tourapis, 2002] */

		thresh2 = MIN(psad[0],iSAD)*6/5 + 128;	
	}

// MV=(0,0) is often a good choice

	CHECK_MV16_ZERO;

	
// left neighbour, if allowed
	if (x != 0) 
	{
		if (!(MotionFlags & PMV_HALFPEL16 ))
		{	pmv[1].x = EVEN(pmv[1].x);
			pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV16_CANDIDATE(pmv[1].x,pmv[1].y);		
	}

// top neighbour, if allowed
	if (y != 0)
	{	
		if (!(MotionFlags & PMV_HALFPEL16 ))
		{	pmv[2].x = EVEN(pmv[2].x);
			pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV16_CANDIDATE(pmv[2].x,pmv[2].y);
	
// top right neighbour, if allowed
		if (x != (iWcount-1))
		{
			if (!(MotionFlags & PMV_HALFPEL16 ))
			{	pmv[3].x = EVEN(pmv[3].x);
				pmv[3].y = EVEN(pmv[3].y);
			}
			CHECK_MV16_CANDIDATE(pmv[3].x,pmv[3].y);
		}
	}

/* Terminate if MinSAD <= T_2 
   Terminate if MV[t] == MV[t-1] and MinSAD[t] <= MinSAD[t-1] 
*/

	if ( (iMinSAD <= thresh2) 
		|| ( MVequal(*currMV,pMB->mvs[0]) && (iMinSAD <= pMB->sad16) ) )
		{	
			if (MotionFlags & PMV_QUICKSTOP16) 
				goto EPZS16_Terminate_without_Refine;
			if (MotionFlags & PMV_EARLYSTOP16) 
				goto EPZS16_Terminate_with_Refine;
		}

/***** predictor SET C: acceleration MV (new!), neighbours in prev. frame(new!) ****/

	backupMV = pMB->mvs[0]; 		// last MV
	backupMV.x += (pMB->mvs[0].x - oldMB->mvs[0].x );	// acceleration X
	backupMV.y += (pMB->mvs[0].y - oldMB->mvs[0].y );	// acceleration Y 

	CHECK_MV16_CANDIDATE(backupMV.x,backupMV.y);	

// left neighbour
	if (x != 0)  
		CHECK_MV16_CANDIDATE((oldMB-1)->mvs[0].x,oldMB->mvs[0].y);		

// top neighbour 
	if (y != 0)
		CHECK_MV16_CANDIDATE((oldMB-iWcount)->mvs[0].x,oldMB->mvs[0].y);		

// right neighbour, if allowed (this value is not written yet, so take it from   pMB->mvs 

	if (x != iWcount-1)
		CHECK_MV16_CANDIDATE((pMB+1)->mvs[0].x,oldMB->mvs[0].y);		

// bottom neighbour, dito
	if (y != iHcount-1)
		CHECK_MV16_CANDIDATE((pMB+iWcount)->mvs[0].x,oldMB->mvs[0].y);		

/* Terminate if MinSAD <= T_3 (here T_3 = T_2)  */
	if (iMinSAD <= thresh2)
		{	
			if (MotionFlags & PMV_QUICKSTOP16) 
				goto EPZS16_Terminate_without_Refine;
			if (MotionFlags & PMV_EARLYSTOP16) 
				goto EPZS16_Terminate_with_Refine;
		}

/************ (if Diamond Search)  **************/

	backupMV = *currMV; /* save best prediction, actually only for EXTSEARCH */

/* default: use best prediction as starting point for one call of PMVfast_MainSearch */

	if (MotionFlags & PMV_USESQUARES16)
		EPZSMainSearchPtr = Square16_MainSearch;
	else
		EPZSMainSearchPtr = Diamond16_MainSearch;

	iSAD = (*EPZSMainSearchPtr)(pRef, pRefH, pRefV, pRefHV, cur,
			x, y, 
			currMV->x, currMV->y, iMinSAD, &newMV, pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, 
			2, iFcode, iQuant, 0);
	
	if (iSAD < iMinSAD) 
	{
		*currMV = newMV;
		iMinSAD = iSAD;
	}


	if (MotionFlags & PMV_EXTSEARCH16)
	{
/* extended mode: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0],backupMV)) )
		{ 	
			iSAD = (*EPZSMainSearchPtr)(pRef, pRefH, pRefV, pRefHV, cur,
				x, y, 
				pmv[0].x, pmv[0].y, iMinSAD, &newMV, 
				pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, 2, iFcode, iQuant, 0);
		}
			
		if (iSAD < iMinSAD) 
		{
			*currMV = newMV;
			iMinSAD = iSAD;
		}
	
		if ( (!(MVzero(pmv[0]))) && (!(MVzero(backupMV))) )
		{ 	
			iSAD = (*EPZSMainSearchPtr)(pRef, pRefH, pRefV, pRefHV, cur,
				x, y, 
			0, 0, iMinSAD, &newMV, 
			pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, /*iDiamondSize*/ 2, iFcode, iQuant, 0);
		
			if (iSAD < iMinSAD) 
			{
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/*************** 	Choose best MV found     **************/

EPZS16_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE16) 		// perform final half-pel step 
		iMinSAD = Halfpel16_Refine( pRef, pRefH, pRefV, pRefHV, cur,
				x, y,
				currMV, iMinSAD, 
				pmv, min_dx, max_dx, min_dy, max_dy, iFcode, iQuant, iEdgedWidth);

EPZS16_Terminate_without_Refine:

	*oldMB = *pMB;
	
	currPMV->x = currMV->x - pmv[0].x;
	currPMV->y = currMV->y - pmv[0].y;
	return iMinSAD;
}


int32_t EPZSSearch8(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const int start_x, int start_y,
					const uint32_t MotionFlags,
					const MBParam * const pParam,
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

	int32_t iDiamondSize=1;
	
	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;
		
	VECTOR newMV;
	VECTOR backupMV;
	
	VECTOR pmv[4];
	int32_t psad[8];

	const	int32_t iSubBlock = ((y&1)<<1) + (x&1);
	
	MACROBLOCK * const pMB = pMBs + (x>>1) + (y>>1) * iWcount;

    	int32_t bPredEq;
    	int32_t iMinSAD,iSAD=9999;

	MainSearch8FuncPtr EPZSMainSearchPtr;

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy,
			x, y, 8, iWidth, iHeight, iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if (!(MotionFlags & PMV_HALFPEL8 ))
	{ min_dx = EVEN(min_dx);
	  max_dx = EVEN(max_dx);
	  min_dy = EVEN(min_dy);
	  max_dy = EVEN(max_dy); 
	}		/* because we might use something like IF (dx>max_dx) THEN dx=max_dx; */
		
	bPredEq  = get_pmvdata(pMBs, x>>1, y>>1, iWcount, iSubBlock, pmv, psad);


/* Step 4: Calculate SAD around the Median prediction. 
        MinSAD=SAD 
        If Motion Vector equal to Previous frame motion vector 
		and MinSAD<PrevFrmSAD goto Step 10. 
        If SAD<=256 goto Step 10. 
*/

// Prepare for main loop 

	
	if (!(MotionFlags & PMV_HALFPEL8))
	{ 	
		currMV->x = EVEN(currMV->x);
		currMV->y = EVEN(currMV->y);
	}
	
	if (currMV->x > max_dx) 	
		currMV->x=max_dx;
	if (currMV->x < min_dx) 
		currMV->x=min_dx;
	if (currMV->y > max_dy) 
		currMV->y=max_dy;
	if (currMV->y < min_dy) 
		currMV->y=min_dy;

/***************** This is predictor SET A: only median prediction ******************/ 

	
	iMinSAD = sad8( cur, 
		get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 8, currMV, iEdgedWidth),
		iEdgedWidth);
  	iMinSAD += calc_delta_8(currMV->x-pmv[0].x, currMV->y-pmv[0].y, (uint8_t)iFcode) * iQuant;

	
// thresh1 is fixed to 256 
	if (iMinSAD < 256/4 )
		{
			if (MotionFlags & PMV_QUICKSTOP8) 
				goto EPZS8_Terminate_without_Refine;
			if (MotionFlags & PMV_EARLYSTOP8) 
				goto EPZS8_Terminate_with_Refine;
		}

/************** This is predictor SET B: (0,0), prev.frame MV, neighbours **************/ 

// previous frame MV 
	CHECK_MV8_CANDIDATE(pMB->mvs[0].x,pMB->mvs[0].y);

// MV=(0,0) is often a good choice

	CHECK_MV8_ZERO;

/* Terminate if MinSAD <= T_2 
   Terminate if MV[t] == MV[t-1] and MinSAD[t] <= MinSAD[t-1] 
*/

	if (iMinSAD < 512/4) 	/* T_2 == 512/4 hardcoded */
		{
			if (MotionFlags & PMV_QUICKSTOP8) 
				goto EPZS8_Terminate_without_Refine;
			if (MotionFlags & PMV_EARLYSTOP8) 
				goto EPZS8_Terminate_with_Refine;
		}

/************ (if Diamond Search)  **************/

	backupMV = *currMV; /* save best prediction, actually only for EXTSEARCH */

	if (!(MotionFlags & PMV_HALFPELDIAMOND8))
		iDiamondSize *= 2;
		
/* default: use best prediction as starting point for one call of PMVfast_MainSearch */

//	if (MotionFlags & PMV_USESQUARES8)
//		EPZSMainSearchPtr = Square8_MainSearch;
//	else
		EPZSMainSearchPtr = Diamond8_MainSearch;
		
	iSAD = (*EPZSMainSearchPtr)(pRef, pRefH, pRefV, pRefHV, cur,
		x, y, 
		currMV->x, currMV->y, iMinSAD, &newMV, 
		pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, 
		iDiamondSize, iFcode, iQuant, 00);

	
	if (iSAD < iMinSAD) 
	{
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH8)
	{
/* extended mode: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0],backupMV)) )
		{ 	
			iSAD = (*EPZSMainSearchPtr)(pRef, pRefH, pRefV, pRefHV, cur,
				x, y, 
			pmv[0].x, pmv[0].y, iMinSAD, &newMV, 
			pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, 0);
		
			if (iSAD < iMinSAD) 
			{
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}

		if ( (!(MVzero(pmv[0]))) && (!(MVzero(backupMV))) )
		{ 	
			iSAD = (*EPZSMainSearchPtr)(pRef, pRefH, pRefV, pRefHV, cur,
				x, y, 
			0, 0, iMinSAD, &newMV, 
			pmv, min_dx, max_dx, min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode, iQuant, 0);
		
			if (iSAD < iMinSAD) 
			{
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/*************** 	Choose best MV found     **************/

EPZS8_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE8) 		// perform final half-pel step 
		iMinSAD = Halfpel8_Refine( pRef, pRefH, pRefV, pRefHV, cur,
				x, y,
				currMV, iMinSAD, 
				pmv, min_dx, max_dx, min_dy, max_dy, iFcode, iQuant, iEdgedWidth);

EPZS8_Terminate_without_Refine:

	currPMV->x = currMV->x - pmv[0].x;
	currPMV->y = currMV->y - pmv[0].y;
	return iMinSAD;
}

