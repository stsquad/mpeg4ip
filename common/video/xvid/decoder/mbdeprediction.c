#include "../portab.h"
#include "../common/common.h"

#include "mbdeprediction.h"
#include "decoder.h"



/* decoder: add predictors to dct_codes[] and
   store current coeffs to pred_values[] for future prediction 
*/


void add_acdc(MACROBLOCK *pMB,
				uint32_t block, 
				int16_t dct_codes[64],
				uint32_t iDcScaler,
				int16_t predictors[8])
{
	uint8_t acpred_direction = pMB->acpred_directions[block];
	int16_t * pCurrent = pMB->pred_values[block];
	uint32_t i;

	dct_codes[0] += predictors[0];	// dc prediction
	pCurrent[0] = dct_codes[0] * iDcScaler;

	if (acpred_direction == 1)
	{
		for (i = 1; i < 8; i++)
		{
			int level = dct_codes[i] + predictors[i];
			dct_codes[i] = level;
			pCurrent[i] = level;
			pCurrent[i+7] = dct_codes[i*8];
		}
	}
	else if (acpred_direction == 2)
	{
		for (i = 1; i < 8; i++)
		{
			int level = dct_codes[i*8] + predictors[i];
			dct_codes[i*8] = level;
			pCurrent[i+7] = level;
			pCurrent[i] = dct_codes[i];
		}
	}
	else
	{
		for (i = 1; i < 8; i++)
		{
			pCurrent[i] = dct_codes[i];
			pCurrent[i+7] = dct_codes[i*8];
		}
	}
}
