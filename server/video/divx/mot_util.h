
#ifndef _MOT_UTIL_H_
#define _MOT_UTIL_H_

#include "limits.h"
#include "momusys.h"

/* search window for the 8x8 block MV; [-DEFAULT_8_WIN;DEFAULT_8_WIN] pixels
   arround the 16x16 motion vector */
//#define DEFAULT_8_WIN    2.0
#define DEFAULT_8_WIN    1.0f

#define B_SIZE           8

/* big value, returned if the MV is not good enought (2^25) */
#define MV_MAX_ERROR    0x2000000

/* Point structure */

typedef struct pixpoint
  {
  Int x;
  Int y;
  } PixPoint;

#define EHUFF struct Modified_Encoder_Huffman

EHUFF
{
  Int n;
  Int *Hlen;
  Int *Hcode;
};

#define  MVLEN(x,y)  (ABS(x) + ABS(y))

#include "mom_structs.h"
#include "vm_common_defs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Void  	InterpolateImage _P_((	Image *input_image,
			Image *output_image,
			Int rounding_control
	));
Int  	GetMotionImages _P_((	Image *imv16_w,
			Image *imv16_h,
			Image *imv8_w,
			Image *imv8_h,
			Image *imode16,
			Image **mv_x,
			Image **mv_y,
			Image **mode
	));
Int  	ChooseMode _P_((	SInt *curr,
			Int x_pos,
			Int y_pos,
			Int min_SAD,
			UInt width
	));
Int  	SAD_Macroblock _P_((	SInt *ii,
			SInt *act_block,
			UInt h_length,
			Int Min_FRAME
	));
Int  	SAD_Block _P_((	SInt *ii,
			SInt *act_block,
			UInt h_length,
			Int min_sofar
	));
Void 	LoadArea _P_((	SInt *im,
			Int x,
			Int y,
			Int x_size,
			Int y_size,
			Int lx,
			SInt *block
	));
Void  	SetArea _P_((	SInt *block,
			Int x,
			Int y,
			Int x_size,
			Int y_size,
			Int lx,
			SInt *im
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 


#endif /* _MOT_UTIL_H_ */
