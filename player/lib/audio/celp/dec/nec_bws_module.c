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
 *	Excitation Other Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "nec_exc_proto.h"
#include "nec_abs_const.h"

#include "fix_acb_int.tbl"

long nec_acb_generation_16(long idx, long len_sf, float mem_ac[],
			       float exci[], float exco[],
			       float ga, long type )
{
   long		i, k, kk, sample;
   long		F_part, I_part, F_part0, I_part0;
   float	dum_dbl;
   float	*P_FILm;
   long pitch_max, pitch_limit;

   /*--- INITIALIZATION ---*/
   P_FILm = wb_FIL;

      pitch_max = NEC_PITCH_MAX_FRQ16;
      pitch_limit = NEC_PITCH_LIMIT_FRQ16;

      if ( idx == pitch_limit ) {
	 I_part = 0;
	 F_part = 0;
      } else if(idx < 2){
	 I_part = 32;
	 F_part = (2 * (idx+1)) % NEC_PITCH_RSLTN;
      }
      else if (2 <= idx && idx <= 777){
	 I_part = 32 + 2 * (idx - 2)/ NEC_PITCH_RSLTN;
	 F_part = ( 2 * (idx - 2)) % NEC_PITCH_RSLTN;
      }
      else {
	 printf("Error %ld\n", idx);
      }

   /*--- EXCITATION GENERATION ---*/
   if ( idx == pitch_limit ) {
      for (i = 0; i < len_sf; i++) exco[i] = exci[i];
      return I_part;
   }

   F_part0 = 0;
   if ( type == 0 ) {
      for (sample = 0; sample < len_sf; ) {
	 F_part0 += F_part;
	 I_part0 = I_part + (F_part0/NEC_PITCH_RSLTN);
	 F_part0 = F_part0 % NEC_PITCH_RSLTN;
	 for ( i = 0; i < I_part0 && sample < len_sf; i++, sample++ ) {
	    dum_dbl = 0.0;
	    for (k = -NEC_PITCH_IFTAP16; k <= NEC_PITCH_IFTAP16; k++) {
	       kk = (k+1) * NEC_PITCH_RSLTN - F_part0;
	       dum_dbl += P_FILm[abs(kk)]*mem_ac[pitch_max+NEC_PITCH_IFTAP16+1-(I_part0-i+k+1)];
	    }
	    dum_dbl = exci[sample] + ga * dum_dbl;
	    exco[sample] = dum_dbl;
	    mem_ac[pitch_max+NEC_PITCH_IFTAP16+1+sample] = dum_dbl;
	 }
      }
   } else {
      for (sample = 0; sample < len_sf; sample++) {
	 dum_dbl = 0.0;
	 for (k = -NEC_PITCH_IFTAP16; k <= NEC_PITCH_IFTAP16; k++) {
	    kk = (k+1) * NEC_PITCH_RSLTN - F_part;
	    dum_dbl += P_FILm[abs(kk)]*mem_ac[pitch_max+NEC_PITCH_IFTAP16+1-(I_part+k+1)+sample];
	 }
	 exco[sample] = dum_dbl;
	 mem_ac[pitch_max+NEC_PITCH_IFTAP16+1+sample] = exci[sample];
      }
   }
   return I_part;

}
