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
 * Modified June 27, 1996
 *    changed a[1]->a[order] to a[0]->a[order-1]
 */
/*----------------------------------------------------------------------------
 * bwx - weight lpc filter coefficients with gamma**n
 *----------------------------------------------------------------------------
 */
#include "att_proto.h"

void bwx(
 float *bw,		/* ouput : bw expanded coefficients */
 const float *a,	/* input : lpc coefficients */
 const float w,		/* input : gamma factor */
 const long order	/* input : LPC order */
)
{
  long i;
  float s;

  /* 06/27/96 
  s = 1.0;
  for(i=0;i<=order;i++)
  */
  s = w;
  for(i=0;i<order;i++)
    {
      bw[i] = a[i] * s;
      s = s * w;
    }
}
