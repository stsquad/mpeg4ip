/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by
Naoya Tanaka (Matsushita Communication Ind. Co., Ltd.)
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
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 *	Ver2.0	97.03.17	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "nec_abs_proto.h"
#include "nec_abs_const.h"
#include "nec_exc_proto.h"

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
)
{
   static float mem_past_exc[NEC_PITCH_MAX_FRQ16 + NEC_PITCH_IFTAP16+1];
   static float	qxnorm[NEC_MAX_NSF];
   static long  flag_mem = 0;
   static long	c_subframe, vu_flag;
   static long pitch_max, pitch_iftap;

   long		i;
   float	*CoreExcitation, *PreStageExcitation;
   float	*acbexc, *mpexc;
   float	g_ac, g_ec, g_pc;
   long		lag_idx, mp_pos_idx, mp_sgn_idx, ga_idx;
   long		integer_lag;
   long		rmsbit, lagbit, posbit, sgnbit, gainbit;
   long		c_enh, idx_ctr, *num_pulse, *pre_indices;

   if (flag_mem == 0){
    if(fs8kHz==SampleRateMode) {
      pitch_max = NEC_PITCH_MAX;
      pitch_iftap = NEC_PITCH_IFTAP;
    }else {
      pitch_max = NEC_PITCH_MAX_FRQ16;
      pitch_iftap = NEC_PITCH_IFTAP16;
    }

      for ( i = 0; i < pitch_max+pitch_iftap+1; i++ ) {
	 mem_past_exc[i] = 0.0;
      }
      c_subframe = 0;
      flag_mem = 1;
   }
   c_subframe = c_subframe % n_subframes;

   rmsbit =frame_bit_allocation[1];
   lagbit =frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+0];
   posbit =frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+1];
   sgnbit =frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+2];
   gainbit=frame_bit_allocation[2+c_subframe*(num_shape_cbks+num_gain_cbks)+3];

   /* Frame Operation */
   if(c_subframe==0) {
      vu_flag = (long)signal_mode;
      if ( vu_flag == 0 ) {
	 nec_dec_rms(qxnorm, n_subframes,
		     (float)NEC_RMS_MAX_U, (float)NEC_MU_LAW_U,
		     rmsbit, (long)rms_index);
      } else {
	 nec_dec_rms(qxnorm, n_subframes,
		     (float)NEC_RMS_MAX_V, (float)NEC_MU_LAW_V,
		     rmsbit, (long)rms_index);
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

   /* decode INDICES */
   lag_idx = (long)shape_indices[c_subframe*num_shape_cbks+0];
   mp_pos_idx = (long)shape_indices[c_subframe*num_shape_cbks+1];
   mp_sgn_idx = (long)shape_indices[c_subframe*num_shape_cbks+2];
   ga_idx = (long)gain_indices[c_subframe*num_gain_cbks+0];

   /* Adaptive Code Book Decode */
   nec_dec_acb(acbexc,lag_idx,sbfrm_size,lagbit,mem_past_exc,
	       &integer_lag, SampleRateMode);

   /* Multi-Pulse Excitation Decode */
   nec_dec_mp(vu_flag,&g_ac, &g_ec, qxnorm[c_subframe],
	      LpcCoef,
	      integer_lag, mp_pos_idx, mp_sgn_idx,
	      mpexc, acbexc, lpc_order, sbfrm_size,
	      sgnbit, gainbit, ga_idx);

   /* Enhanced Multi-Pulse Excitation Decoding */
   if((CoreExcitation = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((PreStageExcitation=(float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_abs_exc_analysis \n");
      exit(1);
   }
   if((num_pulse = (long *)calloc (d_enhstages+1, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((pre_indices = (long *)calloc (d_enhstages, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }

   for(i = 0; i < sbfrm_size; i++){
      CoreExcitation[i] = g_ac * acbexc[i] + g_ec * mpexc[i];
      bws_mp_exc[i] = g_ec * mpexc[i];
   }

   if ( postfilter ) {
      nec_pitch_enhancement(CoreExcitation,PreStageExcitation, mem_past_exc,
			    vu_flag, lag_idx, sbfrm_size, SampleRateMode );
   } else {
      for(i = 0; i < sbfrm_size; i++){
	 PreStageExcitation[i] = CoreExcitation[i];
      }
   }

   for(i = 0; i < pitch_max + pitch_iftap+1 - sbfrm_size; i++){
      mem_past_exc[i] = mem_past_exc[i + sbfrm_size];
   }
   for(i = 0; i < sbfrm_size; i++){
      mem_past_exc[pitch_max + pitch_iftap+1 - sbfrm_size + i]
	 = CoreExcitation[i];
   }

   num_pulse[0] = sgnbit;
   for ( c_enh = 0; c_enh < d_enhstages; c_enh++ ) {
      idx_ctr = (c_enh+1)*n_subframes + c_subframe;
      num_pulse[c_enh+1] = frame_bit_allocation[2+idx_ctr*(num_shape_cbks+num_gain_cbks)+2];
      gainbit = frame_bit_allocation[2+idx_ctr*(num_shape_cbks+num_gain_cbks)+3];
      pre_indices[c_enh] = mp_pos_idx;

      mp_pos_idx = (long)shape_indices[idx_ctr*num_shape_cbks+1];
      mp_sgn_idx = (long)shape_indices[idx_ctr*num_shape_cbks+2];
      ga_idx = (long)gain_indices[idx_ctr*num_gain_cbks+0];

      nec_enh_mp_dec(vu_flag,&g_pc, &g_ec, qxnorm[c_subframe],
		     LpcCoef,
		     integer_lag, mp_pos_idx, mp_sgn_idx,
		     mpexc, lpc_order, sbfrm_size,
		     num_pulse, pre_indices, c_enh+1, gainbit, ga_idx);

      for(i = 0; i < sbfrm_size; i++){
	 PreStageExcitation[i] = g_pc*PreStageExcitation[i] + g_ec*mpexc[i];
	 bws_mp_exc[i] += g_ec * mpexc[i];
      }
   }

   for(i = 0; i < sbfrm_size; i++){
      decoded_excitation[i] = PreStageExcitation[i];
   }

   *adapt_gain = g_ac;
   *acb_delay = 0;

   c_subframe++;

   FREE( CoreExcitation );
   FREE( PreStageExcitation );
   FREE( num_pulse );
   FREE( pre_indices );
   FREE( acbexc );
   FREE( mpexc );
}
