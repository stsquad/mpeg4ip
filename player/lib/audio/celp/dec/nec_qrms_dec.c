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
 *	RMS Decoding Subroutines
 *
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_exc_proto.h"

void nec_dec_rms(
		 float	qxnorm[],	/* output */
		 long	n_subframes,	/* configuration input */
		 float	rms_max,	/* configuration input */
		 float	mu_law,		/* configuration input */
		 long	rmsbit,		/* configuration input */
		 long	rms_index		/* input */
		 )
{
   long		i;
   long		size;
   float	aa, bb, delt;
   float	nwk, qwk;
   static long	flag_rms = 0;
   static float	pwk, pqxnorm;

   if ( flag_rms == 0 ) {
      flag_rms = 1;
      pwk = 0.0;
      pqxnorm = 0.0;
   }

   size = 1 << rmsbit;
   delt = 1.0 / size;
   aa = 1.0 / log10(1.0 + mu_law);
   bb = rms_max / mu_law;

   pwk = aa * log10(1.0 + pqxnorm / bb);

   qwk = delt*(rms_index+1);
   for ( i = 0; i < n_subframes; i++ ) {
      nwk = (qwk-pwk)*(i+1)/n_subframes + pwk;
      qxnorm[i] = bb * (pow((double)10.0,(nwk/aa)) - 1.0);
   }
   pqxnorm = qxnorm[n_subframes-1];

}
