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
/*  06/16/97 NT */

#ifndef _celp_proto_dec_h_
#define _celp_proto_dec_h_

/* #include "libtsp.h" */	/* HP 971117 */

#ifdef __cplusplus
extern "C" {
#endif

/* decoder function prototypes */

void nb_abs_lpc_decode(
    unsigned long lpc_indices[],	/* in: LPC code indices */
    float int_Qlpc_coefficients[],	/* out: quantized & interpolated LPC*/ 
    long lpc_order,			        /* in: order of LPC */
    long n_subframes,               /* in: number of subframes */
    float *prev_Qlsp_coefficients
);

void bws_lpc_decoder(
        unsigned long    lpc_indices_16[],                               
        float   int_Qlpc_coefficients_16[],     
        long    lpc_order_8,
        long    lpc_order_16,
        long    n_subframes_16,
        float   buf_Qlsp_coefficients_16[],     
        float   prev_Qlsp_coefficients_16[]
);

void nb_abs_excitation_generation(
	unsigned long shape_indices[],          /* in: shape code indices */
	unsigned long gain_indices[],           /* in: gain code indices */
	long num_shape_cbks,           /* in: number of shape codebooks */
    long num_gain_cbks,            /* in: number of gain codebooks */
	unsigned long rms_index,                /* in: RMS code index */
	float int_Qlpc_coefficients[], /* in: interpolated LPC */
	long lpc_order,                /* in: order of LPC */
	long sbfrm_size,               /* in: subframe size */
	long n_subframes,              /* in: number of subframes */
	unsigned long signal_mode,              /* in: signal mode */
	long org_frame_bit_allocation[],   /* in: bit number for each index */
	float excitation[],            /* out: decoded excitation */
	float bws_mp_exc[],            /* out: decoded excitation */
	long *acb_delay,               /* out: adaptive code delay */
	float *adaptive_gain,          /* out: adaptive code gain */
    long dec_enhstages,
    long postfilter,
    long SampleRateMode
);

void bws_excitation_generation(
	unsigned long shape_indices[],          /* in: shape code indices */
	unsigned long gain_indices[],           /* in: gain code indices */
	long num_shape_cbks,           /* in: number of shape codebooks */
    long num_gain_cbks,            /* in: number of gain codebooks */
	unsigned long rms_index,                /* in: RMS code index */
	float int_Qlpc_coefficients[], /* in: interpolated LPC */
	long lpc_order,                /* in: order of LPC */
	long sbfrm_size,               /* in: subframe size */
	long n_subframes,              /* in: number of subframes */
	unsigned long signal_mode,              /* in: signal mode */
	long org_frame_bit_allocation[],   /* in: bit number for each index */
	float excitation[],            /* out: decoded excitation */
	float bws_mp_exc[],            /* in: decoded mp excitation */
	long acb_indx_8[],	       /* in: acb_delay */
	long *acb_delay,               /* out: adaptive code delay */
	float *adaptive_gain,           /* out: adaptive code gain */
    long postfilter
);

void nb_abs_postprocessing(
	float synth_signal[], 		/* input */
	float PP_synth_signal[],		/* output */
	float int_Qlpc_coefficients[], /* input */
	long lpc_order, 		/* configuration input */
	long sbfrm_sizes, 	/* configuration input */
	long acb_delay, 		/* input */
	float adaptive_gain /* input */
);

void wb_celp_lsp_decode(
    unsigned long lpc_indices[], 	/* in: LPC code indices */
    float int_Qlpc_coefficients[],	/* out: quantized & interpolated LPC*/ 
    long lpc_order,			        /* in: order of LPC */
    long n_subframes,               /* in: number of subframes */
    float *prev_Qlsp_coefficients
);

#ifdef __cplusplus
}
#endif

#endif 
