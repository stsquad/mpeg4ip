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
				int32_t iDcScaler,
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

quant_intraFunc quant4_intra_mmx;
quant_intraFunc dequant4_intra_mmx;

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
quant_interFunc quant4_inter_mmx;

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
dequant_interFunc dequant4_inter_mmx;



/* --- transfers --- */

// transfer8to16
typedef void (TRANSFER_8TO16COPY)(int16_t * const dst,
					const uint8_t * const src,
					uint32_t stride);
typedef TRANSFER_8TO16COPY* TRANSFER_8TO16COPY_PTR;	
extern TRANSFER_8TO16COPY_PTR transfer_8to16copy;
TRANSFER_8TO16COPY transfer_8to16copy_c;
TRANSFER_8TO16COPY transfer_8to16copy_mmx;

// transfer16to8
typedef void (TRANSFER_16TO8COPY)(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);
typedef TRANSFER_16TO8COPY* TRANSFER_16TO8COPY_PTR;	
extern TRANSFER_16TO8COPY_PTR transfer_16to8copy;
TRANSFER_16TO8COPY transfer_16to8copy_c;
TRANSFER_16TO8COPY transfer_16to8copy_mmx;

// transfer8to16sub
typedef void (TRANSFER_8TO16SUB)(int16_t * const dct,
				uint8_t * const cur,
				const uint8_t * ref,
				const uint32_t stride);
typedef TRANSFER_8TO16SUB* TRANSFER_8TO16SUB_PTR;

extern TRANSFER_8TO16SUB_PTR transfer_8to16sub;
TRANSFER_8TO16SUB transfer_8to16sub_c;
TRANSFER_8TO16SUB transfer_8to16sub_mmx;

// transfer16to8add
typedef void (TRANSFER_16TO8ADD)(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);
typedef TRANSFER_16TO8ADD* TRANSFER_16TO8ADD_PTR;	
extern TRANSFER_16TO8ADD_PTR transfer_16to8add;
TRANSFER_16TO8ADD transfer_16to8add_c;
TRANSFER_16TO8ADD transfer_16to8add_mmx;

// transfer8x8_copy
typedef void (TRANSFER8X8_COPY)(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride);
typedef TRANSFER8X8_COPY* TRANSFER8X8_COPY_PTR;	
extern TRANSFER8X8_COPY_PTR transfer8x8_copy;
TRANSFER8X8_COPY transfer8x8_copy_c;
TRANSFER8X8_COPY transfer8x8_copy_mmx;


/* --- interpolate8x8_halfpel --- */


typedef void (INTERPOLATE8X8)(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride,
					const uint32_t rounding);
typedef INTERPOLATE8X8 * INTERPOLATE8X8_PTR;	

extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_h;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_v;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv;

INTERPOLATE8X8 interpolate8x8_halfpel_h_c;
INTERPOLATE8X8 interpolate8x8_halfpel_v_c;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_c;

INTERPOLATE8X8 interpolate8x8_halfpel_h_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_v_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_mmx;

INTERPOLATE8X8 interpolate8x8_halfpel_h_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_v_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_xmm;

INTERPOLATE8X8 interpolate8x8_halfpel_h_3dn;
INTERPOLATE8X8 interpolate8x8_halfpel_v_3dn;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_3dn;


static __inline void interpolate8x8_switch(uint8_t * const cur,
				     const uint8_t * const refn,
				     const uint32_t x, const uint32_t y,
					 const int32_t dx,  const int dy,
					 const uint32_t stride,
					 const uint32_t rounding)
{
	int32_t ddx, ddy;

	switch ( ((dx&1)<<1) + (dy&1) )   // ((dx%2)?2:0)+((dy%2)?1:0)
    {
    case 0 :
		ddx = dx/2;
		ddy = dy/2;
		transfer8x8_copy(cur + y*stride + x, refn + (y+ddy)*stride + x + ddx, stride);
		break;

    case 1 :
		ddx = dx/2;
		ddy = (dy-1)/2;
		interpolate8x8_halfpel_v(cur + y*stride + x, 
							refn + (y+ddy)*stride + x + ddx, stride, rounding);
		break;

    case 2 :
		ddx = (dx-1)/2;
		ddy = dy/2;
		interpolate8x8_halfpel_h(cur + y*stride + x,
							refn + (y+ddy)*stride + x + ddx, stride, rounding);
		break;

    default :
		ddx = (dx-1)/2;
		ddy = (dy-1)/2;
		interpolate8x8_halfpel_hv(cur + y*stride + x,
							refn + (y+ddy)*stride + x + ddx, stride, rounding);
		break;
    }
}




#endif /* _COMMON_H_ */
