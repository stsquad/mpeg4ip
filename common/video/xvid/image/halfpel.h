#ifndef _IMAGE_HALFPEL_H_
#define _IMAGE_HALFPEL_H_


#include "../portab.h"


typedef void (halfpelFunc)(uint8_t * const dst, 
						   const uint8_t * const src,
						   const uint32_t width, 
						   const uint32_t height,
						   const uint32_t rounding);

typedef halfpelFunc* halfpelFuncPtr;	

extern halfpelFuncPtr interpolate_halfpel_h;
extern halfpelFuncPtr interpolate_halfpel_v;
extern halfpelFuncPtr interpolate_halfpel_hv;

halfpelFunc interpolate_halfpel_h_c;
halfpelFunc interpolate_halfpel_v_c;
halfpelFunc interpolate_halfpel_hv_c;

/* mmx & 3dnow */

halfpelFunc interpolate_halfpel_h_mmx;
halfpelFunc interpolate_halfpel_h_3dn;
halfpelFunc interpolate_halfpel_v_mmx;
halfpelFunc interpolate_halfpel_v_3dn;
halfpelFunc interpolate_halfpel_hv_mmx;

#endif /* _IMAGE_HALFPEL_H_ */
