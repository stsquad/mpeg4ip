#ifndef _QUANT_H263_H_
#define _QUANT_H263_H_

#include "../portab.h"

// intra
typedef void (quanth263_intraFunc)(int16_t * coeff,
				const int16_t * data,
				const uint32_t quant,
				const uint32_t dcscalar);

typedef quanth263_intraFunc* quanth263_intraFuncPtr;	

extern quanth263_intraFuncPtr quant_intra;
extern quanth263_intraFuncPtr dequant_intra;

quanth263_intraFunc quant_intra_c;
quanth263_intraFunc quant_intra_mmx;

quanth263_intraFunc dequant_intra_c;
quanth263_intraFunc dequant_intra_mmx;

// inter_quant
typedef uint32_t (quanth263_interFunc)(int16_t *coeff,
				const int16_t *data,
				const uint32_t quant);

typedef quanth263_interFunc* quanth263_interFuncPtr;	

extern quanth263_interFuncPtr quant_inter;

quanth263_interFunc quant_inter_c;
quanth263_interFunc quant_inter_mmx;

//inter_dequant
typedef void (dequanth263_interFunc)(int16_t *coeff,
				const int16_t *data,
				const uint32_t quant);

typedef dequanth263_interFunc* dequanth263_interFuncPtr;	

extern dequanth263_interFuncPtr dequant_inter;

dequanth263_interFunc dequant_inter_c;
dequanth263_interFunc dequant_inter_mmx;

#endif /* _QUANT_H263_H_ */
