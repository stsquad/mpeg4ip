#ifndef DECODER_COMPENSATE_HALFPEL_H
#define DECODER_COMPENSATE_HALFPEL_H


#include "../portab.h"

typedef void (comphalfpelFunc)(uint8_t * const dst,
					const uint8_t * const src,
					const int16_t * const data,
					const uint32_t stride,
					const uint32_t rounding);

typedef comphalfpelFunc* comphalfpelFuncPtr;	

extern comphalfpelFuncPtr compensate_halfpel_h;
extern comphalfpelFuncPtr compensate_halfpel_v;
extern comphalfpelFuncPtr compensate_halfpel_hv;

/* plain c */

comphalfpelFunc compensate_halfpel_h_c;
comphalfpelFunc compensate_halfpel_v_c;
comphalfpelFunc compensate_halfpel_hv_c;

/* mmx & 3dnow */

comphalfpelFunc compensate_halfpel_h_mmx;
comphalfpelFunc compensate_halfpel_h_3dn;
comphalfpelFunc compensate_halfpel_v_mmx;
comphalfpelFunc compensate_halfpel_v_3dn;
comphalfpelFunc compensate_halfpel_hv_mmx;


#endif /* DECODER_COMPENSATE_HALFPEL_H */
