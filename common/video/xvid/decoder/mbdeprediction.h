#ifndef _DECODER_MBDEPREDICTION_H_
#define _DECODER_MBDEPREDICTION_H_

#include "../portab.h"
#include "decoder.h"

void add_acdc(MACROBLOCK *pMB,
				uint32_t block, 
				int16_t dct_codes[64],
				uint32_t iDcScaler,
				int16_t predictors[8]);

#endif /* _DECODER_MBDEPREDICTION_H_ */
