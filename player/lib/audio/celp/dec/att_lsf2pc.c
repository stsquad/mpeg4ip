/*


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
 */
#include <math.h>
#include "att_proto.h"
#define MAXORDER 20
/*----------------------------------------------------------------------------
 * lsf2pc - convert lsf to predictor coefficients
 *
 * FIR filter using the line spectral frequencies
 * can, of course, also be use to get predictor coeff
 * from the line spectral frequencies
 *----------------------------------------------------------------------------
 */
void lsf2pc(
float *pc,		/* output: predictor coeff: 1,a1,a2...*/
const float *lsf,	/* input : line-spectral freqs [0,pi] */
long order		/* input : predictor order */
)
{
   double mem[2*MAXORDER+2];
   long i;

   testbound(order, MAXORDER, "lsf2pc");
   for (i=0; i< 2*order+2; i++) mem[i] = 0.0;
   for (i=0; i< order+1; i++) pc[i] = 0.0;
   pc[0]=1.0;
   lsffir( pc, lsf, order, mem, order+1);
}
