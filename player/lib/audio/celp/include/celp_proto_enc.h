/*
This software module was originally developed by
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
and edited by

in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 NBC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 NBC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/

/* Function prototypes for VM3.0 */
/* Last modified: 11/07/96 by NT */
/*  06/16/97 by NT */

#ifndef _celp_proto_enc_h_
#define _celp_proto_enc_h_

/* #include "libtsp.h" */ 	/* HP 971117 */

#ifdef __cplusplus
extern "C" {
#endif

void nb_abs_lpc_quantizer (
    float lpc_coefficients[],		/* in: LPC */
    float int_Qlpc_coefficients[],	/* out: quantized & interpolated LPC */ 
    long lpc_indices[], 		/* out: LPC code indices */
    long lpc_order,			/* in: order of LPC */
    long num_lpc_indices,    /* in: number of LPC indices */
    long n_lpc_analysis,     /* in: number of LP analysis per frame */
    long n_subframes,        /* in: number of subframes */
    long *interpolation_flag,	/* out: interpolation flag */
    long signal_mode,			/* inp: signal mode */
    long frame_bit_allocation[], /* in: bit number for each index */
    long sampling_frequency,	/* in: sampling frequency */
    float *prev_Qlsp_coefficients
);

void bws_lpc_quantizer(
        float   lpc_coefficients_16[],                  
        float   int_Qlpc_coefficients_16[],     
        long    lpc_indices_16[],                               
        long    lpc_order_8,
        long    lpc_order_16,
        long    num_lpc_indices_16,
        long    n_lpc_analysis_16,
        long    n_subframes_16,
        float   buf_Qlsp_coefficients_16[],                  
        float   prev_Qlsp_coefficients_16[],
        long    frame_bit_allocation[]
);

void nb_abs_excitation_analysis (
	float PP_InputSignal[],        /* in: preprocessed input signal */
	float lpc_residual[],          /* in: LP residual signal */
	float int_Qlpc_coefficients[], /* in: interpolated LPC */
	long lpc_order,                /* in: order of LPC */
	float Wnum_coeff[],            /* in: weighting coeff.(numerator) */
	float Wden_coeff[],            /* in: weighting coeff.(denominator) */
	float first_order_lpc_par,     /* in: first order LPC */
	long lag_candidates[],         /* in: lag candidates */
	long n_lag_candidates,         /* in: number of lag candididates */
	long frame_size,               /* in: frame size */
	long sbfrm_size,               /* in: subframe size */
	long n_subframes,              /* in: number of subframes */
	long *signal_mode,             /* out: signal mode */
	long frame_bit_allocation[],   /* in: bit number for each index */
	long shape_indices[],          /* out: shape code indices */
	long gain_indices[],           /* out: gain code indices */
	long num_shape_cbks,           /* in: number of shape codebooks */
	long num_gain_cbks,            /* in: number of gain codebooks */
	long *rms_index,               /* out: RMS code index */
	float decoded_excitation[],    /* out: decoded excitation */
	long num_enhstages,
	float bws_mp_exc[],
        long SampleRateMode
);

void bws_excitation_analysis(
	float PP_InputSignal[],        /* in: preprocessed input signal */
	float int_Qlpc_coefficients[], /* in: interpolated LPC */
	long lpc_order,                /* in: order of LPC */
	float Wnum_coeff[],            /* in: weighting coeff.(numerator) */
	float Wden_coeff[],            /* in: weighting coeff.(denominator) */
	long frame_size,               /* in: frame size */
	long sbfrm_size,               /* in: subframe size */
	long n_subframes,              /* in: number of subframes */
	long signal_mode,              /* in: signal mode */
	long frame_bit_allocation[],   /* in: bit number for each index */
	long shape_indices[],          /* out: shape code indices */
	long gain_indices[],           /* out: gain code indices */
	long num_shape_cbks,           /* in: number of shape codebooks */
	long num_gain_cbks,            /* in: number of gain codebooks */
	float decoded_excitation[],    /* out: decoded excitation */
        float bws_mp_exc[],
        long  *acb_index_8,
	long  rms_index			/* in: RMS code index */
);

void wb_celp_lsp_quantizer (
    float lpc_coefficients[],		/* in: LPC */
    float int_Qlpc_coefficients[],	/* out: quantized & interpolated LPC */ 
    long lpc_indices[], 		/* out: LPC code indices */
    long lpc_order,			/* in: order of LPC */
    long num_lpc_indices,    /* in: number of LPC indices */
    long n_lpc_analysis,     /* in: number of LP analysis per frame */
    long n_subframes,        /* in: number of subframes */
    long *interpolation_flag,	/* out: interpolation flag */
    long signal_mode,			/* inp: signal mode */
    long frame_bit_allocation[], /* in: bit number for each index */
    long sampling_frequency,	/* in: sampling frequency */
    float *prev_Qlsp_coefficients
);

#ifdef __cplusplus
}
#endif

#endif 
