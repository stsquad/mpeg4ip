#ifndef _ENCODER_CBP_H_
#define _ENCODER_CBP_H_

#include "../portab.h"

typedef uint32_t (cbpFunc)(const int16_t codes[6][64]);

typedef cbpFunc* cbpFuncPtr;	

extern cbpFuncPtr calc_cbp;

extern cbpFunc calc_cbp_c;
extern cbpFunc calc_cbp_mmx;

#endif /* _ENCODER_CBP_H_ */
