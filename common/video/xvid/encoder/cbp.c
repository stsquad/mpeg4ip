#include "../portab.h"
#include "cbp.h"

cbpFuncPtr calc_cbp;

uint32_t calc_cbp_c(const int16_t codes[6][64])
{
    uint32_t i, j;
    uint32_t cbp = 0;

    for (i = 0; i < 6; i++) {
		for (j = 1; j < 64; j++) {
			if (codes[i][j]) {
				cbp |= 1 << (5 - i);
				break;
			}
		}
    }
    return cbp;
}
