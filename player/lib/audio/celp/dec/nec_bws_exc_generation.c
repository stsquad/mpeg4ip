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
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	Excitation Generation Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "nec_abs_proto.h"
#include "nec_abs_const.h"
#include "nec_exc_proto.h"

#define NEC_LAG_IDX_RNG	8

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
)
{
   static float mem_past_exc[NEC_PITCH_MAX_MAXIMUM + NEC_PITCH_IFTAP16+1];
   static float	qxnorm[NEC_MAX_NSF];
   static long  flag_mem = 0;
   static long	c_subframe, vu_flag;

   static long op_lag_tmp[NEC_MAX_NSF];

   long		i;
   float	*acbexc, *mpexc, *excitation;
   float	g_ac, g_ec;
   float        g_mp8,*mpexc_8;
   long		lag_idx, mp_pos_idx, mp_sgn_idx, ga_idx;
   long		integer_lag;
   long		lagbit, posbit, sgnbit, gainbit;
   long		num_pulse;
   long         pitch_max;
   long         ip16,fp16;
   long         st_idx;

   pitch_max = NEC_PITCH_MAX_FRQ16;


   if (flag_mem == 0){
      for ( i = 0; i < pitch_max+NEC_PITCH_IFTAP16+1; i++ ) {
	 mem_past_exc[i] = 0.0;
      }
      c_subframe = 0;
      flag_mem = 1;
   }

   c_subframe = c_subframe % n_subframes;
   if(c_subframe ==0){
      for(i = 0; i < n_subframes; i++){
	if(acb_idx_8[i] <= 161){
	   ip16 = (17 + 2 * acb_idx_8[i] / NEC_PITCH_RSLTN) * 2;
	   fp16 = (2 * acb_idx_8[i]) % NEC_PITCH_RSLTN;
	} else if(acb_idx_8[i] <= 199){
	   ip16 = (71 + 3 * (acb_idx_8[i] - 162)/NEC_PITCH_RSLTN) * 2;
	   fp16 = (3 * (acb_idx_8[i] - 162)) % NEC_PITCH_RSLTN;
	} else if(acb_idx_8[i] <= 254){
	   ip16 = (90 + (acb_idx_8[i] - 200)) * 2;
	   fp16 = 0;
	} else{
	   ip16 = 0;
	   fp16 = 0;
	}
	if ( fp16 != 0 ) ip16++;

	if(ip16==0) op_lag_tmp[i] = NEC_PITCH_LIMIT_FRQ16;
	else        op_lag_tmp[i] = (ip16 - 32)*NEC_PITCH_RSLTN/2 + 2;
      }
   }


   lagbit =frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+0];
   posbit =frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+1];
   sgnbit =frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+2];
   gainbit=frame_bit_allocation[c_subframe*(num_shape_cbks+num_gain_cbks)+3];

   /* Frame Operation */
   if(c_subframe==0) {
      vu_flag = (long)signal_mode;
      if ( vu_flag == 0 ) {
	 nec_bws_rms_dec(qxnorm, n_subframes,
			 (float)NEC_RMS_MAX_U, (float)NEC_MU_LAW_U,
			 (long)(NEC_BIT_RMS), (long)rms_index);
      } else {
	 nec_bws_rms_dec(qxnorm, n_subframes,
			 (float)NEC_RMS_MAX_V, (float)NEC_MU_LAW_V,
			 (long)(NEC_BIT_RMS), (long)rms_index);
      }
   }
   qxnorm[c_subframe] = qxnorm[c_subframe] * (float)sqrt((float)sbfrm_size);
 

   /*------ Memory Allocation ----------*/
   if((acbexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_generation \n");
      exit(1);
   }
   if((mpexc = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_generation \n");
      exit(1);
   }
   if((excitation = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_generation \n");
      exit(1);
   }
   if((mpexc_8 = (float *)calloc(sbfrm_size,sizeof(float))) == NULL){
      printf("\n Memory allocation error in nec_abs_exc_generation \n");
      exit(1);
   }
 
   /* decode INDICES */
   lag_idx = (long)shape_indices[c_subframe*num_shape_cbks+0];
   st_idx = op_lag_tmp[c_subframe] - NEC_LAG_IDX_RNG/2;
   if(st_idx < 0) st_idx = 0;
   if((st_idx + NEC_LAG_IDX_RNG -1) >= NEC_PITCH_LIMIT_FRQ16)
      st_idx = NEC_PITCH_LIMIT_FRQ16 - NEC_LAG_IDX_RNG;
   
   if(op_lag_tmp[c_subframe]== NEC_PITCH_LIMIT_FRQ16)
      lag_idx = NEC_PITCH_LIMIT_FRQ16;
   else
      lag_idx = lag_idx + st_idx;

   mp_pos_idx = (long)shape_indices[c_subframe*num_shape_cbks+1];
   mp_sgn_idx = (long)shape_indices[c_subframe*num_shape_cbks+2];
   ga_idx = (long)gain_indices[c_subframe*num_gain_cbks+0];

   /* Adaptive Code Book Decode */
   nec_bws_acb_dec(acbexc,lag_idx,sbfrm_size,lagbit,mem_past_exc,&integer_lag);

   /* Multi-Pulse Excitation Decode */
   for ( i = 0; i < sbfrm_size/2; i++ ) {
      mpexc_8[2*i] = bws_mp_exc[i];
      mpexc_8[2*i+1] = 0.0;
   }

   num_pulse = sgnbit;
   nec_bws_mp_dec(vu_flag,&g_ac, &g_ec, &g_mp8, qxnorm[c_subframe],
		  LpcCoef,
		  integer_lag, mp_pos_idx, mp_sgn_idx,
		  mpexc, acbexc, lpc_order, sbfrm_size,
		  num_pulse, gainbit, ga_idx );

   for(i = 0; i < sbfrm_size; i++){
      excitation[i] = g_ac * acbexc[i] + g_ec * mpexc[i] + g_mp8 * mpexc_8[i];
   }

   if ( postfilter ) {
      nec_bws_pitch_enhancement(excitation,decoded_excitation, mem_past_exc,
				vu_flag, lag_idx, sbfrm_size );
   } else {
      for(i = 0; i < sbfrm_size; i++){
	 decoded_excitation[i] = excitation[i];
      }
   }

   for(i = 0; i < pitch_max + NEC_PITCH_IFTAP16+1 - sbfrm_size; i++){
      mem_past_exc[i] = mem_past_exc[i + sbfrm_size];
   }
   for(i = 0; i < sbfrm_size; i++){
      mem_past_exc[pitch_max + NEC_PITCH_IFTAP16+1 - sbfrm_size + i]
	 = excitation[i];
   }

   *adapt_gain = g_ac;
   *acb_delay = 0;

   c_subframe++;
   
  FREE( acbexc );
  FREE( mpexc );
  FREE( mpexc_8 );
  FREE( excitation );

}




