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

#include <stdio.h>
#include <stdlib.h>
#include "att_proto.h"


/*----------------------------------------------------------------------------
 * testbound - test if argument a exceeds int boundary b and print text
 *----------------------------------------------------------------------------
 */
void testbound(
const long a,		/* input: value to be tested */
const long b,		/* input: boundary value */
const char *text	/* input: program name */
)
{
   if( a > b){
      fprintf(stderr,"\n%s: running out of memory: wanted %ld > %ld available\n",
	     text, a, b);
      exit(10);
   }
}



