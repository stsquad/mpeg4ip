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
 *	Multi-Pulse Excitation Decoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "nec_abs_const.h"
#include "nec_abs_proto.h"
#include "nec_exc_proto.h"

void nec_bws_mp_dec(
		long	vu_flag,	/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float   *g_mp8,
		float	qxnorm,		/* input */
		float	alpha[],	/* input */
		long	I_part,		/* input */
		long	pos_idx,	/* input */
		long	sgn_idx,	/* input */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	ga_idx ) 	/* input */
{
   long		i, j, k;
   long		*pul_loc;
   long		*bit_pos, *num_pos, *chn_pos;
   float	*tmp_exc, *sgn;


   /*------- Set Multi-Pulse Configuration -------*/
   if((bit_pos = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((num_pos = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }
   if((chn_pos = (long *)calloc (num_pulse*len_sf, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enc_mp \n");
      exit(1);
   }


   /*---------- Multi-Pulse Decode ----------*/
   if((tmp_exc = (float *)calloc (len_sf, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_dec_mp \n");
      exit(1);
   }
   if((sgn = (float *)calloc (num_pulse, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_dec_mp \n");
      exit(1);
   }
   if((pul_loc = (long *)calloc (num_pulse, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_dec_mp \n");
      exit(1);
   }
   
   nec_mp_position(len_sf, num_pulse, bit_pos, chn_pos);
   for ( i = 0; i < num_pulse; i++ ) num_pos[i] = 1 << bit_pos[i];

   for ( i = num_pulse-1, k = 0; i >= 0; i-- ) {
     sgn[i] = 0;
      pul_loc[i] = 0;
      for ( j = 0; j < bit_pos[i]; j++ ) {
	 pul_loc[i] |= ((pos_idx>>k)&0x1)<<j;
	 k++;
      }
      sgn[i] = 1.0;
      if ( (sgn_idx&0x1) == 1 ) sgn[i] = -1.0;
      sgn_idx = sgn_idx >> 1;
      pul_loc[i] = chn_pos[i*len_sf+pul_loc[i]];
   }
   
   for(i = 0; i < len_sf; i++)
     tmp_exc[i] = 0;
   for (i = 0; i < num_pulse; i++) {
      tmp_exc[pul_loc[i]] = sgn[i];
   }
   nec_comb_filt(tmp_exc, comb_exc, len_sf, I_part, vu_flag);

   nec_bws_gain_dec( vu_flag, qxnorm, alpha, ac, comb_exc,
		len_sf, ga_idx, lpc_order, gainbit, g_ac, g_ec ,g_mp8);

  FREE( bit_pos );
  FREE( num_pos );
  FREE( chn_pos );
  FREE( pul_loc );
  FREE( tmp_exc );
  FREE( sgn );
}





