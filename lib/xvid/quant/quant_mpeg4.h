#ifndef _QUANT_MPEG4_H_
#define _QUANT_MPEG4_H_

#include "../portab.h"

// intra
typedef void (quant_intraFunc)(int16_t * coeff,
				const int16_t * data,
				const uint32_t quant,
				const uint32_t dcscalar);

typedef quant_intraFunc* quant_intraFuncPtr;	

extern quant_intraFuncPtr quant4_intra;
extern quant_intraFuncPtr dequant4_intra;

quant_intraFunc quant4_intra_c;
quant_intraFunc quant4_intra_mmx;

quant_intraFunc dequant4_intra_c;
quant_intraFunc dequant4_intra_mmx;

// inter_quant
typedef uint32_t (quant_interFunc)(int16_t *coeff,
				const int16_t *data,
				const uint32_t quant);

typedef quant_interFunc* quant_interFuncPtr;	

extern quant_interFuncPtr quant4_inter;

quant_interFunc quant4_inter_c;
quant_interFunc quant4_inter_mmx;

//inter_dequant
typedef void (dequant_interFunc)(int16_t *coeff,
				const int16_t *data,
				const uint32_t quant);

typedef dequant_interFunc* dequant_interFuncPtr;	

extern dequant_interFuncPtr dequant4_inter;

dequant_interFunc dequant4_inter_c;
dequant_interFunc dequant4_inter_mmx;

#endif /* _QUANT_MPEG4_H_ */
