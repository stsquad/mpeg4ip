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
 *	Excitation Gain Decoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_gain_wb_4m4b1d7b2d_nl32.tbl"
#include "nec_exc_proto.h"

#define NEC_GAIN_A_BIT	4
#define NEC_GAIN_B_BIT	7
#define NEC_GAIN_DIM	2
#define FREE(x) { if (x != NULL) free(x); x=NULL;  }
void nec_bws_gain_dec(
		  long	vu_flag,	/* input */
		  float	qxnorm,		/* input */
		  float	alpha[],	/* input */
		  float	ac[],		/* input */
		  float	comb_exc[],	/* input */
		  long	len_sf,		/* configuration input */
		  long	ga_idx,		/* input */
		  long	lpc_order,	/* configuration input */
		  long	gainbit,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec,		/* output */
		  float	*g_mp8 )		/* output */
{
   int		i;
   long		size_gc, bit1, bit2;
   long         qga_idx, qgc_idx;
   float	renorm;
   float	vnorm1, vnorm2, ivnorm1, ivnorm2;
   float	*par, parpar0;

   /* Cofiguration Parameter Check */
   if ( gainbit != NEC_GAIN_B_BIT + NEC_GAIN_A_BIT ) {
      printf("\n Configuration error in nec_dec_gain16 \n");
      exit(1);
   }

   bit1 = NEC_GAIN_A_BIT;
   bit2 = NEC_GAIN_B_BIT;
   size_gc = 1 << bit2;

   qga_idx = ga_idx >> bit2;
   qgc_idx = ga_idx - (qga_idx << bit2);

   if((par = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_dec_gain \n");
      exit(1);
   }

   nec_lpc2par(alpha, par, lpc_order);
   parpar0 = 1.0;
   for(i = 0; i < lpc_order; i++){
      parpar0 *= (1 - par[i] * par[i]);
   }
   parpar0 = (parpar0 > 0.0) ? sqrt(parpar0) : 0.0;
   renorm = qxnorm * parpar0;

   vnorm1 = 0.0;
   for (i = 0; i < len_sf; i++) {
      vnorm1 += ac[i] * ac[i];
   }
   vnorm2 = 0.0;
   for (i = 0; i < len_sf; i++) {
      vnorm2 += comb_exc[i] * comb_exc[i];
   }

   if(vnorm1 == 0.0) {
      ivnorm1 = 0.0;
   }
   else {
      ivnorm1 = 1.0 / sqrt(vnorm1);
   }
   if(vnorm2 == 0.0) {
      ivnorm2 = 0.0;
   }
   else {
      ivnorm2 = 1.0 / sqrt(vnorm2);
   }

   *g_ac = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+0] * renorm * ivnorm1;
   *g_mp8 = nec_gc_sq[vu_flag][qga_idx];
   *g_ec  = nec_gc[vu_flag][NEC_GAIN_DIM*qgc_idx+1] * renorm * ivnorm2;

  FREE( par );
}

