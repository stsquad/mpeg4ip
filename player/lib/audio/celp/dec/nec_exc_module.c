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
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "lpc_common.h"
#include "nec_exc_proto.h"
#include "nec_abs_const.h"

#include "fix_acb_int.tbl"

void nec_lpc2par( float a[], float rc[], long m )
{
   int		j, mr, k;
   float	*tmp_fa, *b, d;

   if((tmp_fa = (float *)calloc (m, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_lpc2par \n");
      exit(1);
   }
   if((b = (float *)calloc (m, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_lpc2par \n");
      exit(1);
   }

   for (j = 0; j < m; j++) tmp_fa[j] = a[j];

   for (mr = m - 1; mr >= 0; mr--) {
      d = 1.0 - tmp_fa[mr] * tmp_fa[mr];
      rc[mr] = tmp_fa[mr];
      for (k = 0; k <= mr; k++)
	 b[k] = tmp_fa[k];
      for (k = 0; k < mr; k++)
	 tmp_fa[k] = (b[k] - b[mr] * b[mr - k - 1]) / d;
   }

  FREE ( tmp_fa );
  FREE ( b );
}

void nec_zero_filt(
		   float	x[], 		/* input */
		   float	y[], 		/* output */
		   float	alpha[],	/* input */
		   float	g_den[],	/* input */
		   float	g_num[],	/* input */
		   long		order,		/* configuration input */
		   long		len )		/* configuration input */
{
   long		i;
   float	*syn, *zero1, *zero2;

   if((syn = (float *)calloc (len, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in zero_filt \n");
      exit(1);
   }
   if((zero1 = (float *)calloc (order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in zero_filt \n");
      exit(1);
   }
   if((zero2 = (float *)calloc (order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in zero_filt \n");
      exit(1);
   }

   for ( i = 0; i < order; i++ ) zero1[i] = 0.0;
   nec_syn_filt(x, alpha, zero1, syn, order, len);
   for ( i = 0; i < order; i++ ) zero1[i] = zero2[i] = 0.0;
   nec_pw_filt(y, syn, order, g_den, g_num, zero1, zero2, len);

  FREE( syn );
  FREE( zero1 );
  FREE( zero2 );
}

void nec_pw_imprs(
		  float	y[],		/* output */
		  float	a[],		/* input */
		  long	m,		/* configuration input */
		  float	g_den[],	/* input */
		  float	g_num[],	/* input */
		  long n )		/* configuration input */
{
   long		i;
   float	*in;

   if((in = (float *)calloc (n, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in imprs2 \n");
      exit(1);
   }

   for ( i = 0; i < n; i++ ) in[i] = 0.0;
   in[0] = 1.0;
   nec_zero_filt( in, y, a, g_den, g_num, m, n );
  FREE( in );
}

void nec_comb_filt(
		   float	exc[],		/* input */
		   float	comb_exc[],	/* output */
		   long		len_sf,		/* configuration input */
		   long		I_part,		/* input */
		   long		flag )		/* input */
{
   long		sample;
   float	dum_dbl;
   static float	comb_ga[4] = { 0.0, 0.0, ((float)19661/(float)32768), ((float)26214/(float)32768) };

   if ( I_part == 0 ) {
      for (sample = 0; sample < len_sf; sample++) {
	 comb_exc[sample] = exc[sample];
      }
   }
   else {
      for (sample = 0; sample < len_sf; sample++) {
	 if ( sample - I_part >= 0 ) dum_dbl = comb_exc[sample - I_part];
	 else			     dum_dbl = 0.0;
	 dum_dbl = exc[sample] + comb_ga[flag] * dum_dbl;
	 comb_exc[sample] = dum_dbl;
      }
   }
}

void nec_syn_filt(
		  float	di[],	/* input */
		  float	a[],	/* input */
		  float	pm[],	/* input/output */
		  float	xr[],	/* output */
		  long	np,	/* configuration input */
		  long	n )	/* configuration input */
{
   long		i,j;
   float	s;

   for (j=0; j < n; j++) {
      s = (float)0.0;
      for (i=0; i < np; i++)
	 s = s - a[i] * pm[i];
      xr[j] = di[j] + s;
      for (i=2; i < np+1; i++)
	 pm[np-i+1] = pm[np-i];
      pm[0] = xr[j];
   }
}

void nec_pw_filt(
		 float		y[],	/* output */
		 float		x[],	/* input */
		 long		m,	/* configuration input */
		 float		gd[],	/* input */
		 float		gn[],	/* input */
		 float		pmem1[],/* input/output */
		 float		pmem2[],/* input/output */
		 long		n )	/* configuration input */
{
   long		i, j;
   float	s, s2;

   for (j = 0; j < n; j++) {
      s = x[j];
      /* calculation of numerator */
      for (i = 0; i < m; i++) {
         s = s + gn[i] * pmem1[i];
      }
      s2 = s;
      /* calculation of denominator */
      for (i = 0; i < m; i++) {
         s2 = s2 - gd[i] * pmem2[i];
      }
      y[j] = s2;
      for (i = 2; i < m+1; i++) {
	 pmem1[m-i+1] = pmem1[m-i];
	 pmem2[m-i+1] = pmem2[m-i];
      }
      pmem1[0] = x[j];
      pmem2[0] = y[j];
   }
}

long nec_acb_generation(long idx, long len_sf, float mem_ac[],
			       float exci[], float exco[],
			       float ga, long type, 
                               long SampleRateMode)
{
   long		i, k, kk, sample;
   long		F_part, I_part, F_part0, I_part0;
   float	dum_dbl;
   float	*P_FILm;
   static long	flag_cl = 0;
   static long	idx2lag_int[1<<NEC_ACB_BIT_WB], idx2lag_frac[1<<NEC_ACB_BIT_WB];
   static long pitch_max, idx_max, pitch_iftap;

   /*--- INITIALIZATION ---*/
   if (flag_cl == 0) {
      flag_cl = 1;
    if(fs8kHz==SampleRateMode) {
      pitch_max = NEC_PITCH_MAX;
      idx_max = 255;
      pitch_iftap = NEC_PITCH_IFTAP;

      for (i = 0; i <= 161; i++) {
	 idx2lag_int[i] = 17 + 2 * i / NEC_PITCH_RSLTN;
	 idx2lag_frac[i] = (2 * i) % NEC_PITCH_RSLTN;
      }
      for (i = 162; i <= 199; i++) {
	 idx2lag_int[i] = 71 + 3 * (i - 162) / NEC_PITCH_RSLTN;
	 idx2lag_frac[i] = (3 * (i - 162)) % NEC_PITCH_RSLTN;
      }
      for (i = 200; i <= 254; i++) {
	 idx2lag_int[i] = 90 + (i - 200);
	 idx2lag_frac[i] = 0;
      }
      idx2lag_int[255] = 0;
      idx2lag_frac[255] = 0;

    }else {
      pitch_max = NEC_PITCH_MAX_FRQ16;
      idx_max = 511;
      pitch_iftap = NEC_PITCH_IFTAP16;

      for (i = 0; i <= 215; i++) {
	 idx2lag_int[i] = 20 + 2 * i / NEC_PITCH_RSLTN;
	 idx2lag_frac[i] = (2 * i) % NEC_PITCH_RSLTN;
      }
      for (i = 216; i <= 397; i++) {
	 idx2lag_int[i] = 92 + 3 * (i - 216) / NEC_PITCH_RSLTN;
	 idx2lag_frac[i] = (3 * (i - 216)) % NEC_PITCH_RSLTN;
      }
      for (i = 398; i <= 510; i++) {
	 idx2lag_int[i] = 183 + (i - 398);
	 idx2lag_frac[i] = 0;
      }
      idx2lag_int[511] = 0;
      idx2lag_frac[511] = 0;

    }
   }
   if(fs8kHz==SampleRateMode) {
     P_FILm = nb_FIL;
   } else {
     P_FILm = wb_FIL;
   }

   /*--- EXCITATION GENERATION ---*/
   I_part = idx2lag_int[idx];
   F_part = idx2lag_frac[idx];
   if ( idx == idx_max ) {
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
	    for (k = -pitch_iftap; k <= pitch_iftap; k++) {
	       kk = (k+1) * NEC_PITCH_RSLTN - F_part0;
	       dum_dbl += P_FILm[abs(kk)]*mem_ac[pitch_max+pitch_iftap+1-(I_part0-i+k+1)];
	    }
	    dum_dbl = exci[sample] + ga * dum_dbl;
	    exco[sample] = dum_dbl;
	    mem_ac[pitch_max+pitch_iftap+1+sample] = dum_dbl;
	 }
      }
   } else {
      for (sample = 0; sample < len_sf; sample++) {
	 dum_dbl = 0.0;
	 for (k = -pitch_iftap; k <= pitch_iftap; k++) {
	    kk = (k+1) * NEC_PITCH_RSLTN - F_part;
	    dum_dbl += P_FILm[abs(kk)]*mem_ac[pitch_max+pitch_iftap+1-(I_part+k+1)+sample];
	 }
	 exco[sample] = dum_dbl;
	 mem_ac[pitch_max+pitch_iftap+1+sample] = exci[sample];
      }
   }
   return I_part;

}

void nec_mk_target(
		   float InputSignal[],
		   float target[],
		   long  sbfrm_size,
		   long  lpc_order,
		   float int_Qlps_coefficients[],
		   float Wden_coeff[],
		   float Wnum_coeff[],
		   float mem_past_in[],
		   float mem_past_win[],
		   float mem_past_syn[],
		   float mem_past_wsyn[]
		   )
{
   int		i;
   float	*xr, *xr1, *fk, *cur_wsp;
   float	*pmw, *pmw1, *pmw2;

   /*------ Memory Allocation ----------*/
   if((xr = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((xr1 = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((fk = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((cur_wsp = (float *)calloc (sbfrm_size, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((pmw = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((pmw1 = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }
   if((pmw2 = (float *)calloc (lpc_order, sizeof(float)))==NULL) {
      printf("\n Memory allocation error in nec_mk_target \n");
      exit(1);
   }

   nec_pw_filt(cur_wsp, InputSignal, lpc_order,
	       Wden_coeff, Wnum_coeff,
	       mem_past_in, mem_past_win, sbfrm_size);
   for ( i = 0; i < sbfrm_size; i++ ) xr1[i] = 0.0;
   for ( i = 0; i < lpc_order; i++) pmw[i] = mem_past_syn[i];
   nec_syn_filt(xr1, int_Qlps_coefficients,
		pmw, xr, lpc_order, sbfrm_size);
   for ( i = 0; i < lpc_order; i++) pmw1[i] = mem_past_syn[i];
   for ( i = 0; i < lpc_order; i++) pmw2[i] = mem_past_wsyn[i];
   nec_pw_filt(fk, xr, lpc_order,
	       Wden_coeff, Wnum_coeff, pmw1, pmw2, sbfrm_size);
   for(i = 0; i < sbfrm_size; i++){
      target[i] = cur_wsp[i] - fk[i];
   }

  FREE( xr );
  FREE( xr1 );
  FREE( fk );
  FREE( cur_wsp );
  FREE( pmw );
  FREE( pmw1 );
  FREE( pmw2 );
}
