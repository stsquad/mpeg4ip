#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "xvid.h"
#include "portab.h"

/* --- macroblock stuff --- */

#define MODE_INTER		0
#define MODE_INTER_Q	1
#define MODE_INTER4V	2
#define	MODE_INTRA		3
#define MODE_INTRA_Q	4
#define MODE_STUFFING	7
#define MODE_NOT_CODED	16

typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t buf;
	uint32_t pos;
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
} 
Bitstream;


#define MBPRED_SIZE  15


typedef struct
{
	// decoder/encoder 
	VECTOR mvs[4];
	uint32_t sad8[4];		// SAD values for inter4v-VECTORs
	uint32_t sad16;			// SAD value for inter-VECTOR

    short int pred_values[6][MBPRED_SIZE];
    int acpred_directions[6];
    
	int mode;
	int quant;		// absolute quant

	int field_dct;
	int field_pred;
	int field_for_top;
	int field_for_bot;

	// encoder specific

	VECTOR pmvs[4];
	int dquant;
	int cbp;

} MACROBLOCK;

static __inline int8_t get_dc_scaler(int32_t quant, uint32_t lum)
{
    int8_t dc_scaler;

	if(quant > 0 && quant < 5) {
        dc_scaler = 8;
		return dc_scaler;
	}

	if(quant < 25 && !lum) {
        dc_scaler = (quant + 13) >> 1;
		return dc_scaler;
	}

	if(quant < 9) {
        dc_scaler = quant << 1;
		return dc_scaler;
	}

    if(quant < 25) {
        dc_scaler = quant + 8;
		return dc_scaler;
	}

	if(lum)
		dc_scaler = (quant << 1) - 16;
	else
        dc_scaler = quant - 6;

    return dc_scaler;
}

#endif /* _GLOBAL_H_ */
