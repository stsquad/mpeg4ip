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
Copyright (c)1997.
*/
/*
 *	MPEG-4 Audio Verification Model (LPC-ABS Core)
 *	
 *	Enhanced Excitation Gain Decoding Subroutines
 *
 *	Ver1.0	97.03.17	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_gain_4m4b1d.tbl"
#include "nec_exc_proto.h"
#include "bitstream.h"
#define NEC_ENH_GAIN_BIT	4

void nec_enh_gain_dec(
		  long	vu_flag,	/* input */
		  float	qxnorm,		/* input */
		  float	alpha[],	/* input */
		  float	comb_exc[],	/* input */
		  long	len_sf,		/* configuration input */
		  long	ga_idx,		/* input */
		  long	lpc_order,	/* configuration input */
		  long	gainbit,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec )		/* output */
{
   int		i;
   long		size_gc;
   float	renorm;
   float	vnorm2, ivnorm2;
   float	*par, parpar0;

   /* Cofiguration Parameter Check */
   if ( gainbit != NEC_ENH_GAIN_BIT ) {
      printf("\n Configuration error in nec_dec_gain \n");
      exit(1);
   }

   size_gc = 1 << gainbit;

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

   vnorm2 = 0.0;
   for (i = 0; i < len_sf; i++) {
      vnorm2 += comb_exc[i] * comb_exc[i];
   }
   if(vnorm2 == 0.0) {
      ivnorm2 = 0.0;
   }
   else {
      ivnorm2 = 1.0 / sqrt(vnorm2);
   }

   *g_ac = 1.0;
   *g_ec  = nec_egc[vu_flag][ga_idx] * renorm * ivnorm2;

   FREE( par );
}

