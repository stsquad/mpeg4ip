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
 *	LSP Parameters Decoding Subroutines
 *
 *	Ver1.0	97.09.08	T.Nomura(NEC)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "nec_abs_proto.h"
#include "nec_abs_const.h"
#include "nec_lspnw20.tbl"

#define	NEC_LSPPRDCT_ORDER	4
#define NEC_NUM_LSPSPLIT1	2
#define NEC_NUM_LSPSPLIT2	4
#define NEC_LSP_MINWIDTH_FRQ16	0.028

#define NEC_MAX_LSPVQ_ORDER	20

static void nec_lsp_sort( float *, long );

static float	nec_lsp_minwidth;

void nec_bws_lsp_decoder(
		     unsigned long indices[],	/* input  */
		     float qlsp8[],		/* input  */
		     float qlsp[],		/* output  */
		     long lpc_order,		/* configuration input */
		     long lpc_order_8 )		/* configuration input */
{
   long		i, j, k, sp_order;
   float	*tlsp, *vec_hat;
   float	*cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];
   static long	init_flag = 0;
   static float	blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];

   /* Predictor Memory Initialization */
   if ( init_flag == 0 ) {
      for ( i = 0; i < NEC_LSPPRDCT_ORDER; i++ ) {
	 for ( j = 0; j < lpc_order; j++ ) {
	    if ( j >= lpc_order_8 )
	       blsp[i][j]=(float)NEC_PAI/(float)(lpc_order+1)*(j+1);
	    else
	       blsp[i][j]=0.0;
	 }
      }
      init_flag = 1;
   }

   /* Memory allocation */
   if ((tlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_decoder \n");
      exit(1);
   }
   if ((vec_hat = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_decoder \n");
      exit(1);
   }

   if ( lpc_order == 20 && lpc_order_8 == 10 ) {
      cb[0] = nec_lspnw_p;
      cb[1] = nec_lspnw_1a;
      cb[2] = nec_lspnw_1b;
      cb[3] = nec_lspnw_2a;
      cb[4] = nec_lspnw_2b;
      cb[5] = nec_lspnw_2c;
      cb[6] = nec_lspnw_2d;
      nec_lsp_minwidth = NEC_LSP_MINWIDTH_FRQ16;
   } else {
      printf("Error in nec_bws_lsp_decoder\n");
      exit(1);
   }

   /*--- vector linear prediction ----*/
   for ( i = 0; i < lpc_order; i++)
     blsp[NEC_LSPPRDCT_ORDER-1][i] = 0.0;
   for ( i = 0; i < lpc_order_8; i++)
     blsp[NEC_LSPPRDCT_ORDER-1][i] = qlsp8[i];

   for ( i = 0; i < lpc_order; i++ ) {
      vec_hat[i] = 0.0;
      for ( k = 1; k < NEC_LSPPRDCT_ORDER; k++ ) {
	 vec_hat[i] += (cb[0][k*lpc_order+i] * blsp[k][i]);
      }
   }

   sp_order = lpc_order/NEC_NUM_LSPSPLIT1;
   for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
      for ( j = 0; j < sp_order; j++)
	 tlsp[i*sp_order+j] = cb[i+1][sp_order*indices[i]+j];
   }
   sp_order = lpc_order/NEC_NUM_LSPSPLIT2;
   for ( i = 0; i < NEC_NUM_LSPSPLIT2; i++ ) {
      for ( j = 0; j < sp_order; j++)
	 tlsp[i*sp_order+j] += cb[i+1+NEC_NUM_LSPSPLIT1][sp_order*indices[i+NEC_NUM_LSPSPLIT1]+j];
   }
   for ( i = 0; i < lpc_order; i++ ) qlsp[i] = vec_hat[i]+cb[0][i]*tlsp[i];
   nec_lsp_sort( qlsp, lpc_order );

   for ( i = 0; i < lpc_order; i++ ) blsp[0][i] = tlsp[i];

   /*--- store previous vector ----*/
   for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) {
      for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
   }

  FREE( tlsp );
  FREE( vec_hat );

}

void nec_lsp_sort( float x[], long order )
{
   long		i, j;
   float	tmp;

   for ( i = 0; i < order; i++ ) {
      if ( x[i] < 0.0 || x[i] > (float)NEC_PAI ) {
	 x[i] = 0.05 + (float)NEC_PAI * (float)i / (float)order;
      }
   }

   for ( i = (order-1); i > 0; i-- ) {
      for ( j = 0; j < i; j++ ) {
         if ( x[j] + nec_lsp_minwidth > x[j+1] ) {
            tmp = 0.5 * (x[j] + x[j+1]);
            x[j] = tmp - 0.51 * nec_lsp_minwidth;
            x[j+1] = tmp + 0.51 * nec_lsp_minwidth;
         }
      }
   }
}
