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
/*----------------------------------------------------------------------------
 * firfilt - fir filter
 *----------------------------------------------------------------------------
 */
#include "att_proto.h"

void firfilt(
float *output,		/* output: filtered signal */
const float *input,	/* input : signal */
const float *a,		/* input : filter coefficients */
float *mem,		/* in/out: filter memory */
long order,		/* input : filter order */
long length		/* input : number of samples to filter */
)
{
   long i;
   long j;
   float temp;			/* used to allow input=output loc */
   const float *pa;
   float *pmem;
   float *ppmem;

   for(i=0; i< length; i++){
      temp = *input;
      *output = *a * *input++;
      pa = a + order;
      pmem = mem + order - 1;
      ppmem = pmem - 1;
      for( j=order; j>1; j--){
	 *output += *pa-- * *pmem;
	 *pmem-- = *ppmem--;
      }
      *output++ += *pa * *pmem;
      *mem = temp;
   }

}
