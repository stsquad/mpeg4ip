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
 *	Multi-Pulse Excitation Configuration Subroutines
 *
 *	Ver1.0	96.12.16	T.Nomura(NEC)
 *	Ver2.0	97.03.17	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "nec_abs_const.h"
#include "nec_abs_proto.h"
#include "nec_exc_proto.h"

#define	NEC_MAX_PULSE	12
#define	NEC_MIN_PULSE	3

static long nec_pulse_bit(long len, long num, long bit[]);
static void nec_pulse_pos(long len, long num, long bit[], long pos[]);

void nec_enh_mp_position(
			 long	len,		/* input */
			 long	num[],		/* input */
			 long	idx[],		/* input */
			 long	num_enh,	/* input */
			 long	bit[],		/* output */
			 long	pos[] )		/* output */
{
   long		i, j, k, l, m, n;
   long		max_num, pul_loc, min_ctr, min_chn;
   long		*bit_pos_org, *chn_pos_org, *chn_ctr, *ctr_tmp;
   long		*bit_pos, *chn_pos;
   static long	num_org = 10;

   if((bit_pos_org = (long *)calloc (num_org, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enh_mp_position \n");
      exit(1);
   }
   if((chn_pos_org = (long *)calloc (num_org*len, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enh_mp_position \n");
      exit(1);
   }
   if((chn_ctr = (long *)calloc (num_org, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enh_mp_position \n");
      exit(1);
   }
   if((ctr_tmp = (long *)calloc (num_org, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enh_mp_position \n");
      exit(1);
   }

   nec_mp_position(len, num_org, bit_pos_org, chn_pos_org);
   for ( i = 0; i < num_org; i++ ) chn_ctr[i] = 0;

   max_num = 0;
   for ( i = 0; i <= num_enh; i++ ) {
      if ( num[i] > max_num ) max_num = num[i];
   }

   if((bit_pos = (long *)calloc (max_num, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enh_mp_position \n");
      exit(1);
   }
   if((chn_pos = (long *)calloc (max_num*len, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_enh_mp_position \n");
      exit(1);
   }
   nec_mp_position(len, num[0], bit_pos, chn_pos);

   for ( n = 0; n < num_enh; n++ ) {
      for ( i = num[n]-1, k = 0; i >= 0; i-- ) {
	 pul_loc = 0;
	 for ( j = 0; j < bit_pos[i]; j++ ) {
	    pul_loc |= ((idx[n]>>k)&0x1)<<j;
	    k++;
	 }
	 pul_loc = chn_pos[i*len+pul_loc];
	 for ( l = 0; l < num_org; l++ ) {
	    for ( m = 0; m < (1<<bit_pos_org[l]); m++ ) {
	       if ( pul_loc == chn_pos_org[l*len+m] ) {
		  chn_ctr[l]++;
		  break;
	       }
	    }
	 }
      }

      for ( i = 0; i < num_org; i++ ) ctr_tmp[i] = chn_ctr[i];
      for ( i = 0; i < num[n+1]; i++ ) {
	 min_ctr = len;
	 for ( j = 0; j < num_org; j++ ) {
	    if ( ctr_tmp[j] < min_ctr ) {
	       min_ctr = ctr_tmp[j];
	       min_chn = j;
	    }
	 }
	 ctr_tmp[min_chn] = len;
	 bit_pos[i] = bit_pos_org[min_chn];
	 for ( j = 0; j < (1<<bit_pos_org[min_chn]); j++ )
	    chn_pos[i*len+j] = chn_pos_org[min_chn*len+j];
      }
   }

   for ( i = 0; i < num[num_enh]; i++ ) {
      bit[i] = bit_pos[i];
      for ( j = 0; j < (1<<bit[i]); j++ )
	 pos[i*len+j] = chn_pos[i*len+j];
   }

  FREE( bit_pos_org );
  FREE( chn_pos_org );
  FREE( chn_ctr );
  FREE( ctr_tmp );
  FREE( bit_pos );
  FREE( chn_pos );

}

void nec_mp_config(
		   long len,		/* input */
		   long tgt_bit,	/* input */
		   long *pos_bit,	/* output */
		   long *sgn_bit )	/* output */
{
   long	num;
   long tmp[NEC_MAX_PULSE], tbit, dbit, min_dbit, opt_bit;

   if ( (len%2) != 0 ) {
      printf("\n Configuration error in nec_mp_config \n");
      exit(1);
   }

   min_dbit = 8*sizeof(long)-1;
   opt_bit = -1;
   for ( num = NEC_MIN_PULSE; num <= NEC_MAX_PULSE; num++ ) {
      tbit = nec_pulse_bit( len, num, tmp );
      if ( tbit == -1 ) continue;
      tbit += num;
      dbit = tgt_bit - tbit;
      if ( dbit < 0 ) dbit = -dbit;
      if ( dbit < min_dbit ) {
	 min_dbit = dbit;
	 opt_bit = tbit;
	 *pos_bit = tbit - num;
	 *sgn_bit = num;
      }
   }
   if ( opt_bit == -1 ) {
      printf("\n Configuration error in nec_mp_config \n");
      exit(1);
   }
}

void nec_mp_position(
			    long	len,
			    long	num,
			    long	bit[],
			    long	pos[] )
{
   long	tbit;

   tbit = nec_pulse_bit( len, num, bit );
   if ( tbit == -1 ) {
      printf("\n Configuration error in nec_mp_position \n");
      exit(1);
   }
   nec_pulse_pos( len, num, bit, pos );
}

static long nec_pulse_bit(
			  long	len,
			  long	num,
			  long	bit[] )
{
   long	i, j, k, l;
   long	xnum, ynum;
   long	*tbit, *nbit;
   long err_flg, ttl_bit;

   if ( len < 2*num ) {
      return -1;
   }

   if((tbit = (long *)calloc (len/2+1, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_pulse_bit \n");
      exit(1);
   }
   if((nbit = (long *)calloc (len/2+1, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_pulse_bit \n");
      exit(1);
   }

   xnum = len / 2;
   for ( i = 0; i < xnum; i++ ) tbit[i] = 1;
   tbit[xnum] = 0;

   err_flg = 0;
   while ( xnum > num ) {
      tbit[xnum] = 0;
      ynum = xnum;
      for ( i = 0, j = 0; i < xnum; j++ ) {
	 if (  tbit[i] == tbit[i+1] ) {
	    nbit[j] = tbit[i]+1;
	    i = i + 2;
	    ynum--;
	    if ( ynum <= num ) {
	       for ( k = j+1, l = i; k < ynum; k++ ) {
		  nbit[k] = tbit[l++];
	       }
	       break;
	    }
	 } else	{
	    nbit[j] = tbit[i];
	    i++;
	 }
      }
      if ( xnum == ynum ) {
	 err_flg = 1;
	 break;
      }
      xnum = ynum;
      for ( i = 0; i < xnum; i++ ) tbit[i] = nbit[i];
   }

   if ( err_flg == 1 ) {
     FREE(tbit);
     FREE(nbit);
      return -1;
   }
   ttl_bit = 0;
   for ( i = 0; i < num; i++ ) {
      ttl_bit += tbit[i];
      bit[i] = tbit[i];
   }
  FREE(tbit);
  FREE(nbit);
   return ttl_bit;
}

static void nec_pulse_pos(
			  long	len,
			  long	num,
			  long	bit[],
			  long	pos[] )
{
   long	i, j, k, l;
   long	*ch_num;
   long	mbit, dbit, num_p, mrg_ch, step;

   if((ch_num = (long *)calloc (len/2, sizeof(long)))==NULL) {
      printf("\n Memory allocation error in nec_pulse_pos \n");
      exit(1);
   }

   mbit = 8*sizeof(long)-1;
   for ( i = 0; i < num; i++ ) {
      if ( bit[i] < mbit ) mbit = bit[i];
   }
   num_p = 1 << mbit;
   step = len / num_p;

   for ( i = 0; i < step; i++ ) ch_num[i] = -1;
   for ( i = 0; i < num; i++ ) {
      dbit = bit[i] - mbit;
      mrg_ch = 1 << dbit;
      for ( j = 0, k = 0; k < mrg_ch; ) {
	 if ( ch_num[j] == -1 ) {
	    ch_num[j] = i;
	    k++;
	    j += (long)((float)step/mrg_ch + 0.5);
	    j = j % step;
	 } else j++;
      }
   }

   for ( i = 0; i < num; i++ ) {
      l = 0;
      for ( k = 0; k < step; k++ ) {
	 if ( i == ch_num[k] ) {
	    for ( j = 0; j < num_p; j++ ) {
	       pos[i*len+l] = k + step * j;
	       l++;
	    }
	 }
      }
   }
  FREE(ch_num);
}
