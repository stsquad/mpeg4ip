/*
 * Pitch/ Pole/Zero/Tilt postfilter
 *


This software module was originally developed by

Peter Kroon (Bell Laboratories, Lucent Technologies)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.


 * Last modified: May 1, 1996
 *
 * Modified July 1, 1996
 *    changed a[1]->a[order] to a[0]->a[order-1]
 */
#include <stdio.h>
#include <math.h>
#include "att_proto.h"

/* according to the VM, the following parameters are fixed. However, for
 * optimal performance they should be definable for different applications
 * 
 */

#define ALPHA 0.55	/* weight factor numerator */
#define BETA  0.70	/* weight factor denominator  */
#define MU 0.25		/* weight factor high-frequency tilt */
#define AGCFAC 0.95	/* smoothing filter coefficient */
#define MAXORDER 20	/* maxorder for LPC */

void att_abs_postprocessing( 
 float *input,		/* input : signal */
 float *output,		/* output: signal */
 float *a,		/* input : lpc coefficients 1 + a1z^-1 + a2 */
 long order,		/* input : lpc order */
 long len,		/* input : frame size */
 long acb_delay,	/* input : delay (samples) for pitch postfilter  */
 float acb_gain		/* input : adaptive codebook gain */
)
{
  static long firstcall=0;

   float ax[MAXORDER+1];	/* weighted FIR coefficients */
   float bx[MAXORDER+1];	/* weighted IIR coefficients */
   static float firmem[MAXORDER];
   static float iirmem[MAXORDER];
   static float scalefil;
   static float tmem;
   static float alpha=ALPHA;
   static float beta=BETA;
   static float mu=MU;
   float scale;
   float ein;
   float eout;
   float rx;
   float xtmp;
   long i;

   float rc0, c0, c1;
   static float pre_rc0;

   if (firstcall == 0) {	/* initialization */
     for (i=0; i< order ; i++){
       firmem[i] = 0.;
       iirmem[i] = 0.;
     }
     tmem = 0.;
     scalefil=0.;
     firstcall = 1;
   } /* else { */
   
     /* postfilter for spectral envelope */
     bwx( ax, a, alpha, order);
     
	 /* Revised 07/01/96  Because a[] is chenged a[1]->a[10] to a[0]->a[9] */
	 for(i=order;i>0;i--) ax[i] = ax[i-1];
	 ax[0] = 1.;
     firfilt( output, input, ax, firmem, order, len);

     bwx( bx, a, beta, order);
	 for(i=order;i>0;i--) bx[i] = bx[i-1];
	 bx[0] = 1.;
     iirfilt( output, output, bx, iirmem, order, len);

     /* spectral tilt compensation */
     c0 = 0.0;
     for ( i = 0; i < len; i++ ) c0 += input[i]*input[i];
     c1 = 0.0;
     for ( i = 1; i < len; i++ ) c1 += input[i-1]*input[i];
     rc0 = (c0 == 0.0) ? 0.0 : c1/c0;
     rc0 = 0.75 * pre_rc0 + 0.25 * rc0;
     pre_rc0 = rc0;
     rx = mu * rc0;

	 ein = 0.0001;
     eout = 0.0001;
     for (i=0; i< len; i++){
       ein += input[i] * input[i];
       xtmp = output[i];
	   output[i] = output[i] - rx * tmem;
	   tmem = xtmp;
       eout += output[i]*output[i];
     }

     if (eout > 1.) {
       scale = sqrt( ein / eout);
     } else
       scale = 1.0;
     
     /* actual amplitude correction */
     for (i=0; i< len; i++){
       scalefil = AGCFAC * scalefil + ( 1. - AGCFAC) * scale;
       output[i] = output[i] * scalefil;
     }
}




