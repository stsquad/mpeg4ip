#ifndef _COMMON_H_
#define _COMMON_H_

#include "../portab.h"



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
	int x;
	int y;
} VECTOR;


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

	// encoder specific

	VECTOR pmvs[4];
	int dquant;
	int cbp;

} MACROBLOCK;

void init_common(uint32_t cpu_flags);

void get_pmv(const MACROBLOCK * const pMBs,
							const uint32_t x, const uint32_t y,
							const uint32_t x_dim,
							const uint32_t block,
							int32_t * const pred_x, int32_t * const pred_y);

void predict_acdc(MACROBLOCK *pMBs,
				uint32_t x, uint32_t y,	uint32_t mb_width, 
				uint32_t block, 
				int16_t qcoeff[64],
				uint32_t current_quant,
				uint32_t iDcScaler,
				int16_t predictors[8]);


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


int check_cpu_features(void);


/* --- emms --- */

typedef void (emmsFunc)();
typedef emmsFunc* emmsFuncPtr;	

extern emmsFuncPtr emms;

emmsFunc emms_c;
emmsFunc emms_mmx;

							
/* --- fdct --- */

typedef void (fdctFunc)(short * const block);
typedef fdctFunc* fdctFuncPtr;	

extern fdctFuncPtr fdct;

fdctFunc fdct_int32;
fdctFunc fdct_mmx;


/* --- idct --- */

void idct_int32_init();

typedef void (idctFunc)(short * const block);
typedef idctFunc* idctFuncPtr;	

extern idctFuncPtr idct;

idctFunc idct_int32;
idctFunc idct_mmx;
idctFunc idct_xmm;


/* --- h263/MPEG4 quantization --- */

// intra
typedef void (quant_intraFunc)(int16_t * coeff,
				const int16_t * data,
				const uint32_t quant,
				const uint32_t dcscalar);

typedef quant_intraFunc* quant_intraFuncPtr;	

extern quant_intraFuncPtr quant_intra;
extern quant_intraFuncPtr dequant_intra;

extern quant_intraFuncPtr quant4_intra;
extern quant_intraFuncPtr dequant4_intra;

quant_intraFunc quant_intra_c;
quant_intraFunc dequant_intra_c;

quant_intraFunc quant4_intra_c;
quant_intraFunc dequant4_intra_c;

quant_intraFunc quant_intra_mmx;
quant_intraFunc dequant_intra_mmx;

// inter_quant
typedef uint32_t (quant_interFunc)(int16_t *coeff,
				const int16_t *data,
				const uint32_t quant);

typedef quant_interFunc* quant_interFuncPtr;	

extern quant_interFuncPtr quant_inter;
extern quant_interFuncPtr quant4_inter;

quant_interFunc quant_inter_c;
quant_interFunc quant4_inter_c;
quant_interFunc quant_inter_mmx;

//inter_dequant
typedef void (dequant_interFunc)(int16_t *coeff,
				const int16_t *data,
				const uint32_t quant);

typedef dequant_interFunc* dequant_interFuncPtr;	

extern dequant_interFuncPtr dequant_inter;

extern dequant_interFuncPtr dequant4_inter;

dequant_interFunc dequant_inter_c;
dequant_interFunc dequant4_inter_c;
dequant_interFunc dequant_inter_mmx;



/* --- transfers --- */

typedef void (transfer_8to16copyFunc)(int16_t * const dst,
					const uint8_t * const src,
					uint32_t stride);

typedef transfer_8to16copyFunc* transfer_8to16copyFuncPtr;	

extern transfer_8to16copyFuncPtr transfer_8to16copy;
transfer_8to16copyFunc transfer_8to16copy_c;
transfer_8to16copyFunc transfer_8to16copy_mmx;


typedef void (transfer_16to8Func)(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);

typedef transfer_16to8Func* transfer_16to8FuncPtr;	

extern transfer_16to8FuncPtr transfer_16to8copy;
extern transfer_16to8FuncPtr transfer_16to8add;

transfer_16to8Func transfer_16to8copy_c;
transfer_16to8Func transfer_16to8copy_mmx;

transfer_16to8Func transfer_16to8add_c;
transfer_16to8Func transfer_16to8add_mmx;


typedef void (transfer_8to8copyFunc)(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride);

typedef transfer_8to8copyFunc* transfer_8to8copyFuncPtr;	

extern transfer_8to8copyFuncPtr transfer_8to8copy;

transfer_8to8copyFunc transfer_8to8copy_c;
transfer_8to8copyFunc transfer_8to8copy_mmx;


typedef void (transfer_8to8add16Func)(uint8_t * const dst,
					const uint8_t * const src,
					const int16_t * const data,
					const uint32_t stride);

typedef transfer_8to8add16Func* transfer_8to8add16FuncPtr;	

extern transfer_8to8add16FuncPtr transfer_8to8add16;

transfer_8to8add16Func transfer_8to8add16_c;
transfer_8to8add16Func transfer_8to8add16_mmx;


/* --- compensate --- */

typedef void (compensateFunc)(int16_t * const dct,
				uint8_t * const cur,
				const uint8_t * ref,
				const uint32_t stride);

typedef compensateFunc* compensateFuncPtr;	

extern compensateFuncPtr compensate;
compensateFunc compensate_c;
compensateFunc compensate_mmx;

#endif /* _COMMON_H_ */
