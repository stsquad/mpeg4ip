#ifndef _MBPREDICTION_H_
#define _MBPREDICTION_H_

#include "../portab.h"
#include "../decoder.h"
#include "../global.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))

// very large value
#define MV_MAX_ERROR	(4096 * 256)

#define MVequal(A,B) ( ((A).x)==((B).x) && ((A).y)==((B).y) )

void MBPrediction(MBParam *pParam,	 /* <-- the parameter for ACDC and MV prediction */
		  uint32_t x_pos,		 /* <-- The x position of the MB to be searched */
		  uint32_t y_pos, 		 /* <-- The y position of the MB to be searched */ 
		  uint32_t x_dim,		 /* <-- Number of macroblocks in a row */
		  int16_t *qcoeff, 	 /* <-> The quantized DCT coefficients */ 
		  MACROBLOCK *MB_array		 /* <-> the array of all the MB Infomations */
    );

void add_acdc(MACROBLOCK *pMB,
				uint32_t block, 
				int16_t dct_codes[64],
				uint32_t iDcScaler,
				int16_t predictors[8]);


void predict_acdc(MACROBLOCK *pMBs,
				uint32_t x, uint32_t y,	uint32_t mb_width, 
				uint32_t block, 
				int16_t qcoeff[64],
				uint32_t current_quant,
				int32_t iDcScaler,
				int16_t predictors[8]);

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


#endif /* _MBPREDICTION_H_ */
