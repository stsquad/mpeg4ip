/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
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
#ifndef nec_abs_proto_h_
#define nec_abs_proto_h_

#include "lpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void nec_mp_config(
		   long len,		/* input */
		   long tgt_bit,	/* input */
		   long *pos_bit,	/* output */
		   long *sgn_bit );	/* output */

void nec_abs_excitation_analysis (
	float InputSignal[],		/* input */
	float LpcCoef[],		/* input */
	float WnumCoef[],		/* input */
	float WdenCoef[],		/* input */
	long  shape_indices[],		/* output */
	long  gain_indices[],		/* output */
	long  *rms_index,		/* output */
	long  *signal_mode,		/* output */
	float decoded_excitation[],	/* output */
	long  lpc_order,		/* configuration input */
	long  frame_size, 		/* configuration input */
	long  sbfrm_size, 		/* configuration input */
	long  n_subframes,		/* configuration input */
	long  frame_bit_allocation[],	/* configuration input */
	long  num_shape_cbks,		/* configuration input */
	long  num_gain_cbks,		/* configuration input */
	long  n_enhstages,		/* configuration input */
	float bws_mp_exc[],
        long SampleRateMode );

void nec_bws_excitation_analysis(
	float InputSignal[],		/* input */
	float LpcCoef[],		/* input */
	float WnumCoef[],		/* input */
	float WdenCoef[],		/* input */
	long  shape_indices[],		/* output */
	long  gain_indices[],		/* output */
	float decoded_excitation[],	/* output */
	long  lpc_order,		/* configuration input */
	long  sbfrm_size, 		/* configuration input */
	long  n_subframes,		/* configuration input */
	long  frame_bit_allocation[],	/* configuration input */
	long  num_shape_cbks,		/* configuration input */
	long  num_gain_cbks,		/* configuration input */
	float bws_mp_exc[],		/* input */
	long  *acb_index_8,		/* input */
	long  rms_index,		/* input */
	long  signal_mode		/* input */
);

void nec_abs_excitation_generation(
	float LpcCoef[],		/* input */
	unsigned long  shape_indices[],	/* input */
	unsigned long  gain_indices[],	/* input */
	unsigned long  rms_index,	/* input */
	unsigned long  signal_mode,	/* input */
	float decoded_excitation[],	/* output */
	float *adapt_gain,		/* output */
	long  *acb_delay,		/* output */
	long  lpc_order,		/* configuration input */
	long  sbfrm_size, 		/* configuration input */
	long  n_subframes,		/* configuration input */
	long  frame_bit_allocation[],	/* configuration input */
	long  num_shape_cbks,		/* configuration input */
	long  num_gain_cbks,		/* configuration input */
	long  d_enhstages,		/* configuration input */
	float bws_mp_exc[],
	long postfilter,
        long SampleRateMode
);

void nec_bws_excitation_generation(
	float LpcCoef[],		/* input */
	unsigned long  shape_indices[],		/* input */
	unsigned long  gain_indices[],		/* input */
	unsigned long  rms_index,		/* input */
	unsigned long  signal_mode,		/* input */
	float decoded_excitation[],	/* output */
	float *adapt_gain,		/* output */
	long  *acb_delay,		/* output */
	long  lpc_order,		/* configuration input */
	long  sbfrm_size, 		/* configuration input */
	long  n_subframes,		/* configuration input */
	long  frame_bit_allocation[],	/* configuration input */
	long  num_shape_cbks,		/* configuration input */
	long  num_gain_cbks,		/* configuration input */
	float bws_mp_exc[],
	long acb_idx_8[],
	long postfilter
);

void nec_bws_lsp_decoder(
		     unsigned long indices[],	/* input  */
		     float qlsp8[],		/* input  */
		     float qlsp[],		/* output  */
		     long lpc_order,		/* configuration input */
		     long lpc_order_8 );	/* configuration input */

void nec_bws_lsp_quantizer(
		       float lsp[],		/* input  */
		       float qlsp8[],		/* input  */
		       float qlsp[],		/* output  */
		       long indices[],		/* output  */
		       long frame_bit_allocation[], /* configuration input */
		       long lpc_order,		/* configuration input */
		       long lpc_order_8,	/* configuration input */
		       long num_lpc_indices); 	/* configuration input */

void nec_lpf_down( float xin[], float xout[], int len );
 
#ifdef __cplusplus
}
#endif

#endif
