#ifndef _ENCODER_CBP_H_
#define _ENCODER_CBP_H_

#include "../portab.h"

typedef uint32_t (cbpFunc)(const int16_t *codes);

typedef cbpFunc* cbpFuncPtr;	

extern cbpFuncPtr calc_cbp;

extern cbpFunc calc_cbp_c;
extern cbpFunc calc_cbp_mmx;
extern cbpFunc calc_cbp_ppc;
extern cbpFunc calc_cbp_altivec;

#endif /* _ENCODER_CBP_H_ */
