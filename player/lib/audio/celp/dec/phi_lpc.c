/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands) in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/
/*======================================================================*/
/*                                                                      */
/*      SOURCE_FILE:    PHI_LPC.C                                       */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Linear Prediction Subroutines                   */
/*                                                                      */
/*======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "phi_cons.h"  
#include "phi_priv.h"
#include "phi_lpc.h"
#include "phi_lpcq.h"
#include "phi_lsfr.h"
#include "nec_abs_proto.h"
#include "pan_celp_const.h"
#include "pan_celp_proto.h"
#include "att_proto.h"
#include "nec_abs_const.h"
#include "nec_lspnw20.tbl"


#define NEC_PAI			3.141592
#define	NEC_LSPPRDCT_ORDER	4
#define NEC_NUM_LSPSPLIT1	2
#define NEC_NUM_LSPSPLIT2	4
#define NEC_QLSP_CAND		2
#define NEC_LSP_MINWIDTH_FRQ16	0.028
#define NEC_MAX_LSPVQ_ORDER	20


#define DEBUG_YM        /* flag for halting LPC analysis : '98/11/27 */

/*======================================================================*/
/*     L O C A L    F U N C T I O N S     P R O T O T Y P E S           */ 
/*======================================================================*/

/*======================================================================*/
/*   Modified Panasonic/NEC functions                                   */
/*======================================================================*/

static void mod_nec_lsp_sort( float x[], long order , PHI_PRIV_TYPE *PHI_Priv);

/*======================================================================*/

static void mod_nec_psvq( float *, float *, float *,
		     long, long, float *, long *, long );

/*======================================================================*/

static void mod_nb_abs_lsp_quantizer (
    float current_lsp[],	    	/* in: current LSP to be quantized */
    float previous_Qlsp[],          /* In: previous Quantised LSP */
    float current_Qlsp[],	        /* out: quantized LSP */ 
    long lpc_indices[], 		    /* out: LPC code indices */
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices,           /* in: number of LPC indices */
    long n_lpc_analysis             /* in: number of LP analysis per frame */
);

/*======================================================================*/

static void mod_nec_bws_lsp_quantizer(
		       float lsp[],		/* input  */
		       float qlsp8[],		/* input  */
		       float qlsp[],		/* output  */
		       long indices[],		/* output  */
		       long lpc_order,		/* configuration input */
		       long lpc_order_8,	/* configuration input */
		       long num_lpc_indices, 	/* configuration input */
		       float blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER],
		       PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
);

/*======================================================================*/

static void mod_nb_abs_lsp_decode(
    unsigned long lpc_indices[], 		    /* in: LPC code indices */
    float prev_Qlsp[],    		    /* in: previous LSP vector */
    float current_Qlsp[],	        /* out: quantized LSP vector */ 
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices            /* in: number of LPC indices */
);		       

/*======================================================================*/

static void mod_nec_bws_lsp_decoder(
		     unsigned long indices[],		/* input  */
		     float qlsp8[],		/* input  */
		     float qlsp[],		/* output  */
		     long lpc_order,		/* configuration input */
		     long lpc_order_8,		/* configuration input */
		     long num_lpc_indices,  	/* configuration input */
		     float blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER],
		     PHI_PRIV_TYPE *PHI_Priv	/* In/Out: PHI private data (instance context) */
);

/*======================================================================*/	

static void mod_wb_celp_lsp_quantizer (
    float current_lsp[],	    	/* in: current LSP to be quantized */
    float previous_Qlsp[],          /* In: previous Quantised LSP */
    float current_Qlsp[],	        /* out: quantized LSP */ 
    long lpc_indices[], 		    /* out: LPC code indices */
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices,           /* in: number of LPC indices */
    long n_lpc_analysis             /* in: number of LP analysis per frame */
);


/*======================================================================*/

static void mod_wb_celp_lsp_decode(
    unsigned long lpc_indices[], 		    /* in: LPC code indices */
    float prev_Qlsp[],    		    /* in: previous LSP vector */
    float current_Qlsp[],	        /* out: quantized LSP vector */ 
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices            /* in: number of LPC indices */
);

/*======================================================================*/
/*   Function Prototype: PHI_Apar2Rfc                                   */
/*======================================================================*/
static void PHI_Apar2Rfc
(
int    P,               /* In:  LPC Order                               */
float ap[],             /* In:  Polynomial coefficients [0..P-1]        */
float rfc[]             /* Out: Reflection coefficients [0..P-1]        */
);

/*======================================================================*/
/*   Function Prototype: PHI_Rfc2Apar                                   */
/*======================================================================*/
static void PHI_Rfc2Apar
(
int   P,                /* In:  LPC Order                               */
float rq[],             /* In:  Reflection coefficients [0..P-1]        */
float aq[]              /* Out: a-parameters [0..P-1]                   */   
);

/*======================================================================*/
/*   Function Prototype: PHI_CalcAcf                                    */
/*======================================================================*/
static void PHI_CalcAcf
(
double sp_in[],         /* In:  Input segment [0..N-1]                  */
double acf[],           /* Out: Autocorrelation coeffs [0..P]           */
int N,                  /* In:  Number of samples in the segment        */ 
int P                   /* In:  LPC Order                               */
);

/*======================================================================*/
/*   Function Prototype: PHI_LevinsonDurbin                             */
/*======================================================================*/
static int PHI_LevinsonDurbin    /* Retun Value: 1 if unstable filter   */ 
( 
double acf[],           /* In:  Autocorrelation coeffs [0..P]           */         
double apar[],          /* Out: Polynomial coeffciients  [0..P-1]       */           
double rfc[],           /* Out: Reflection coefficients [0..P-1]        */
int P,                  /* In:  LPC Order                               */
double * E              /* Out: Residual energy at the last stage       */
);

/*======================================================================*/
/*   Function Prototype: PHI_Rfc2Lar                                    */
/*======================================================================*/
static void PHI_Rfc2Lar
(
int   P,                /* In:  LPC Order                               */
float rfc[],            /* In:  Array of reflection coeffs[0..P-1]      */
float lar[]             /* Out: Array of log area ratios[0..P-1]        */
);

/*======================================================================*/
/*   Function Prototype: PHI_Lar2Rfc                                    */
/*======================================================================*/
static int PHI_Lar2Rfc
(
int   P,                /* In:  LPC Order                               */
float lar[],            /* In:  Array of log area ratios[0..P-1]        */
float rfc[]             /* Out: Array of reflection coeffs[0..P-1]      */
);

/*======================================================================*/
/*   Function Prototype: PHI_PackLpcCodes                               */
/*======================================================================*/
static void PHI_PackLpcCodes
(
const long  lpc_order,        /*In:  Order of LPC                       */
const long  rfc_code[],       /*In:  Array of Refl. Coeffs[0..order-1]  */
const long  pack_array_size,  /*In:  Array size after packing           */
      long  packed_code[]     /*Out: Packed Array[0..pack_array-size-1] */
);

/*======================================================================*/
/*   Function Prototype: PHI_UnpackLpcCodes                             */
/*======================================================================*/
static void PHI_UnpackLpcCodes
(
const long  lpc_order,        /*In:  Order of LPC                       */      
      long  rfc_code[],       /*Out: Array of Refl. Coeffs[0..order-1]  */
const long  pack_array_size,  /*In:  Array size after packing           */
const long  packed_code[]     /*In:  Packed Array[0..pack_array-size-1] */
);

/*======================================================================*/
/*   Function Prototype: PHI_lpc_analysis                               */
/*======================================================================*/
void PHI_lpc_analysis
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
float  HamWin[],                /* In:  Hamming Window                  */
long  window_offset,            /* In:  offset for window w.r.t curr. fr*/
long  window_size,              /* In:  LPC Analysis-Window Size        */
float gamma_be[],               /* In:  Bandwidth expansion coeffs.     */
long  lpc_order                 /* In:  Order of LPC                    */
);

/*======================================================================*/
/*   Function Definition:PHI_InitLpcAnalysisEncoder                     */
/*======================================================================*/
void PHI_InitLpcAnalysisEncoder
(
long  win_size[],               /* In:  LPC Analysis-Window Size        */
long  n_lpc_analysis,           /* In:  Numberof LPC Analysis Frame     */
long  order,                    /* In:  Order of LPC                    */
long  order_8,                  /* In:  Order of LPC                    */
float gamma_be,                 /* In:  Bandwidth Expansion Coefficient */
long  bit_rate,                 /* In:  Bit Rate                        */
long  sampling_frequency,       /* In:  Sampling Frequency              */
long  SampleRateMode,           /* In:  SampleRateMode                  */ 
long  frame_size,               /* In:  Frame Size                      */
long  num_lpc_indices,          /* In:  Number of LPC indices           */  
long  n_subframes,              /* In:  Number of subframes             */
long  num_shape_cbks,           /* In:  Number of Shape Codebooks       */
long  num_gain_cbks,            /* In:  Number of Gain Codebooks        */
long  frame_bit_allocation[],   /* In:  Frame bit allocation            */
long num_indices,
long QuantizationMode,
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
    int     x;
    int i,j;
    /* -----------------------------------------------------------------*/
    /* Create Arrays for Hamming Window and gamma array                 */
    /* -----------------------------------------------------------------*/
    if
    (
    (( PHI_Priv->PHI_mem_i = (float *)malloc((unsigned int)order * sizeof(float))) == NULL ) ||
    (( PHI_Priv->PHI_current_lar = (float *)malloc((unsigned int)order * sizeof(float))) == NULL )||
    (( PHI_Priv->PHI_prev_lar = (float *)malloc((unsigned int)order * sizeof(float))) == NULL )||
    (( PHI_Priv->PHI_prev_indices = (long *)malloc((unsigned int)order * sizeof(long))) == NULL )
   )
    {
		printf("MALLOC FAILURE in Routine InitLpcAnalysis \n");
		exit(1);
    }
    
    /* -----------------------------------------------------------------*/
    /* Create Array for Quantiser BLSP                                  */
    /* -----------------------------------------------------------------*/
    for ( i = 0; i < NEC_LSPPRDCT_ORDER; i++ ) 
    {
       for ( j = 0; j < (int)order; j++ ) 
       {
	       if ( j >= (int)(order/2) )
	           PHI_Priv->blsp_previous[i][j]=(float)NEC_PAI/(float)(order+1)*(j+1);
	      else
	           PHI_Priv->blsp_previous[i][j]=(float)0.0;
        }
    }
    
    
    /* -----------------------------------------------------------------*/
    /* Create Array for Quantiser (narrowband)                          */
    /* -----------------------------------------------------------------*/
    if((PHI_Priv->previous_q_lsf_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
       *(PHI_Priv->previous_q_lsf_8+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->current_q_lsf_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
      printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
         *(PHI_Priv->current_q_lsf_8+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->next_q_lsf_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
     printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
        *(PHI_Priv->next_q_lsf_8+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->previous_uq_lsf_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
      printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
        *(PHI_Priv->previous_uq_lsf_8+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->current_uq_lsf_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
      printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
        *(PHI_Priv->current_uq_lsf_8+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->next_uq_lsf_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
      printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
     *(PHI_Priv->next_uq_lsf_8+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->previous_q_lsf_int_8=(float *)calloc(order_8, sizeof(float)))==NULL) {
      printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
        *(PHI_Priv->previous_q_lsf_int_8+i) = ((i+1)/(float)((order_8)+1)) * PAN_PI;

    /* -----------------------------------------------------------------*/
    /* Create Array for Quantiser (wideband)                            */
    /* -----------------------------------------------------------------*/
     if((PHI_Priv->previous_q_lsf_int_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
       *(PHI_Priv->previous_q_lsf_int_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;


     if((PHI_Priv->previous_q_lsf_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
         *(PHI_Priv->previous_q_lsf_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

     if((PHI_Priv->current_q_lsf_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
        *(PHI_Priv->current_q_lsf_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

    if((PHI_Priv->next_q_lsf_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
        *(PHI_Priv->next_q_lsf_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

    if((PHI_Priv->previous_uq_lsf_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
        *(PHI_Priv->previous_uq_lsf_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

    if((PHI_Priv->current_uq_lsf_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
      *(PHI_Priv->current_uq_lsf_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

    if((PHI_Priv->next_uq_lsf_16=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
        *(PHI_Priv->next_uq_lsf_16+i) = ((i+1)/(float)((order)+1)) * PAN_PI;
    
    /* -----------------------------------------------------------------*/       
    /* Initialise arrays                                                */
    /* -----------------------------------------------------------------*/
    for(x=0; x < (int)order; x++)
    {
	PHI_Priv->PHI_mem_i[x] = PHI_Priv->PHI_prev_lar[x] = PHI_Priv->PHI_current_lar[x] = (float)0.0;
	PHI_Priv->PHI_prev_indices[x] = (long)(21 - PHI_tbl_rfc_range[0][x]);
    }
    /* -----------------------------------------------------------------*/       
    /* Initialise variables for dynamic threshold                       */
    /* -----------------------------------------------------------------*/
    {         
	long bits_frame;
	float per_lpc;
	long k;

	/* ------------------------------------------------------------*/
	/* Determine length of LPC frames and no LPC frames            */
	/* ------------------------------------------------------------*/

	PHI_Priv->PHI_MAX_BITS = 0;
	
	for (k = 0; k < (2 + num_lpc_indices + n_subframes*(num_shape_cbks+num_gain_cbks)); k++)
	{
		PHI_Priv->PHI_MAX_BITS += frame_bit_allocation[k];
	}
	PHI_Priv->PHI_MIN_BITS = PHI_Priv->PHI_MAX_BITS;
	for (k = 0; k < num_lpc_indices; k++)
	{
		PHI_Priv->PHI_MIN_BITS -= frame_bit_allocation[k+2];
	}

	PHI_Priv->PHI_FR = ((float)frame_size / (float)sampling_frequency);

	/*-------------------------------------------------------------*/
        /* How many bits per frame are needed to achieve fixed bit rate*/
        /* equal to what user has input                                */
        /*-------------------------------------------------------------*/
        
	bits_frame = (long)((float)bit_rate * PHI_Priv->PHI_FR + 0.5F);
	
	/* ------------------------------------------------------------*/       
	/*   Store desired bit rate                                    */
	/* ------------------------------------------------------------*/       
	
	PHI_Priv->PHI_desired_bit_rate = (long) ((float) bits_frame / PHI_Priv->PHI_FR + 0.5F);
	PHI_Priv->PHI_actual_bits = 0;

    	 
         /*-------------------------------------------------------------*/
         /* Needed %-lpc to get the desired bit rate                    */
         /*-------------------------------------------------------------*/
         per_lpc = ((float)(bits_frame-PHI_Priv->PHI_MIN_BITS))/((float)(PHI_Priv->PHI_MAX_BITS-PHI_Priv->PHI_MIN_BITS));

         /*-------------------------------------------------------------*/
         /*  Choose initial value for distance threshold                */
         /*-------------------------------------------------------------*/
         if (per_lpc < 0.51F)
         {
              PHI_Priv->PHI_dyn_lpc_thresh = 1.0F;
         }
         else
         if (per_lpc < 0.6F)
         {
              PHI_Priv->PHI_dyn_lpc_thresh = 0.25F;   
         }
         else
         if (per_lpc < 0.75F)
         {
               PHI_Priv->PHI_dyn_lpc_thresh = 0.15F;   
         }
         else
         if (per_lpc < 0.85F)
         {
              PHI_Priv->PHI_dyn_lpc_thresh = 0.1F;   
         }
         else
         if (per_lpc < 0.97F)
         {
             PHI_Priv->PHI_dyn_lpc_thresh = 0.05F;   
         }
         else
         {
              PHI_Priv->PHI_dyn_lpc_thresh = 0.0F;   
         }
         
         
         /*-------------------------------------------------------------*/
         /*  Compute number of frames per second                        */
         /*-------------------------------------------------------------*/
         PHI_Priv->PHI_FRAMES = (long)((float)sampling_frequency/(float)frame_size + 0.5F);

         /*-------------------------------------------------------------*/
         /*  Determine the stop threshold                               */
         /*-------------------------------------------------------------*/
        
         PHI_Priv->PHI_stop_threshold = (float)0.35;
        
    }
}


/*======================================================================*/
/*   Function Definition:PAN_InitLpcAnalysisEncoder                     */
/*     Nov. 07 96  - Added for narrowband coder                         */
/*======================================================================*/
void PAN_InitLpcAnalysisEncoder
(
long  win_size[],               /* In:  LPC Analysis-Window Size        */
long  n_lpc_analysis,           /* In:  Numberof LPC Analysis Frame     */
long  order,                    /* In:  Order of LPC                    */
float gamma_be,                 /* In:  Bandwidth Expansion Coefficient */
long  bit_rate,                 /* In:  Bit Rate                        */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
    
    /* -----------------------------------------------------------------*/
    /* Create Arrays for Hamming Window and gamma array                 */
    /* -----------------------------------------------------------------*/
    if
    (
    (( PHI_Priv->PHI_mem_i = (float *)malloc((unsigned int)order * sizeof(float))) == NULL)
    )
    {
		printf("MALLOC FAILURE in Routine InitLpcAnalysis \n");
		exit(1);
    }

}


/*======================================================================*/
/*   Function Definition:PHI_InitLpcAnalysisDecoder                     */
/*======================================================================*/
void PHI_InitLpcAnalysisDecoder
(
long  order,                   /* In:  Order of LPC                    */
long  order_8,                  /* In:  Order of LPC                    */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
    int     x;
    int     i,j ;
       
    /* -----------------------------------------------------------------*/
    /* Create Arrays for lars in the decoder                            */
    /* -----------------------------------------------------------------*/
    if
    (
    (( PHI_Priv->PHI_mem_s = (float *)malloc((unsigned int)order * sizeof(float))) == NULL ) ||
    (( PHI_Priv->PHI_dec_current_lar = (float *)malloc((unsigned int)order * sizeof(float))) == NULL )||
    (( PHI_Priv->PHI_dec_prev_lar = (float *)malloc((unsigned int)order * sizeof(float))) == NULL )
    )
    {
		printf("MALLOC FAILURE in Routine InitLpcAnalysis \n");
		exit(1);
    }
    /* -----------------------------------------------------------------*/
    /* Create Array for Quantiser BLSP                                  */
    /* -----------------------------------------------------------------*/
    for ( i = 0; i < NEC_LSPPRDCT_ORDER; i++ ) 
    {
       for ( j = 0; j < (int)order; j++ ) 
       {
	       if ( j >= (int)(order/2) )
	           PHI_Priv->blsp_dec[i][j]=(float)NEC_PAI/(float)(order+1)*(j+1);
	      else
	           PHI_Priv->blsp_dec[i][j]=(float)0.0;
       }
    }
    
    /* -----------------------------------------------------------------*/
    /* Create Array for Quantiser (narrowband)                          */
    /* -----------------------------------------------------------------*/
    if((PHI_Priv->previous_q_lsf_8_dec=(float *)calloc(order_8, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    
    for(i=0;i<order_8;i++) 
       *(PHI_Priv->previous_q_lsf_8_dec+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->current_q_lsf_8_dec=(float *)calloc(order_8, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
       *(PHI_Priv->current_q_lsf_8_dec+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    if((PHI_Priv->next_q_lsf_8_dec=(float *)calloc(order_8, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order_8;i++) 
       *(PHI_Priv->next_q_lsf_8_dec+i) = ((i+1)/(float)((order_8)+1))*PAN_PI;

    
   
    /* -----------------------------------------------------------------*/
    /* Create Array for Quantiser (wideband)                            */
    /* -----------------------------------------------------------------*/
  
    if((PHI_Priv->previous_q_lsf_16_dec=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
       *(PHI_Priv->previous_q_lsf_16_dec+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

    if((PHI_Priv->current_q_lsf_16_dec=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
       *(PHI_Priv->current_q_lsf_16_dec+i) = ((i+1)/(float)((order)+1)) * PAN_PI;

    if((PHI_Priv->next_q_lsf_16_dec=(float *)calloc(order, sizeof(float)))==NULL) {
       printf("\n memory allocation error in initialization_encoder\n");
       exit(1);
    }
    for(i=0;i<order;i++) 
       *(PHI_Priv->next_q_lsf_16_dec+i) = ((i+1)/(float)((order)+1)) * PAN_PI;
   
       
    /* -----------------------------------------------------------------*/       
    /* Initialise arrays                                                */
    /* -----------------------------------------------------------------*/
    for(x=0; x < (int)order; x++)
    {
        PHI_Priv->PHI_mem_s[x] = PHI_Priv->PHI_dec_prev_lar[x] = PHI_Priv->PHI_dec_current_lar[x] = (float)0.0;
    }    
}

/*======================================================================*/
/* Function Definition: PHI_FreeLpcAnalysisEncoder                      */
/*======================================================================*/
void PHI_FreeLpcAnalysisEncoder
(
long n_lpc_analysis,
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{

    /* -----------------------------------------------------------------*/
    /*FREE the arrays that were malloc'd                               */
    /* -----------------------------------------------------------------*/
    
  FREE(PHI_Priv->next_uq_lsf_16);
  FREE(PHI_Priv->current_uq_lsf_16);
  FREE(PHI_Priv->previous_uq_lsf_16);

  FREE(PHI_Priv->next_q_lsf_16);
  FREE(PHI_Priv->current_q_lsf_16);
  FREE(PHI_Priv->previous_q_lsf_16);

  FREE(PHI_Priv->previous_q_lsf_int_16);
  FREE(PHI_Priv->previous_q_lsf_int_8);

  FREE(PHI_Priv->next_uq_lsf_8);
  FREE(PHI_Priv->current_uq_lsf_8);
  FREE(PHI_Priv->previous_uq_lsf_8);

  FREE(PHI_Priv->next_q_lsf_8);
  FREE(PHI_Priv->current_q_lsf_8);
  FREE(PHI_Priv->previous_q_lsf_8);

  FREE(PHI_Priv->PHI_mem_i);
  FREE(PHI_Priv->PHI_prev_lar);
  FREE(PHI_Priv->PHI_prev_indices);
  FREE(PHI_Priv->PHI_current_lar);

}

/*======================================================================*/
/* Function Definition: PAN_FreeLpcAnalysisEncoder                      */
/*     Nov. 07 96  - Added for narrowband coder                         */
/*======================================================================*/
void PAN_FreeLpcAnalysisEncoder
(
long n_lpc_analysis,
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
    /* -----------------------------------------------------------------*/
    /*FREE the arrays that were malloc'd                               */
    /* -----------------------------------------------------------------*/
  FREE(PHI_Priv->PHI_mem_i);
}

/*======================================================================*/
/* Function Definition: PHI_FreeLpcAnalysisDecoder                      */
/*======================================================================*/
void PHI_FreeLpcAnalysisDecoder
(
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
    /* -----------------------------------------------------------------*/
    /*FREE the arrays that were malloc'd                               */
    /* -----------------------------------------------------------------*/
     
  FREE(PHI_Priv->next_q_lsf_16_dec);
  FREE(PHI_Priv->current_q_lsf_16_dec);
  FREE(PHI_Priv->previous_q_lsf_16_dec);

  FREE(PHI_Priv->next_q_lsf_8_dec);
  FREE(PHI_Priv->current_q_lsf_8_dec);
  FREE(PHI_Priv->previous_q_lsf_8_dec);

  FREE(PHI_Priv->PHI_mem_s);
  FREE(PHI_Priv->PHI_dec_prev_lar);
  FREE(PHI_Priv->PHI_dec_current_lar);

}

 
/*======================================================================*/
/*   Function Definition:celp_lpc_analysis                              */
/*======================================================================*/
void celp_lpc_analysis
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
float *windows[],               /* In:  Array of LPC Analysis windows   */
float gamma_be[],               /* In:  Bandwidth expansion coefficients*/
long  lpc_order,                /* In:  Order of LPC                    */
long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
)
{
    int i;
    
    
    for(i = 0; i < (int)n_lpc_analysis; i++)
    {
         PHI_lpc_analysis(PP_InputSignal,lpc_coefficients+lpc_order*(long)i, 
             first_order_lpc_par,
             frame_size, windows[i], window_offsets[i], window_sizes[i],
             gamma_be,
             lpc_order); 
    }
}

/*======================================================================*/
/*   Function Definition:PHI_lpc_analysis                               */
/*======================================================================*/
void PHI_lpc_analysis
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
float HamWin[],                 /* In:  Hamming Window                  */
long  window_offset,            /* In:  offset for window w.r.t curr. fr*/
long  window_size,              /* In:  LPC Analysis-Window Size        */
float gamma_be[],               /* In:  Bandwidth expansion coeffs.     */
long  lpc_order                 /* In:  Order of LPC                    */
)
{

    /*------------------------------------------------------------------*/
    /*    Volatile Variables                                            */
    /* -----------------------------------------------------------------*/
    double *acf;                 /* For Autocorrelation Function        */
    double *reflection_coeffs;   /* For Reflection Coefficients         */
    double *LpcAnalysisBlock;    /* For Windowed Input Signal           */
    double *apars;               /* a-parameters of double precision    */
    int k;
 
    /*------------------------------------------------------------------*/
    /* Allocate space for lpc, acf, a-parameters and rcf                */
    /*------------------------------------------------------------------*/

    if 
    (
    (( reflection_coeffs = (double *)malloc((unsigned int)lpc_order * sizeof(double))) == NULL ) ||
    (( acf = (double *)malloc((unsigned int)(lpc_order + 1) * sizeof(double))) == NULL ) ||
    (( apars = (double *)malloc((unsigned int)(lpc_order) * sizeof(double))) == NULL )  ||
    (( LpcAnalysisBlock = (double *)malloc((unsigned int)window_size * sizeof(double))) == NULL )
    )
    {
		printf("MALLOC FAILURE in Routine abs_lpc_analysis \n");
		exit(1);
    }
    
    /*------------------------------------------------------------------*/
    /* Windowing of the input signal                                    */
    /*------------------------------------------------------------------*/
    for(k = 0; k < (int)window_size; k++)
    {
        LpcAnalysisBlock[k] = (double)PP_InputSignal[k + (int)window_offset] * (double)HamWin[k];
    }    

    /*------------------------------------------------------------------*/
    /* Compute Autocorrelation                                          */
    /*------------------------------------------------------------------*/
    PHI_CalcAcf(LpcAnalysisBlock, acf, (int)window_size, (int)lpc_order);

    /*------------------------------------------------------------------*/
    /* Levinson Recursion                                               */
    /*------------------------------------------------------------------*/
    {
         double Energy = 0.0;
     
         PHI_LevinsonDurbin(acf, apars, reflection_coeffs,(int)lpc_order,&Energy);  
    }  
    
    /*------------------------------------------------------------------*/
    /* First-Order LPC Fit                                              */
    /*------------------------------------------------------------------*/
    *first_order_lpc_par = (float)reflection_coeffs[0];

    /*------------------------------------------------------------------*/
    /* Bandwidth Expansion                                              */
    /*------------------------------------------------------------------*/    
    for(k = 0; k < (int)lpc_order; k++)
    {
        lpc_coefficients[k] = (float)apars[k] * gamma_be[k];
    }    

    /*------------------------------------------------------------------*/
    /*FREE the arrays that were malloced                               */
    /*------------------------------------------------------------------*/
  FREE(LpcAnalysisBlock);
  FREE(reflection_coeffs);
  FREE(acf);
  FREE(apars);
}

/*======================================================================*/
/*   Function Definition:VQ_celp_lpc_decode                             */
/*======================================================================*/
void VQ_celp_lpc_decode
(
unsigned long  lpc_indices[],  /* In: Received Packed LPC Codes         */
float int_Qlpc_coefficients[], /* Out: Qaunt/interpolated a-pars        */
long  lpc_order,               /* In:  Order of LPC                     */
long  num_lpc_indices,         /* In:  Number of packes LPC codes       */
long  n_subframes,             /* In:  Number of subframes              */
unsigned long interpolation_flag, /* In:  Was interpolation done?          */
long  Wideband_VQ,             /* In:  Wideband VQ switch               */
PHI_PRIV_TYPE *PHI_Priv		/* In/Out: PHI private data (instance context) */
)
{
    if (lpc_order == ORDER_LPC_8)
    {   /*   8 khz  */
    	float *int_lsf;
    	int   s,k;
    	float *tmp_lpc_coefficients;

    	/*------------------------------------------------------------------*/
    	/* Allocate space for current_rfc and current lar                   */
    	/*------------------------------------------------------------------*/
    	if 
    	(
    	(( int_lsf = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
    	)
    	{
			printf("MALLOC FAILURE in Routine abs_lpc_quantizer \n");
			exit(1);
    	}

    	if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
			printf("\n Memory allocation error in abs_lpc_quantizer\n");
			exit(4);
		}

    	if (interpolation_flag)
    	{
    		/*----------------------------------------------------------*/
    		/* Compute LSP coeffs of the next frame                     */
    		/*----------------------------------------------------------*/
  		mod_nb_abs_lsp_decode(lpc_indices, PHI_Priv->previous_q_lsf_8_dec,
    	            	  PHI_Priv->next_q_lsf_8_dec, lpc_order, 5);

    		/*------------------------------------------------------------------*/
    		/* Interpolate LSP coeffs to obtain coeficients of cuurent frame    */
    		/*------------------------------------------------------------------*/
        	for(k = 0; k < (int)lpc_order; k++)
        	{
        		PHI_Priv->current_q_lsf_8_dec[k] = 
        	    	(float)0.5 * (PHI_Priv->previous_q_lsf_8_dec[k] + PHI_Priv->next_q_lsf_8_dec[k]);
        	} 
    	}
    	else
    	{
         	if (PHI_Priv->PHI_dec_prev_interpolation_flag == 0)
        	{
    			/*--------------------------------------------------------------*/
    			/* Compute LSP coeffs of the current frame                      */
    			/*--------------------------------------------------------------*/
     			mod_nb_abs_lsp_decode(lpc_indices, PHI_Priv->previous_q_lsf_8_dec,
    	            		  PHI_Priv->current_q_lsf_8_dec,
		    				  lpc_order, 5);
        	}
                else
        	{
            	/* LSPs received in the previous frame: do nothing */
            	;
        	}
    	}

    	/*------------------------------------------------------------------*/
    	/* Interpolation Procedure on LSPs                                  */
    	/*------------------------------------------------------------------*/
    	for(s = 0; s < (int)n_subframes; s++)
    	{
        	for(k = 0; k < (int)lpc_order; k++)
        	{
            	register float tmp;

             	tmp = (((float)((int)n_subframes - s - 1) * PHI_Priv->previous_q_lsf_8_dec[k]) 
            	   + ((float)(1 + s) * PHI_Priv->current_q_lsf_8_dec[k]))/(float)n_subframes;
           	int_lsf[k] = tmp;
        	}

    		/*--------------------------------------------------------------*/
    		/*Compute corresponding a-parameters                            */
    		/*--------------------------------------------------------------*/
	    	lsf2pc(tmp_lpc_coefficients, int_lsf, lpc_order);
	    	for( k = 0; k < lpc_order; k++)
	    	{
	    	  int_Qlpc_coefficients[s*(int)lpc_order + k] = -tmp_lpc_coefficients[k+1];
        	}
    	}

    	/* -----------------------------------------------------------------*/
    	/* Save memory                                                      */
    	/* -----------------------------------------------------------------*/
    	for(k=0; k < (int)lpc_order; k++)
    	{
         	PHI_Priv->previous_q_lsf_8_dec[k] = PHI_Priv->current_q_lsf_8_dec[k];
        	PHI_Priv->current_q_lsf_8_dec[k] = PHI_Priv->next_q_lsf_8_dec[k];
    	}

    	PHI_Priv->PHI_dec_prev_interpolation_flag = interpolation_flag;

    	/* -----------------------------------------------------------------*/
    	/*FREE the malloc'd arrays                                         */
    	/* -----------------------------------------------------------------*/
    	free(int_lsf);
    	free(tmp_lpc_coefficients);
    
    }
    else
    {   /*  16 khz  */
    
    	float *int_lsf;
    	int   s,k;
    	float *tmp_lpc_coefficients;

	if (Wideband_VQ == Scalable_VQ)
	{   
	    /*   16 khz Scalable VQ  */
	    
	    /*------------------------------------------------------------------*/
	    /* Allocate space for current_rfc and current lar                   */
	    /*------------------------------------------------------------------*/
	    if 
	    (
	    (( int_lsf = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
	    )
	    {
			    printf("MALLOC FAILURE in Routine abs_lpc_quantizer \n");
			    exit(1);
	    }
    
	    if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
			    printf("\n Memory allocation error in abs_lpc_quantizer\n");
			    exit(4);
		    }
    
	    if (interpolation_flag)
	    {
		    /*----------------------------------------------------------*/
		    /* Compute LSP coeffs of the next frame                     */
		    /*----------------------------------------------------------*/
		    mod_nb_abs_lsp_decode(lpc_indices, PHI_Priv->previous_q_lsf_8_dec,
			      PHI_Priv->next_q_lsf_8_dec,
					      lpc_order/2, 5);
    
		    mod_nec_bws_lsp_decoder( lpc_indices + 5, PHI_Priv->next_q_lsf_8_dec,
				    PHI_Priv->next_q_lsf_16_dec,
				    lpc_order, lpc_order/2, 6, PHI_Priv->blsp_dec, PHI_Priv);
    
		    /*------------------------------------------------------------------*/
		    /* Interpolate LSP coeffs to obtain coeficients of cuurent frame    */
		    /*------------------------------------------------------------------*/
		    for(k = 0; k < (int)lpc_order; k++)
		    {
			    PHI_Priv->current_q_lsf_16_dec[k] = 
			    (float)0.5 * (PHI_Priv->previous_q_lsf_16_dec[k] + PHI_Priv->next_q_lsf_16_dec[k]);
		    } 
	    }
	    else
	    {
		    if (PHI_Priv->PHI_dec_prev_interpolation_flag == 0)
		    {
			    /*--------------------------------------------------------------*/
			    /* Compute LSP coeffs of the current frame                      */
			    /*--------------------------------------------------------------*/
			    mod_nb_abs_lsp_decode(lpc_indices, PHI_Priv->previous_q_lsf_8_dec,
				      PHI_Priv->current_q_lsf_8_dec,
						      lpc_order/2, 5);
    
			    mod_nec_bws_lsp_decoder( lpc_indices + 5, PHI_Priv->current_q_lsf_8_dec,
					    PHI_Priv->current_q_lsf_16_dec,
					    lpc_order, lpc_order/2, 6, PHI_Priv->blsp_dec, PHI_Priv);
		    }
		    else
		    {
		    /* LSPs received in the previous frame: do nothing */
		    ;
		    }
	    }
    
	    /*------------------------------------------------------------------*/
	    /* Interpolation Procedure on LSPs                                  */
	    /*------------------------------------------------------------------*/
	    for(s = 0; s < (int)n_subframes; s++)
	    {
		    for(k = 0; k < (int)lpc_order; k++)
		    {
		    register float tmp;
    
		    tmp = (((float)((int)n_subframes - s - 1) * PHI_Priv->previous_q_lsf_16_dec[k]) 
		       + ((float)(1 + s) * PHI_Priv->current_q_lsf_16_dec[k]))/(float)n_subframes;
		    int_lsf[k] = tmp;
		    }
    
		    /*--------------------------------------------------------------*/
		    /*Compute corresponding a-parameters                            */
		    /*--------------------------------------------------------------*/
		    lsf2pc(tmp_lpc_coefficients, int_lsf, lpc_order);
		    for( k = 0; k < lpc_order; k++)
		    {
		      int_Qlpc_coefficients[s*(int)lpc_order + k] = -tmp_lpc_coefficients[k+1];
		    }
	    }
    
	    /* -----------------------------------------------------------------*/
	    /* Save memory                                                      */
	    /* -----------------------------------------------------------------*/
	    for(k=0; k < (int)lpc_order/2; k++)
	    {
		    PHI_Priv->previous_q_lsf_8_dec[k] = PHI_Priv->current_q_lsf_8_dec[k];
		    PHI_Priv->current_q_lsf_8_dec[k] = PHI_Priv->next_q_lsf_8_dec[k];
	    }
    
	    for(k=0; k < (int)lpc_order; k++)
	    {
		    PHI_Priv->previous_q_lsf_16_dec[k] = PHI_Priv->current_q_lsf_16_dec[k];
		    PHI_Priv->current_q_lsf_16_dec[k] = PHI_Priv->next_q_lsf_16_dec[k];
	    }
	    PHI_Priv->PHI_dec_prev_interpolation_flag = interpolation_flag;
    
	    /* -----------------------------------------------------------------*/
	    /*FREE the malloc'd arrays                                         */
	    /* -----------------------------------------------------------------*/
	  FREE(int_lsf);
	  FREE(tmp_lpc_coefficients);
	
	}
	else
	{    /*    16 khz Optimized VQ   */

	    float *int_lsf;
	    int   s,k;
	    float *tmp_lpc_coefficients;
    
	    /*------------------------------------------------------------------*/
	    /* Allocate space for current_rfc and current lar                   */
	    /*------------------------------------------------------------------*/
	    if 
	    (
	    (( int_lsf = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL )
	    )
	    {
			    printf("MALLOC FAILURE in Routine abs_lpc_quantizer \n");
			    exit(1);
	    }
    
	    if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
			    printf("\n Memory allocation error in abs_lpc_quantizer\n");
			    exit(4);
		    }
    
	    if (interpolation_flag)
	    {
		    /*----------------------------------------------------------*/
		    /* Compute LSP coeffs of the next frame                     */
		    /*----------------------------------------------------------*/

		    mod_wb_celp_lsp_decode(lpc_indices, PHI_Priv->previous_q_lsf_16_dec,
		                           PHI_Priv->next_q_lsf_16_dec, lpc_order, 
		                           num_lpc_indices);
    
		    /*------------------------------------------------------------------*/
		    /* Interpolate LSP coeffs to obtain coeficients of cuurent frame    */
		    /*------------------------------------------------------------------*/
		    for(k = 0; k < (int)lpc_order; k++)
		    {
			    PHI_Priv->current_q_lsf_16_dec[k] = 
			    (float)0.5 * (PHI_Priv->previous_q_lsf_16_dec[k] + PHI_Priv->next_q_lsf_16_dec[k]);
		    } 
	    }
	    else
	    {
		    if (PHI_Priv->PHI_dec_prev_interpolation_flag == 0)
		    {
			    /*--------------------------------------------------------------*/
			    /* Compute LSP coeffs of the current frame                      */
			    /*--------------------------------------------------------------*/

			    mod_wb_celp_lsp_decode(lpc_indices, 
					    PHI_Priv->previous_q_lsf_16_dec,
					    PHI_Priv->current_q_lsf_16_dec,
						      lpc_order, num_lpc_indices);
		    }
		    else
		    {
		    /* LSPs received in the previous frame: do nothing */
		    ;
		    }
	    }
    
	    /*------------------------------------------------------------------*/
	    /* Interpolation Procedure on LSPs                                  */
	    /*------------------------------------------------------------------*/

	    for(s = 0; s < (int)n_subframes; s++)
	    {
		    for(k = 0; k < (int)lpc_order; k++)
		    {
		    register float tmp;
    
		    tmp = (((float)((int)n_subframes - s - 1) * PHI_Priv->previous_q_lsf_16_dec[k]) 
		       + ((float)(1 + s) * PHI_Priv->current_q_lsf_16_dec[k]))/(float)n_subframes;
		    int_lsf[k] = tmp;
		    }
    
		    /*--------------------------------------------------------------*/
		    /*Compute corresponding a-parameters                            */
		    /*--------------------------------------------------------------*/
		    lsf2pc(tmp_lpc_coefficients, int_lsf, lpc_order);
		    for( k = 0; k < lpc_order; k++)
		    {
		      int_Qlpc_coefficients[s*(int)lpc_order + k] = -tmp_lpc_coefficients[k+1];
		    }
	    }
    
	    /* -----------------------------------------------------------------*/
	    /* Save memory                                                      */
	    /* -----------------------------------------------------------------*/
	    for(k=0; k < (int)lpc_order; k++)
	    {
		    PHI_Priv->previous_q_lsf_16_dec[k] = PHI_Priv->current_q_lsf_16_dec[k];
		    PHI_Priv->current_q_lsf_16_dec[k] = PHI_Priv->next_q_lsf_16_dec[k];
	    }
	    PHI_Priv->PHI_dec_prev_interpolation_flag = interpolation_flag;
    
	    /* -----------------------------------------------------------------*/
	    /*FREE the malloc'd arrays                                         */
	    /* -----------------------------------------------------------------*/
	  FREE (int_lsf);
	  FREE (tmp_lpc_coefficients);

	}
    
    }
}

/*======================================================================*/
/*   Function Definition:celp_lpc_quantizer using VQ                    */
/*======================================================================*/
void VQ_celp_lpc_quantizer
(
float lpc_coefficients[],      /* In:  Current unquantised a-pars       */
float lpc_coefficients_8[],    /* In:  Current unquantised a-pars(8 kHz)*/
float int_Qlpc_coefficients[], /* Out: Qaunt/interpolated a-pars        */
long  lpc_indices[],           /* Out: Codes thar are transmitted       */
long  lpc_order,               /* In:  Order of LPC                     */
long  num_lpc_indices,         /* In:  Number of packes LPC codes       */
long  n_lpc_analysis,          /* In:  Number of LPC/frame              */
long  n_subframes,             /* In:  Number of subframes              */
long  *interpolation_flag,     /* Out: Interpolation Flag               */
long  *send_lpc_flag,          /* Out: Send LPC flag                    */
long  Wideband_VQ,
PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
)
{
    if (lpc_order == ORDER_LPC_8)
    {    /*   fs = 8 khz  */
	    float *int_lsf;
	    long  *next_codes;
	    long  *current_codes;
	    float *tmp_lpc_coefficients;
	    int   i,s,k;

	    /*------------------------------------------------------------------*/
	    /* Allocate space for current_rfc and current lar                   */
	    /*------------------------------------------------------------------*/
	    if 
	    (
	    (( int_lsf = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL ) ||
	    (( next_codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL ) ||
	    (( current_codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL )
	    )
	    {
			    printf("MALLOC FAILURE in Routine abs_lpc_quantizer \n");
			    exit(1);
	    }
    
	    if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
			    printf("\n Memory allocation error in abs_lpc_quantizer\n");
			    exit(4);
		    }
    
	    /*------------------------------------------------------------------*/
	    /* Calculate Narrowband LSPs                                        */
	    /*------------------------------------------------------------------*/
	    tmp_lpc_coefficients[0] = 1.;
	    for(i=0;i<lpc_order;i++) 
		    tmp_lpc_coefficients[i+1] = 
				    -lpc_coefficients[i];
    
	    pc2lsf(PHI_Priv->next_uq_lsf_8, tmp_lpc_coefficients, lpc_order);


	    /*------------------------------------------------------------------*/
	    /* Narrowband Quantization Procedure                                */
	    /*------------------------------------------------------------------*/
	    mod_nb_abs_lsp_quantizer(PHI_Priv->next_uq_lsf_8, PHI_Priv->previous_q_lsf_8, PHI_Priv->next_q_lsf_8,
				     next_codes, lpc_order, 5,
				     n_lpc_analysis);

	    /*------------------------------------------------------------------*/
	    /* Narrowband Quantization Procedure                                */
	    /*------------------------------------------------------------------*/
	    mod_nb_abs_lsp_quantizer(PHI_Priv->current_uq_lsf_8, PHI_Priv->previous_q_lsf_8, PHI_Priv->current_q_lsf_8,
				     current_codes, lpc_order, 5,
				     n_lpc_analysis);
    
	    /*------------------------------------------------------------------*/
	    /* Determine dynamic threshold                                      */
	    /*------------------------------------------------------------------*/
	    if (PHI_Priv->PHI_frames_sent == PHI_Priv->PHI_FRAMES)
	    {
		    long actual_bit_rate = (long)(((float)PHI_Priv->PHI_actual_bits)/(((float)PHI_Priv->PHI_frames_sent)*PHI_Priv->PHI_FR));
    
		    float diff = (float)fabs((double)(PHI_Priv->PHI_desired_bit_rate - actual_bit_rate));   
		    float per_diff = diff/(float)PHI_Priv->PHI_desired_bit_rate;
    
		    if (per_diff > (float)0.005)
		    {
    
		    long coeff = (long)(per_diff * (float)10.0+(float)0.5) + 1;
    
		    if (actual_bit_rate > PHI_Priv->PHI_desired_bit_rate)
			    PHI_Priv->PHI_dyn_lpc_thresh += (float)0.01 * (float)coeff;
		    if (actual_bit_rate < PHI_Priv->PHI_desired_bit_rate)
			    PHI_Priv->PHI_dyn_lpc_thresh -= (float)0.01 * (float)coeff;
    
		    if (PHI_Priv->PHI_dyn_lpc_thresh < (float)0.0)
			    PHI_Priv->PHI_dyn_lpc_thresh = (float)0.0;
    
		    if (PHI_Priv->PHI_dyn_lpc_thresh > PHI_Priv->PHI_stop_threshold)
			    PHI_Priv->PHI_dyn_lpc_thresh = PHI_Priv->PHI_stop_threshold;
		    }
    
		    PHI_Priv->PHI_frames_sent = 0;
		    PHI_Priv->PHI_actual_bits = 0;                
	    }
    
    
	    /*------------------------------------------------------------------*/
	    /* Check if you really need to transmit an LPC set                  */
	    /*------------------------------------------------------------------*/
	    if (PHI_Priv->PHI_prev_int_flag)
	    {
		    *interpolation_flag = 0;
		    if (!PHI_Priv->PHI_prev_lpc_flag)
		    {
			*send_lpc_flag = 1;
		    }
		    else
		    {
			*send_lpc_flag = 0;
		    }
	    }
	    else
	    {
		    float d;
		    float lpc_thresh = PHI_Priv->PHI_dyn_lpc_thresh;
    
    
		    for(d = (float)0.0, k = 0; k < (int)lpc_order; k++)
		    {
			    double temp;
    
			    temp  = (double)(((float)0.5 * (PHI_Priv->previous_q_lsf_8[k] + PHI_Priv->next_q_lsf_8[k])) - PHI_Priv->current_q_lsf_8[k]);
			    d    += (float)fabs(temp);
		    }
		    d /= 2.0F;
    
		    if (d  < lpc_thresh) 
		    {
			    *interpolation_flag = 1;
			    *send_lpc_flag  = 1;
    
			    /*--------------------------------------------------------------*/
			    /* Perform Interpolation                                        */
			    /*--------------------------------------------------------------*/
			    for(k = 0; k < (int)lpc_order; k++)
			    {
				    PHI_Priv->current_q_lsf_8[k] = (float)0.5 * (PHI_Priv->previous_q_lsf_8[k] + PHI_Priv->next_q_lsf_8[k]);
			    } 
		    }
		    else
		    {
		    *interpolation_flag = 0;
		    *send_lpc_flag  = 1;
		    }
    
	    }
    
	    /*------------------------------------------------------------------*/
	    /* Make sure to copy the right indices for the bit stream           */
	    /*------------------------------------------------------------------*/
	    if (*send_lpc_flag)
	    {
		    if (*interpolation_flag)
		    {
		    /* INTERPOLATION: use next indices (= indices of next frame) */
			    for (k=0; k < (int)num_lpc_indices; k++)    
			    {
			    lpc_indices[k] = next_codes[k];
			    }
		    }
		    else
		    {
		    /* NO INTERPOLATION: use current indices (= indices of current frame) */
			    for (k=0; k < (int)num_lpc_indices; k++)    
			    {
			    lpc_indices[k] = current_codes[k];
			    }
		    }
	    }
	    else
	    {
		    int k;
    
		    for (k=0; k < (int)num_lpc_indices; k++)    
		    {
		    lpc_indices[k] = 0;
		    }
    
	    }
	    PHI_Priv->PHI_prev_lpc_flag = *send_lpc_flag;
	    PHI_Priv->PHI_prev_int_flag = *interpolation_flag;
    
	    /*------------------------------------------------------------------*/
	    /* Update the number of frames and bits that are output             */
	    /*------------------------------------------------------------------*/
	    PHI_Priv->PHI_frames_sent += 1;
	    if (*send_lpc_flag)
	    {
		PHI_Priv->PHI_actual_bits += PHI_Priv->PHI_MAX_BITS;
	    }
	    else
	    {
		PHI_Priv->PHI_actual_bits += PHI_Priv->PHI_MIN_BITS;
	    }
       
	    /*------------------------------------------------------------------*/
	    /* Interpolation Procedure on LSPs                                  */
	    /*------------------------------------------------------------------*/
    
	    for(s = 0; s < (int)n_subframes; s++)
	    {
		    for(k = 0; k < (int)lpc_order; k++)
		    {
		    register float tmp;
    
		    tmp = (((float)((int)n_subframes - s - 1) * PHI_Priv->previous_q_lsf_int_8[k]) 
		       + ((float)(1 + s) * PHI_Priv->current_q_lsf_8[k]))/(float)n_subframes;
		    int_lsf[k] = tmp;
		    }
    
		    /*--------------------------------------------------------------*/
		    /*Compute corresponding a-parameters                            */
		    /*--------------------------------------------------------------*/
		    lsf2pc(tmp_lpc_coefficients, int_lsf, lpc_order);
		    for( k = 0; k < lpc_order; k++)
		    {
		      int_Qlpc_coefficients[s*(int)lpc_order + k] = -tmp_lpc_coefficients[k+1];
		    }
	    }
    
	    /* -----------------------------------------------------------------*/
	    /* Save memory                                                      */
	    /* -----------------------------------------------------------------*/
	    for(k=0; k < (int)lpc_order; k++)
	    {
	       PHI_Priv->previous_uq_lsf_8[k] = PHI_Priv->current_uq_lsf_8[k];
	       PHI_Priv->current_uq_lsf_8[k] = PHI_Priv->next_uq_lsf_8[k];
	       PHI_Priv->previous_q_lsf_int_8[k] = PHI_Priv->current_q_lsf_8[k];
	    }
    
	    if (*interpolation_flag)
	    {
		    ;    
	    }
	    else
	    {
		    for(k=0; k < (int)lpc_order; k++)
		    {
			    PHI_Priv->previous_q_lsf_8[k] = PHI_Priv->current_q_lsf_8[k];
			    PHI_Priv->current_q_lsf_8[k] = PHI_Priv->next_q_lsf_8[k];
		    }
	    }


	    /* -----------------------------------------------------------------*/
	    /*FREE the malloc'd arrays                                         */
	    /* -----------------------------------------------------------------*/
	  FREE(int_lsf);
	  FREE(next_codes);
	  FREE(current_codes);
	  FREE(tmp_lpc_coefficients);		
    }
    else
    {
	    if (Wideband_VQ==Scalable_VQ)
	    {    /*   16 khz Scalable VQ  */
		float *int_lsf;
		long  *next_codes;
		long  *current_codes;
		float *tmp_lpc_coefficients;
		int   i, j, s,k;
		float blsp_next[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];
		float blsp_current[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER];
	
		/*------------------------------------------------------------------*/
		/* Allocate space for current_rfc and current lar                   */
		/*------------------------------------------------------------------*/
		if 
		(
		(( int_lsf = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL ) ||
		(( next_codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL ) ||
		(( current_codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL )
		)
		{
				printf("MALLOC FAILURE in Routine abs_lpc_quantizer \n");
				exit(1);
		}
	
		if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
				printf("\n Memory allocation error in abs_lpc_quantizer\n");
				exit(4);
			}
	
		/*------------------------------------------------------------------*/
		/* Copy memory for Wideband Quantizer                               */
		/*------------------------------------------------------------------*/
			for (i=0; i < NEC_LSPPRDCT_ORDER; i++)
			{
			for(j=0; j < NEC_MAX_LSPVQ_ORDER; j++)
			{
				blsp_next[i][j] = PHI_Priv->blsp_previous[i][j];
				blsp_current[i][j] = PHI_Priv->blsp_previous[i][j];
			}
		}
	
		/*------------------------------------------------------------------*/
		/* Calculate Narrowband LSPs                                        */
		/*------------------------------------------------------------------*/
		tmp_lpc_coefficients[0] = 1.;
		for(i=0;i<lpc_order/2;i++) 
			tmp_lpc_coefficients[i+1] = 
					-lpc_coefficients_8[i];
	
		pc2lsf(PHI_Priv->next_uq_lsf_8, tmp_lpc_coefficients, lpc_order/2);
	
		/*------------------------------------------------------------------*/
		/* Narrowband Quantization Procedure                                */
		/*------------------------------------------------------------------*/
		mod_nb_abs_lsp_quantizer(PHI_Priv->next_uq_lsf_8, PHI_Priv->previous_q_lsf_8, PHI_Priv->next_q_lsf_8,
					 next_codes, lpc_order/2, 5,
					 n_lpc_analysis);
	
		/*------------------------------------------------------------------*/
		/* Calculate Wideband LSPs                                          */
		/*------------------------------------------------------------------*/
		tmp_lpc_coefficients[0] = 1.;
		for(i=0;i<lpc_order;i++) 
			tmp_lpc_coefficients[i+1] = 
					-lpc_coefficients[i];
	
		pc2lsf(PHI_Priv->next_uq_lsf_16, tmp_lpc_coefficients, lpc_order);
	
		/*------------------------------------------------------------------*/
		/* Wideband Quantization Procedure                                  */
		/*------------------------------------------------------------------*/
			mod_nec_bws_lsp_quantizer(PHI_Priv->next_uq_lsf_16, PHI_Priv->next_q_lsf_8,
					  PHI_Priv->next_q_lsf_16, next_codes + 5,
					  lpc_order, lpc_order/2, num_lpc_indices, blsp_next, PHI_Priv);
	
		/*------------------------------------------------------------------*/
		/* Narrowband Quantization Procedure                                */
		/*------------------------------------------------------------------*/
		mod_nb_abs_lsp_quantizer(PHI_Priv->current_uq_lsf_8, PHI_Priv->previous_q_lsf_8, PHI_Priv->current_q_lsf_8,
					 current_codes, lpc_order/2, 5,
					 n_lpc_analysis);
	
	 
		/*------------------------------------------------------------------*/
		/* Wideband Quantization Procedure                                  */
		/*------------------------------------------------------------------*/
		mod_nec_bws_lsp_quantizer(PHI_Priv->current_uq_lsf_16, PHI_Priv->current_q_lsf_8,
					  PHI_Priv->current_q_lsf_16, current_codes + 5,
					  lpc_order, lpc_order/2, num_lpc_indices, blsp_current, PHI_Priv);
	
		
		/*------------------------------------------------------------------*/
		/* Determine dynamic threshold                                      */
		/*------------------------------------------------------------------*/
		if (PHI_Priv->PHI_frames_sent == PHI_Priv->PHI_FRAMES)
		{
			long actual_bit_rate = (long)(((float)PHI_Priv->PHI_actual_bits)/(((float)PHI_Priv->PHI_frames_sent)*PHI_Priv->PHI_FR));
	
			float diff = (float)fabs((double)(PHI_Priv->PHI_desired_bit_rate - actual_bit_rate));   
			float per_diff = diff/(float)PHI_Priv->PHI_desired_bit_rate;
	
			if (per_diff > (float)0.005)
			{
	
			long coeff = (long)(per_diff * (float)10.0+(float)0.5) + 1;
	
			if (actual_bit_rate > PHI_Priv->PHI_desired_bit_rate)
				PHI_Priv->PHI_dyn_lpc_thresh += (float)0.01 * (float)coeff;
			if (actual_bit_rate < PHI_Priv->PHI_desired_bit_rate)
				PHI_Priv->PHI_dyn_lpc_thresh -= (float)0.01 * (float)coeff;
	
			if (PHI_Priv->PHI_dyn_lpc_thresh < (float)0.0)
				PHI_Priv->PHI_dyn_lpc_thresh = (float)0.0;
	
			if (PHI_Priv->PHI_dyn_lpc_thresh > PHI_Priv->PHI_stop_threshold)
				PHI_Priv->PHI_dyn_lpc_thresh = PHI_Priv->PHI_stop_threshold;
			}
	
			PHI_Priv->PHI_frames_sent = 0;
			PHI_Priv->PHI_actual_bits = 0;                
		}
	
	
		/*------------------------------------------------------------------*/
		/* Check if you really need to transmit an LPC set                  */
		/*------------------------------------------------------------------*/
		if (PHI_Priv->PHI_prev_int_flag)
		{
			*interpolation_flag = 0;
			if (!PHI_Priv->PHI_prev_lpc_flag)
			{
				*send_lpc_flag = 1;
			}
			else
			{
				*send_lpc_flag = 0;
			}
		}
		else
		{
			float d;
			float lpc_thresh = PHI_Priv->PHI_dyn_lpc_thresh;
	
	
			for(d = (float)0.0, k = 0; k < (int)lpc_order; k++)
			{
				double temp;
	
				temp  = (double)(((float)0.5 * (PHI_Priv->previous_q_lsf_16[k] + PHI_Priv->next_q_lsf_16[k])) - PHI_Priv->current_q_lsf_16[k]);
				d    += (float)fabs(temp);
			}
			d /= 2.0F;
	
			if (d  < lpc_thresh) 
			{
				*interpolation_flag = 1;
				*send_lpc_flag  = 1;
	
				/*--------------------------------------------------------------*/
				/* Perform Interpolation                                        */
				/*--------------------------------------------------------------*/
				for(k = 0; k < (int)lpc_order; k++)
				{
					PHI_Priv->current_q_lsf_16[k] = (float)0.5 * (PHI_Priv->previous_q_lsf_16[k] + PHI_Priv->next_q_lsf_16[k]);
				} 
			}
			else
			{
			    *interpolation_flag = 0;
			    *send_lpc_flag  = 1;
			}
	
		}
	
		/*------------------------------------------------------------------*/
		/* Make sure to copy the right indices for the bit stream           */
		/*------------------------------------------------------------------*/
		if (*send_lpc_flag)
		{
			if (*interpolation_flag)
			{
			/* INTERPOLATION: use next indices (= indices of next frame) */
				for (k=0; k < (int)num_lpc_indices; k++)    
				{
					lpc_indices[k] = next_codes[k];
				}
			}
			else
			{
			/* NO INTERPOLATION: use current indices (= indices of current frame) */
				for (k=0; k < (int)num_lpc_indices; k++)    
				{
					lpc_indices[k] = current_codes[k];
				}
			}
		}
		else
		{
			int k;
	
			for (k=0; k < (int)num_lpc_indices; k++)    
			{
				lpc_indices[k] = 0;
			}
	
		}
		PHI_Priv->PHI_prev_lpc_flag = *send_lpc_flag;
		PHI_Priv->PHI_prev_int_flag = *interpolation_flag;
	
		/*------------------------------------------------------------------*/
		/* Update the number of frames and bits that are output             */
		/*------------------------------------------------------------------*/
		PHI_Priv->PHI_frames_sent += 1;
		if (*send_lpc_flag)
		{
		    PHI_Priv->PHI_actual_bits += PHI_Priv->PHI_MAX_BITS;
		}
		else
		{
		    PHI_Priv->PHI_actual_bits += PHI_Priv->PHI_MIN_BITS;
		}
		
		/*------------------------------------------------------------------*/
		/* Interpolation Procedure on LSPs                                  */
		/*------------------------------------------------------------------*/
	
		for(s = 0; s < (int)n_subframes; s++)
		{
			for(k = 0; k < (int)lpc_order; k++)
			{
			    register float tmp;
	    
			    tmp = (((float)((int)n_subframes - s - 1) * PHI_Priv->previous_q_lsf_int_16[k]) 
			       + ((float)(1 + s) * PHI_Priv->current_q_lsf_16[k]))/(float)n_subframes;
			    int_lsf[k] = tmp;
			}
	
			/*--------------------------------------------------------------*/
			/*Compute corresponding a-parameters                            */
			/*--------------------------------------------------------------*/
			lsf2pc(tmp_lpc_coefficients, int_lsf, lpc_order);
			for( k = 0; k < lpc_order; k++)
			{
			  int_Qlpc_coefficients[s*(int)lpc_order + k] = -tmp_lpc_coefficients[k+1];
			}
		}
	
		/* -----------------------------------------------------------------*/
		/* Save memory                                                      */
		/* -----------------------------------------------------------------*/
		for(k=0; k < (int)lpc_order/2; k++)
		{
			PHI_Priv->previous_uq_lsf_8[k] = PHI_Priv->current_uq_lsf_8[k];
			PHI_Priv->current_uq_lsf_8[k] = PHI_Priv->next_uq_lsf_8[k];
		}
	
		for(k=0; k < (int)lpc_order; k++)
		{
			PHI_Priv->previous_uq_lsf_16[k] = PHI_Priv->current_uq_lsf_16[k];
			PHI_Priv->current_uq_lsf_16[k] = PHI_Priv->next_uq_lsf_16[k];
		}
	
		for(k=0; k < (int)lpc_order; k++)
		{
			PHI_Priv->previous_q_lsf_int_16[k] = PHI_Priv->current_q_lsf_16[k];
		}
	
		if (*interpolation_flag)
		{
			;    
		}
		else
		{
			for(k=0; k < (int)lpc_order/2; k++)
			{
				PHI_Priv->previous_q_lsf_8[k] = PHI_Priv->current_q_lsf_8[k];
				PHI_Priv->current_q_lsf_8[k] = PHI_Priv->next_q_lsf_8[k];
			}
	
			for(k=0; k < (int)lpc_order; k++)
			{
				PHI_Priv->previous_q_lsf_16[k] = PHI_Priv->current_q_lsf_16[k];
				PHI_Priv->current_q_lsf_16[k] = PHI_Priv->next_q_lsf_16[k];
			}
	
			for (i=0; i < NEC_LSPPRDCT_ORDER; i++)
			{
			    for(j=0; j < NEC_MAX_LSPVQ_ORDER; j++)
			    {
				    PHI_Priv->blsp_previous[i][j] = blsp_current[i][j];
			    }
			}
		}
	
		/* -----------------------------------------------------------------*/
		/*FREE the malloc'd arrays                                         */
		/* -----------------------------------------------------------------*/
		free(int_lsf);
		free(next_codes);
		free(current_codes);
		free(tmp_lpc_coefficients);		
	    }
	    else
	    {   /*   16 khz Optimized VQ  */

		float *int_lsf;
		long  *next_codes;
		long  *current_codes;
		float *tmp_lpc_coefficients;
		int   i, s,k;
	
		/*------------------------------------------------------------------*/
		/* Allocate space for current_rfc and current lar                   */
		/*------------------------------------------------------------------*/
		if 
		(
		(( int_lsf = (float *)malloc((unsigned int)lpc_order * sizeof(float))) == NULL ) ||
		(( next_codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL ) ||
		(( current_codes = (long *)malloc((unsigned int)num_lpc_indices * sizeof(long))) == NULL )
		)
		{
				printf("MALLOC FAILURE in Routine abs_lpc_quantizer \n");
				exit(1);
		}
	
		if((tmp_lpc_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
				printf("\n Memory allocation error in abs_lpc_quantizer\n");
				exit(4);
		}
	
	
		/*------------------------------------------------------------------*/
		/* Calculate Wideband LSPs                                          */
		/*------------------------------------------------------------------*/
		tmp_lpc_coefficients[0] = 1.;
		for(i=0;i<lpc_order;i++) 
			tmp_lpc_coefficients[i+1] = 
					-lpc_coefficients[i];
	
		pc2lsf(PHI_Priv->next_uq_lsf_16, tmp_lpc_coefficients, lpc_order);
	
		/*------------------------------------------------------------------*/
		/* Wideband Quantization Procedure                                  */
		/*------------------------------------------------------------------*/
	
		mod_wb_celp_lsp_quantizer(PHI_Priv->next_uq_lsf_16, PHI_Priv->previous_q_lsf_16, 
					PHI_Priv->next_q_lsf_16,
					 next_codes, lpc_order, num_lpc_indices,
					 n_lpc_analysis);
	
	
		mod_wb_celp_lsp_quantizer(PHI_Priv->current_uq_lsf_16, PHI_Priv->previous_q_lsf_16, 
					PHI_Priv->current_q_lsf_16,
					 current_codes, lpc_order, num_lpc_indices,
					 n_lpc_analysis);
	
		
		/*------------------------------------------------------------------*/
		/* Determine dynamic threshold                                      */
		/*------------------------------------------------------------------*/
		if (PHI_Priv->PHI_frames_sent == PHI_Priv->PHI_FRAMES)
		{
			long actual_bit_rate = (long)(((float)PHI_Priv->PHI_actual_bits)/(((float)PHI_Priv->PHI_frames_sent)*PHI_Priv->PHI_FR));
	
			float diff = (float)fabs((double)(PHI_Priv->PHI_desired_bit_rate - actual_bit_rate));   
			float per_diff = diff/(float)PHI_Priv->PHI_desired_bit_rate;
	
			if (per_diff > (float)0.005)
			{
	
			long coeff = (long)(per_diff * (float)10.0+(float)0.5) + 1;
	
			if (actual_bit_rate > PHI_Priv->PHI_desired_bit_rate)
				PHI_Priv->PHI_dyn_lpc_thresh += (float)0.01 * (float)coeff;
			if (actual_bit_rate < PHI_Priv->PHI_desired_bit_rate)
				PHI_Priv->PHI_dyn_lpc_thresh -= (float)0.01 * (float)coeff;
	
			if (PHI_Priv->PHI_dyn_lpc_thresh < (float)0.0)
				PHI_Priv->PHI_dyn_lpc_thresh = (float)0.0;
	
			if (PHI_Priv->PHI_dyn_lpc_thresh > PHI_Priv->PHI_stop_threshold)
				PHI_Priv->PHI_dyn_lpc_thresh = PHI_Priv->PHI_stop_threshold;
			}
	
			PHI_Priv->PHI_frames_sent = 0;
			PHI_Priv->PHI_actual_bits = 0;                
		}
	
	
		/*------------------------------------------------------------------*/
		/* Check if you really need to transmit an LPC set                  */
		/*------------------------------------------------------------------*/
		if (PHI_Priv->PHI_prev_int_flag)
		{
			*interpolation_flag = 0;
			if (!PHI_Priv->PHI_prev_lpc_flag)
			{
				*send_lpc_flag = 1;
			}
			else
			{
				*send_lpc_flag = 0;
			}
		}
		else
		{
			float d;
			float lpc_thresh = PHI_Priv->PHI_dyn_lpc_thresh;
	
	
			for(d = (float)0.0, k = 0; k < (int)lpc_order; k++)
			{
				double temp;
	
				temp  = (double)(((float)0.5 * (PHI_Priv->previous_q_lsf_16[k] + PHI_Priv->next_q_lsf_16[k])) - PHI_Priv->current_q_lsf_16[k]);
				d    += (float)fabs(temp);
			}
			d /= 2.0F;
	
			if (d  < lpc_thresh) 
			{
			*interpolation_flag = 1;
			*send_lpc_flag  = 1;
	
				/*--------------------------------------------------------------*/
				/* Perform Interpolation                                        */
				/*--------------------------------------------------------------*/
				for(k = 0; k < (int)lpc_order; k++)
				{
					PHI_Priv->current_q_lsf_16[k] = (float)0.5 * (PHI_Priv->previous_q_lsf_16[k] + PHI_Priv->next_q_lsf_16[k]);
				} 
			}
			else
			{
			*interpolation_flag = 0;
			*send_lpc_flag  = 1;
			}
	
		}
	
		/*------------------------------------------------------------------*/
		/* Make sure to copy the right indices for the bit stream           */
		/*------------------------------------------------------------------*/
		if (*send_lpc_flag)
		{
			if (*interpolation_flag)
			{
			/* INTERPOLATION: use next indices (= indices of next frame) */
				for (k=0; k < (int)num_lpc_indices; k++)    
				{
				lpc_indices[k] = next_codes[k];
				}
			}
			else
			{
			/* NO INTERPOLATION: use current indices (= indices of current frame) */
				for (k=0; k < (int)num_lpc_indices; k++)    
				{
				lpc_indices[k] = current_codes[k];
				}
			}
		}
		else
		{
			int k;
	
			for (k=0; k < (int)num_lpc_indices; k++)    
			{
			lpc_indices[k] = 0;
			}
	
		}
		PHI_Priv->PHI_prev_lpc_flag = *send_lpc_flag;
		PHI_Priv->PHI_prev_int_flag = *interpolation_flag;
	
		/*------------------------------------------------------------------*/
		/* Update the number of frames and bits that are output             */
		/*------------------------------------------------------------------*/
		PHI_Priv->PHI_frames_sent += 1;
		if (*send_lpc_flag)
		{
			PHI_Priv->PHI_actual_bits += PHI_Priv->PHI_MAX_BITS;
		}
		else
		{
			PHI_Priv->PHI_actual_bits += PHI_Priv->PHI_MIN_BITS;
		}
	
		/*------------------------------------------------------------------*/
		/* Interpolation Procedure on LSPs                                  */
		/*------------------------------------------------------------------*/
	
		for(s = 0; s < (int)n_subframes; s++)
		{
			for(k = 0; k < (int)lpc_order; k++)
			{
			register float tmp;
	
			tmp = (((float)((int)n_subframes - s - 1) * PHI_Priv->previous_q_lsf_int_16[k]) 
			   + ((float)(1 + s) * PHI_Priv->current_q_lsf_16[k]))/(float)n_subframes;
			int_lsf[k] = tmp;

			}
	
			/*--------------------------------------------------------------*/
			/*Compute corresponding a-parameters                            */
			/*--------------------------------------------------------------*/
			lsf2pc(tmp_lpc_coefficients, int_lsf, lpc_order);
			for( k = 0; k < lpc_order; k++)
			{
			  int_Qlpc_coefficients[s*(int)lpc_order + k] = -tmp_lpc_coefficients[k+1];
			}
		}
	
		/* -----------------------------------------------------------------*/
		/* Save memory                                                      */
		/* -----------------------------------------------------------------*/
		for(k=0; k < (int)lpc_order; k++)
		{
			PHI_Priv->previous_uq_lsf_16[k] = PHI_Priv->current_uq_lsf_16[k];
			PHI_Priv->current_uq_lsf_16[k] = PHI_Priv->next_uq_lsf_16[k];
		}
	
		for(k=0; k < (int)lpc_order; k++)
		{
			PHI_Priv->previous_q_lsf_int_16[k] = PHI_Priv->current_q_lsf_16[k];
		}
	
		if ((*interpolation_flag) == 0)
		{
			for(k=0; k < (int)lpc_order; k++)
			{
				PHI_Priv->previous_q_lsf_16[k] = PHI_Priv->current_q_lsf_16[k];
				PHI_Priv->current_q_lsf_16[k] = PHI_Priv->next_q_lsf_16[k];
			}
		}
	
		/* -----------------------------------------------------------------*/
		/*FREE the malloc'd arrays                                         */
		/* -----------------------------------------------------------------*/
		free(int_lsf);
		free(next_codes);
		free(current_codes);
		free(tmp_lpc_coefficients);		
	    
	    }
	}
}

/*======================================================================*/
/*   Function Definition:celp_lpc_analysis_filter                       */
/*======================================================================*/
void celp_lpc_analysis_filter
(
float PP_InputSignal[],         /* In:  Input Signal [0..sbfrm_size-1]  */
float lpc_residual[],           /* Out: LPC residual [0..sbfrm_size-1]  */
float int_Qlpc_coefficients[],  /* In:  LPC Coefficients[0..lpc_order-1]*/
long  lpc_order,                /* In:  Order of LPC                    */
long  sbfrm_size,               /* In:  Number of samples in subframe   */
PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
)
{
    register float temp1,temp2, temp3;
    register int k;
    register int i;
    register float *pin;
    register float *pcoef;
    register float *pout;
    register float *p_mem,*p_mem1,*p_mem2;

    k    = (int)sbfrm_size;
    pin  = PP_InputSignal;
    pout = lpc_residual;

    do
    {
        temp1  = temp3 = (*pin++);
        pcoef  = int_Qlpc_coefficients;
        p_mem  = PHI_Priv->PHI_mem_i;
        p_mem1 = p_mem;
        p_mem2 = p_mem;
        i      = (int)lpc_order;
        do
        {
            temp1    -= (*pcoef++ * *p_mem++);
            temp2     = *p_mem2++;
            *p_mem1++ = temp3;
            temp3     = temp2;
        }
        while(--i);
        *pout++ = temp1;
    }
    while(--k);
}

/*======================================================================*/
/*   Function Definition:celp_lpc_synthesis_filter                      */
/*======================================================================*/
void celp_lpc_synthesis_filter
(
float excitation[],             /* In:  Input Signal [0..sbfrm_size-1]  */
float synth_signal[],           /* Out: LPC residual [0..sbfrm_size-1]  */
float int_Qlpc_coefficients[],  /* In:  LPC Coefficients[0..lpc_order-1]*/
long  lpc_order,                /* In:  Order of LPC                    */
long  sbfrm_size,               /* In:  Number of samples in subframe   */
PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
)
{
    register float *pin = excitation;
    register float *pout = synth_signal;
    register float temp3 = (float)0.0;
    register int   k     = (int)sbfrm_size;

    do
    {
        register float *pcoef = int_Qlpc_coefficients;
        register float *p_mem  = PHI_Priv->PHI_mem_s;
        register float *p_mem1 = p_mem;
        register float *p_mem2 = p_mem;
        register int i = (int)lpc_order;
        register float temp1;
        
        temp1  = *pin++;
        do
        {
    		register float temp2 = (float)0.0;
    		
            temp1    += *pcoef++ * *p_mem++;
            temp2     = *p_mem2++;
            *p_mem1++ = temp3;
            temp3     = temp2;
        }
        while(--i);
         p_mem    = PHI_Priv->PHI_mem_s;
        *p_mem++ = temp1;
        *pout++  = temp1;
    }
    while(--k);
}


/*======================================================================*/
/*   Function Definition:celp_weighting_module                          */
/*======================================================================*/
void celp_weighting_module
(
float lpc_coefficients[],       /* In:  LPC Coefficients[0..lpc_order-1]*/
long  lpc_order,                /* In:  Order of LPC                    */
float Wnum_coeff[],             /* Out: num. coeffs[0..lpc_order-1]     */
float Wden_coeff[],             /* Out: den. coeffs[0..lpc_order-1]     */
float gamma_num,                /* In:  Weighting factor: numerator     */
float gamma_den                 /* In:  Weighting factor: denominator   */
)
{
	int   k  	 = (int)lpc_order;
	float *pin1  = lpc_coefficients;
	float dgamma = gamma_den;
	float *pin3  = Wden_coeff;
	float ngamma = gamma_num;
	float *pin4  = Wnum_coeff;
    
    do
    {
        *pin3++ = *pin1   * dgamma;
        *pin4++ = *pin1++ * ngamma;
        dgamma *= gamma_den;    
        ngamma *= gamma_num;    
    }
    while(--k);
}


/*======================================================================*/
/*   Function Definition:PHI_Apar2Rfc                                   */
/*======================================================================*/
void PHI_Apar2Rfc
(
int    P,               /* In:  LPC Order                               */
float ap[],             /* In:  Polynomial coefficients [0..P-1]        */
float rfc[]             /* Out: Reflection coefficients [0..P-1]        */
)
{
    float  *temp1;
    float  *temp2;
    int    k;
    
    /*==================================================================*/
    /* Allocate memory for reflection coefficients and temps            */
    /*==================================================================*/
    if ( ((temp1 = (float *)malloc((unsigned int)P * sizeof(float)))== NULL ) ||
         ((temp2 = (float *)malloc((unsigned int)P * sizeof(float)))== NULL ) )
    {
       printf("\n Malloc Failure in Block:Excitation Anlaysis \n");
       exit(1);
    }
    
    /*==================================================================*/
    /* Initialise data                                                  */
    /*==================================================================*/
    for(k = 0; k < P; k++)
    {
        rfc[k]   = (float)0.0;
        temp1[k] = ap[k];
    }
    
    /*==================================================================*/
    /* Begin Recursion: This is implemented in accordance with the      */    
    /* algorithm as outlined in the book titled "Digital Proceesing of  */ 
    /* Speech" by Rabiner and Schafer, Chapter 8                        */
    /*==================================================================*/
    for(k = P - 1; k >= 0; k--)
    {
        int j;

        rfc[k] = temp1[k];

        if (fabs((double)rfc[k]) >= 1.0)
        {
            printf("FATAL ERROR: [%d] Unstable filter \n", k);
          FREE(temp1);
          FREE(temp2);
            exit(1);
        }
        else
        {
            for(j = 0; j < k; j++)
            {
                temp2[j] = (temp1[j] + (temp1[k] * temp1[k-j-1]))
                /((float)1.0 - rfc[k] * rfc[k]); 
            }
            for(j = 0; j < k; j++)
            {
                temp1[j] = temp2[j]; 
            }
        }
    }
    /*==================================================================*/
    /*FREE memory                                                      */
    /*==================================================================*/
  FREE (temp1);
  FREE (temp2);
}

/*======================================================================*/
/*   Function Definition:PHI_Rfc2Apar                                   */
/*======================================================================*/
static void PHI_Rfc2Apar
(
int   P,                /* In:  LPC Order                               */
float rq[],             /* In:  Reflection coefficients [0..P-1]        */
float aq[]              /* Out: a-parameters [0..P-1]                   */   
)
{
     register int k;
     float *temp;
     
    /*==================================================================*/
    /* Allocate memory for reflection coefficients and temps            */
    /*==================================================================*/
    if ((temp = (float *)malloc((unsigned int)P * sizeof(float)))== NULL )
    {
       printf("\n Malloc Failure in Block:Excitation Anlaysis \n");
       exit(1);
    }
    for(k = 0; k < P; k++)
    {
        register int j;

        aq[k] = rq[k];

        for(j = 0; j < k; j++)
        {
           aq[j] = temp[j] - rq[k]*temp[k-j-1];
        }
        for(j =0; j <=k; j++)
        {
            temp[j] = aq[j];
        }
    }
  FREE(temp);
}

/*======================================================================*/
/*   Function Definition:PHI_CalcAcf                                    */
/*======================================================================*/
static void PHI_CalcAcf
(
double sp_in[],         /* In:  Input segment [0..N-1]                  */
double acf[],           /* Out: Autocorrelation coeffs [0..P]           */
int N,                  /* In:  Number of samples in the segment        */ 
int P                   /* In:  LPC Order                               */
)
{
    int	    i, l;
    double  *pin1, *pin2;
    double  temp;
    
    for (i = 0; i <= P; i++)
    {
		pin1 = sp_in;
		pin2 = pin1 + i;
		temp = (double)0.0;
		for (l = 0; l < N - i; l++)
	    	temp += *pin1++ * *pin2++;
		*acf++ = temp;
    }
    return;
}

/*======================================================================*/
/*   Function Definition:PHI_LevinsonDurbin                             */
/*======================================================================*/
static int PHI_LevinsonDurbin     /* Retun Value: 1 if unstable filter            */ 
( 
double acf[],           /* In:  Autocorrelation coeffs [0..P]           */         
double apar[],          /* Out: Polynomial coeffciients  [0..P-1]       */           
double rfc[],           /* Out: Reflection coefficients [0..P-1]        */
int P,                  /* In:  LPC Order                               */
double * E              /* Out: Residual energy at the last stage       */
)
{
    int	    i, j;
    double  tmp1;
    double  *tmp;
    
    if ((tmp = (double *) malloc((unsigned int)P * sizeof(double))) == NULL)
    {
	printf("\nERROR in library procedure levinson_durbin: memory allocation failed!");
	exit(1);
    }
    
    *E = acf[0];
#ifdef DEBUG_YM
    if (*E > (double)1.0)
#else
    if (*E > (double)0.0)
#endif
    {
		for (i = 0; i < P; i++)
		{
	    	tmp1 = (double)0.0;
	    	for (j = 0; j < i; j++)
			tmp1 += tmp[j] * acf[i-j];
	    	rfc[i] = (acf[i+1] - tmp1) / *E;
#ifdef DEBUG_YM
		if (fabs(rfc[i]) >= 1.0 || *E <= acf[0] * 0.000001)
#else
	    	if (fabs(rfc[i]) >= 1.0)
#endif
	    	{
			for (j = i; j < P; j++)
		    	rfc[j] = (double)0.0;
#ifdef DEBUG_YM
                        fprintf(stderr, "phi_lpc : stop lpc at %d\n", i);
                        for (j = i; j < P; j++)
			  apar[j] = (double)0.0;
#endif
			free(tmp);
			return(1);  /* error in calculation */
	    	}
	    	apar[i] = rfc[i];
	    	for (j = 0; j < i; j++)
			apar[j] = tmp[j] - rfc[i] * tmp[i-j-1];
	    	*E *= (1.0 - rfc[i] * rfc[i]);
	    	for (j = 0; j <= i; j++)
			tmp[j] = apar[j];	
		}
		free(tmp);
		return(0);
    }
    else
    {
        for(i= 0; i < P; i++)
        {
            rfc[i] = 0.0;
            apar[i] = 0.0;
        }
		free(tmp);
		return(2);	    /* zero energy frame */
    }
}

/*======================================================================*/
/*   Function Definition:PHI_Rfc2Lar                                    */
/*======================================================================*/
static void PHI_Rfc2Lar
(
int   P,                /* In:  LPC Order                               */
float rfc[],            /* In:  Array of reflection coeffs[0..P-1]      */
float lar[]             /* Out: Array of log area ratios[0..P-1]        */
)
{
    int	  i;                  /* Local index                           */
    float *pi = rfc;          /* Pointer to input array                */ 
    float *po = lar;          /* Pointer to output array               */ 
    
    for (i = 0; i < P; i++)
    {
       if (*pi !=  (float)0.0)
       {
		   *po++ = (float)log((1.0 - (double)*pi)/(1.0 + (double)*pi));
	   }
	   else
	   {
		   *po++ = (float)0.0;
	   }
	   pi++;
    }
}

/*======================================================================*/
/*   Function Definition:PHI_Lar2Rfc                                    */
/*======================================================================*/
static int PHI_Lar2Rfc
(
int   P,                /* In:  LPC Order                               */
float lar[],            /* In:  Array of log area ratios[0..P-1]        */
float rfc[]             /* Out: Array of reflection coeffs[0..P-1]      */
)
{
    int	   i;                /* Local index                            */
    double r;                /* Intermediate variable for storage      */
    float  *po = rfc;        /* Pointer to output array                */ 
    float *pi = lar;         /* Pointer to input array                 */ 
    
    for (i = 0; i < P; i++)
    {
		r   = exp((double)*pi++);
		if (r != 0.0)
		{
		    *po = (float)((1.0 - r)/(1.0 + r));
		}
		else
		{
		    *po = (float)0.0;
		}
		if ((float)fabs((double)*po) >= (float)1.0)
		{
		    int j;

	    	for (j = i; j < P; j++)
	    	{
			    *po++ = (float)0.0;
			}
	    	return(1);	    /* error in reflection coefficient         */
		}
		po++; 
    }
	return(0);
}


/*======================================================================*/
/* Function Definition: PHI_PackLpcCodes                                */
/*======================================================================*/
static void PHI_PackLpcCodes
(
const long  lpc_order,        /*In:  Order of LPC                       */
const long  rfc_code[],       /*In:  Array of Refl. Coeffs[0..order-1]  */
const long  pack_array_size,  /*In:  Array size after packing           */
      long  packed_code[]     /*Out: Packed Array[0..pack_array-size-1] */
)
{   
    if (lpc_order == 20)
    {
        if (pack_array_size == 20)
        {
            long kk;
            
			/* Copy the rfc_codes */
            for (kk=0; kk < pack_array_size; kk++)
            {
                packed_code[kk] = rfc_code[kk];
            }
        }
        else
        {
        	if (pack_array_size != (short)9 )
        	{
            	printf("ERROR: Can only pack 9 codes \n");
            	exit(1);
        	}
        	packed_code[0] =((rfc_code[0] * 14  ) + rfc_code[3]);
        	packed_code[1] =((rfc_code[1] * 1155) +(rfc_code[2] * 77) + (rfc_code[7] * 7) + rfc_code[11]);
        	packed_code[2] =((rfc_code[4] * 156 ) +(rfc_code[5] * 12) + rfc_code[6]);
        	packed_code[3] =((rfc_code[8] * 7	) + rfc_code[12]);
        	packed_code[4] =rfc_code[9];
        	packed_code[5] =rfc_code[10];
        	packed_code[6] =rfc_code[13];
        	packed_code[7] =((rfc_code[14] * 36 ) +(rfc_code[15] * 6) + rfc_code[16]);
        	packed_code[8] =((rfc_code[17] * 36 ) +(rfc_code[18] * 6) + rfc_code[19]);

        	assert(packed_code[0] < 512);
        	assert(packed_code[1] < 32768);
        	assert(packed_code[2] < 2048);
        	assert(packed_code[3] < 64);
        	assert(packed_code[4] < 8);
        	assert(packed_code[5] < 8);
        	assert(packed_code[6] < 8);
        	assert(packed_code[7] < 256);
        	assert(packed_code[8] < 256);
        }
        
    }
    else
    if (lpc_order == 10)
    {
        if (pack_array_size == 10)
        {
            long kk;
            
			/* Copy the rfc_codes */
            for (kk=0; kk < pack_array_size; kk++)
            {
                packed_code[kk] = rfc_code[kk];
            }
        }
        else
		{
        	/*--------------------------------------------------------------*/
        	/* Narrowband Mode (10 coeffs packed in 4 indices)              */
        	/*--------------------------------------------------------------*/
        	if (pack_array_size != (short)4)
        	{
            	fprintf(stderr, "ERROR: Can only pack into 4 codes \n");
            	exit(1);
        	}

        	packed_code[0] =((rfc_code[0] * 108 ) +(rfc_code[6] * 9   ) + rfc_code[8]);
        	packed_code[1] =((rfc_code[1] * 143 ) +(rfc_code[4] * 11  ) + rfc_code[7]);
        	packed_code[2] =((rfc_code[2] * 182 ) +(rfc_code[3] * 13  ) + rfc_code[5]);
        	packed_code[3] =  rfc_code[9];

        	assert(packed_code[0] < 4096);
        	assert(packed_code[1] < 4096);
        	assert(packed_code[2] < 4096);
        	assert(packed_code[3] < 8);
		}
    }
	else
    {
        printf("Packing for order other than 10 or 20:Not implemented \n");
        exit(1);
    }
}

/*======================================================================*/
/* Function Definition: PHI_UnpackLpcCodes                              */
/*======================================================================*/
static void PHI_UnpackLpcCodes
(
const long  lpc_order,        /*In:  Order of LPC                       */      
      long  rfc_code[],       /*Out: Array of Refl. Coeffs[0..order-1]  */
const long  pack_array_size,  /*In:  Array size after packing           */
const long  packed_code[]     /*In:  Packed Array[0..pack_array-size-1] */
)
{
    if ((lpc_order == 9) || (lpc_order == 10))
    {
    	if ( (pack_array_size == (short)10 ) )
    	{
    		int k;

    		for(k=0; k < (int)lpc_order; k++)
    		{
        		rfc_code[k] = packed_code[k];      
    		}
    	}
    	else
		{
    		long codes[10];
			int k;

        	/*--------------------------------------------------------------*/
        	/* Narrowband Mode ( 10 coeffs packed in 4 indices)             */
        	/*--------------------------------------------------------------*/
    		if (pack_array_size != 3 && pack_array_size != 4)
    		{
        		fprintf(stderr, "Can only unpack 4 codes \n");
        		exit(1);
    		}

        	codes[0] = ( packed_code[0] / 108);
        	codes[1] = ( packed_code[1] / 143);
        	codes[2] = ( packed_code[2] / 182);
        	codes[3] = ((packed_code[2] / 13) % 14);
        	codes[4] = ((packed_code[1] / 11) % 13);
        	codes[5] = ( packed_code[2] % 13);
        	codes[6] = ((packed_code[0] /  9) % 12);
        	codes[7] = ( packed_code[1] % 11);
        	codes[8] = ( packed_code[0] %  9);
        	if (pack_array_size == 4)
        	{
        		codes[9] = ( packed_code[3] );
        	}

     		for(k=0; k < (int)lpc_order; k++)
    		{
        		rfc_code[k] = codes[k];      
    		}
		}
    }
    else
    {
    	if ( (pack_array_size >= (short)14 ) && (pack_array_size <= (short)20 ) )
    	{
    		int k;

    		for(k=0; k < (int)lpc_order; k++)
    		{
        		rfc_code[k] = packed_code[k];      
    		}
    	}
    	else
		{
    		long codes[20];
    		int k;

    		if ( (pack_array_size < (short)7 ) || (pack_array_size > (short)9 ) )
    		{
        		printf("ERROR: Cannot only pack less than 7 codes and more than 9 codes \n");
        		exit(1);
    		}

    		if (lpc_order > 20)
    		{
        		fprintf(stderr, "Packing for order higher than 20:Not implemented\n");
        		exit(1);
    		}

        	codes[0] = ( packed_code[0] / 14);
        	codes[1] = ( packed_code[1] / 1155);
        	codes[2] = ((packed_code[1] / 77) % 15);
        	codes[3] = ( packed_code[0] % 14);
        	codes[4] = ( packed_code[2] / 156);
        	codes[5] = ((packed_code[2] / 12) % 13);
        	codes[6] = ( packed_code[2] % 12);
        	codes[7] = ((packed_code[1] / 7) % 11);
        	codes[8] = ( packed_code[3] / 7);
        	codes[9] = ( packed_code[4]);
        	codes[10]= ( packed_code[5]);
        	codes[11]= ( packed_code[1] % 7);
        	codes[12]= ( packed_code[3] % 7);
        	codes[13]= ( packed_code[6]);
        	if (pack_array_size > (short)7)
        	{
        		codes[14]= ( packed_code[7] / 36);
        		codes[15]= ((packed_code[7] / 6) % 6);
        		codes[16]= ( packed_code[7] % 6);
        	}

        	if (pack_array_size > (short)8)
        	{
        		codes[17]= ( packed_code[8] / 36);
        		codes[18]= ((packed_code[8] / 6) % 6);
        		codes[19]= ( packed_code[8] % 6);
        	}

    		for(k=0; k < (int)lpc_order; k++)
    		{
        		rfc_code[k] = codes[k];      
    		}
		}
    }
}    


/*======================================================================*/
/*   Function Definition: PHI_Interpolation                             */
/*======================================================================*/
void PHI_Interpolation
(
    const long flag,                     /* In: Interpoaltion     flag  */
    PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
)
{
    /* -----------------------------------------------------------------*/
    /* Set interpolation switch for complexity scalability              */
    /* -----------------------------------------------------------------*/
    PHI_Priv->PHI_dec_int_switch = flag;
}




/*======================================================================*/
/*   Modified NEC functions                                             */
/*======================================================================*/

static void mod_nb_abs_lsp_quantizer (
    float current_lsp[],	    	/* in: current LSP to be quantized */
    float previous_Qlsp[],          /* In: previous Quantised LSP */
    float current_Qlsp[],	        /* out: quantized LSP */ 
    long lpc_indices[], 		    /* out: LPC code indices */
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices,           /* in: number of LPC indices */
    long n_lpc_analysis             /* in: number of LP analysis per frame */
)
{

    #include	"inc_lsp22.tbl"

    float *lsp_coefficients;
    float *Qlsp_coefficients, *prev_Qlsp_coefficients;
    long i;

    float *lsp_weight;
    float *d_lsp;

    float *lsp_tbl;
    float *d_tbl;
    float *pd_tbl;
    long *dim_1;
    long *dim_2;
    long *ncd_1;
    long *ncd_2;


/* Memory allocation */
    if((lsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(2);
	}
    if((prev_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(2);
	}
    if((lsp_weight=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(5);
	}
    if((d_lsp=(float *)calloc((lpc_order+1), sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(6);
	}

    for(i=0;i<lpc_order;i++) lsp_coefficients[i] = current_lsp[i]/PAN_PI;
    for(i=0;i<lpc_order;i++) prev_Qlsp_coefficients[i] = previous_Qlsp[i]/PAN_PI;

/* Weighting factor */
    d_lsp[0] = lsp_coefficients[0];
    for(i=1;i<lpc_order;i++) { 
        d_lsp[i] = lsp_coefficients[i]-lsp_coefficients[i-1];
    }
    d_lsp[lpc_order] = 1.-lsp_coefficients[lpc_order-1];
    for(i=0;i<=lpc_order;i++) {
        if(d_lsp[i]<PAN_MINGAP_CELP) d_lsp[i] = PAN_MINGAP_CELP;
    }
 
    for(i=0;i<=lpc_order;i++) d_lsp[i] = 1./d_lsp[i];
    for(i=0;i<lpc_order;i++) {
        lsp_weight[i] = d_lsp[i]+d_lsp[i+1];
    }

/* Not weighted 
    for(i=0;i<lpc_order;i++) lsp_weight[i] = 1.;
*/


/* Quantization */
    lsp_tbl = lsp_tbl22;
    d_tbl = d_tbl22;
    pd_tbl = pd_tbl22;
    dim_1 = dim22_1;
    dim_2 = dim22_2;
    ncd_1 = ncd22_1;
    ncd_2 = ncd22_2;
    pan_lspqtz2_dd(lsp_coefficients, 
        prev_Qlsp_coefficients, Qlsp_coefficients, 
        lsp_weight, PAN_LSP_AR_R_CELP, PAN_MINGAP_CELP, lpc_order, PAN_N_DC_LSP_CELP,     
        lpc_indices, lsp_tbl, d_tbl, pd_tbl, dim_1,
        ncd_1, dim_2, ncd_2, 1);

/* for Testing 
    for(i=0;i<lpc_order;i++) printf("%7.5f ", Qlsp_coefficients[i]);
    printf("\n");
*/
    for(i=0;i<lpc_order;i++) current_Qlsp[i] = Qlsp_coefficients[i]*PAN_PI;


  FREE(lsp_coefficients);
  FREE(Qlsp_coefficients);
  FREE(prev_Qlsp_coefficients);
  FREE(lsp_weight);
  FREE(d_lsp);
}

/*======================================================================*/
/*   Function Definition: mod_nb_abs_lsp_decode                         */
/*======================================================================*/

static void mod_nb_abs_lsp_decode(
    unsigned long lpc_indices[], 		    /* in: LPC code indices */
    float prev_Qlsp[],    		    /* in: previous LSP vector */
    float current_Qlsp[],	        /* out: quantized LSP vector */ 
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices            /* in: number of LPC indices */
)
{

    #include	"inc_lsp22.tbl"

    float *Qlsp_coefficients;
    float *prev_Qlsp_coefficients;
    float *tmp_lsp_coefficients;
    long i;

    float *lsp_tbl;
    float *d_tbl;
    float *pd_tbl;
    long *dim_1;
    long *dim_2;
    long *ncd_1;
    long *ncd_2;

/* Memory allocation */
    if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((prev_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((tmp_lsp_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(3);
	}

    for(i=0;i<lpc_order;i++) prev_Qlsp_coefficients[i] = prev_Qlsp[i]/PAN_PI;

/* LSP decode */
        lsp_tbl = lsp_tbl22;
        d_tbl = d_tbl22;
        pd_tbl = pd_tbl22;
        dim_1 = dim22_1;
        dim_2 = dim22_2;
        ncd_1 = ncd22_1;
        ncd_2 = ncd22_2;
	pan_lspdec(prev_Qlsp_coefficients, Qlsp_coefficients, 
            PAN_LSP_AR_R_CELP, PAN_MINGAP_CELP, lpc_order, lpc_indices, 
            lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 1, 1);

    for(i=0;i<lpc_order;i++) current_Qlsp[i] = Qlsp_coefficients[i]*PAN_PI;

/* for Testing 
    for(i=0;i<lpc_order;i++) printf("%7.5f ", Qlsp_coefficients[i]);
    printf("\n");
*/
  FREE(Qlsp_coefficients);
  FREE(prev_Qlsp_coefficients);
  FREE(tmp_lsp_coefficients);
}

/*======================================================================*/
/*   Function Definition: mod_nec_bws_lsp_decoder                       */
/*======================================================================*/

static void mod_nec_bws_lsp_decoder(
		     unsigned long indices[],		/* input  */
		     float qlsp8[],		/* input  */
		     float qlsp[],		/* output  */
		     long lpc_order,		/* configuration input */
		     long lpc_order_8,		/* configuration input */
		     long num_lpc_indices,  	/* configuration input */
		     float blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER],
		     PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
)		     
{
   long		i, j, k, sp_order;
   float	*tlsp, *vec_hat;
   float	*cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];
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
       PHI_Priv->nec_lsp_minwidth = NEC_LSP_MINWIDTH_FRQ16;
   } else {
      printf("Error in mod_nec_bws_lsp_decoder\n");
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
   mod_nec_lsp_sort( qlsp, lpc_order, PHI_Priv );

   for ( i = 0; i < lpc_order; i++ ) blsp[0][i] = tlsp[i];

   /*--- store previous vector ----*/
   for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) {
      for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
   }

 FREE( tlsp );
 FREE( vec_hat );

}


/*======================================================================*/
/*   Function Definition: mod_nec_bws_lsp_quantizer                     */
/*======================================================================*/

static void mod_nec_bws_lsp_quantizer(
		       float lsp[],		/* input  */
		       float qlsp8[],		/* input  */
		       float qlsp[],		/* output  */
		       long indices[],		/* output  */
		       long lpc_order,		/* configuration input */
		       long lpc_order_8,	/* configuration input */
		       long num_lpc_indices, 	/* configuration input */
		       float blsp[NEC_LSPPRDCT_ORDER][NEC_MAX_LSPVQ_ORDER],
		       PHI_PRIV_TYPE *PHI_Priv	      /* In/Out: PHI private data (instance context) */
)
{
   long		i, j, k;
   long		cb_size;
   long		cidx, sp_order;
   long		cand1[NEC_NUM_LSPSPLIT1][NEC_QLSP_CAND];
   long		cand2[NEC_NUM_LSPSPLIT2][NEC_QLSP_CAND];
   float	*qqlsp, *tlsp;
   float	*error, *error2;
   float	*vec_hat, *weight;
   float	mindist, dist, sub;
   float	*cb[1+NEC_NUM_LSPSPLIT1+NEC_NUM_LSPSPLIT2];
   long     frame_bit_allocation[6] = {NEC_BIT_LSP1620_0, NEC_BIT_LSP1620_1, NEC_BIT_LSP1620_2,
                                   NEC_BIT_LSP1620_3, NEC_BIT_LSP1620_4,
		                           NEC_BIT_LSP1620_5};

   /* Memory allocation */
   if ((qqlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((tlsp = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((error = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((error2 = (float *)calloc(lpc_order*NEC_QLSP_CAND, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((vec_hat = (float *)calloc(lpc_order, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
      exit(1);
   }
   if ((weight = (float *)calloc(lpc_order+2, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_lsp_quantizer \n");
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
     PHI_Priv->nec_lsp_minwidth = NEC_LSP_MINWIDTH_FRQ16;
   } else {
      printf("Error in mod_nec_bws_lsp_quantizer\n");
      exit(1);
   }

   /*--- calc. weight ----*/
   weight[0] = 0.0;
   weight[lpc_order+1] = (float)NEC_PAI;
   for ( i = 0; i < lpc_order; i++ )
      weight[i+1] = lsp[i];
   for ( i = 0; i <= lpc_order; i++ )
      weight[i] = 1.0/(weight[i+1]-weight[i]);
   for ( i = 0; i < lpc_order; i++ )
      weight[i] = (weight[i]+weight[i+1]);

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
   for ( i = 0; i < lpc_order; i++) error[i] = lsp[i] - vec_hat[i];

   /*--- 1st VQ -----*/
   sp_order = lpc_order/NEC_NUM_LSPSPLIT1;
   for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
      cb_size = 1<<frame_bit_allocation[i];
      mod_nec_psvq(error+i*sp_order,&cb[0][i*sp_order],cb[i+1],cb_size,sp_order,
	       weight+i*sp_order,cand1[i],NEC_QLSP_CAND);
   }
   for ( k = 0; k < NEC_QLSP_CAND; k++ ) {
      for ( i = 0; i < NEC_NUM_LSPSPLIT1; i++ ) {
	 for ( j = 0; j < sp_order; j++)
	    error2[k*lpc_order+i*sp_order+j] = 
	       error[i*sp_order+j] - cb[0][i*sp_order+j]
		                   * cb[i+1][sp_order*cand1[i][k]+j];
      }
   }

   /*--- 2nd VQ -----*/
   sp_order = lpc_order/NEC_NUM_LSPSPLIT2;
   for ( k = 0; k < NEC_QLSP_CAND; k++ ) {
      for ( i = 0; i < NEC_NUM_LSPSPLIT2; i++ ) {
	 cb_size = 1<<frame_bit_allocation[i+NEC_NUM_LSPSPLIT1];
	 mod_nec_psvq(error2+k*lpc_order+i*sp_order,&cb[0][i*sp_order],
		  cb[i+1+NEC_NUM_LSPSPLIT1], cb_size,sp_order,
		  weight+i*sp_order,&cand2[i][k],1);
      }
   }

   mindist = 1.0e30;
   for ( k = 0; k < NEC_QLSP_CAND*NEC_QLSP_CAND; k++ ) {
      switch ( k ) {
      case 0:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][0]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][0]+j];
	 break;
      case 1:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][1]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][1]+j];
	 break;
      case 2:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][0]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][0]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][0]+j];
	 break;
      case 3:
	 sp_order = 10;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] = cb[0+1][sp_order*cand1[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] = cb[1+1][sp_order*cand1[1][1]+j];
	 sp_order = 5;
	 for ( j = 0; j < sp_order; j++)
	    tlsp[0+j] += cb[0+1+2][sp_order*cand2[0][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[5+j] += cb[1+1+2][sp_order*cand2[1][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[10+j] += cb[2+1+2][sp_order*cand2[2][1]+j];
	 for ( j = 0; j < sp_order; j++)
	    tlsp[15+j] += cb[3+1+2][sp_order*cand2[3][1]+j];
	 break;
      }

      for ( i = 0; i < lpc_order; i++ ) qqlsp[i] = vec_hat[i]+cb[0][i]*tlsp[i];
      mod_nec_lsp_sort( qqlsp, lpc_order, PHI_Priv );
      dist = 0.0;
      for ( i = 0; i < lpc_order; i++ ) {
	 sub = lsp[i] - qqlsp[i];
	 dist += weight[i] * sub * sub;
      }
      if ( dist < mindist || k == 0 ) {
	 mindist = dist;
	 cidx = k;
	 for ( i = 0; i < lpc_order; i++ ) qlsp[i] = qqlsp[i];
	 for ( i = 0; i < lpc_order; i++ ) blsp[0][i] = tlsp[i];
      }
   }
   
   /*--- store previous vector ----*/
   for ( k = NEC_LSPPRDCT_ORDER-2; k > 0; k-- ) {
      for ( i = 0; i < lpc_order; i++ ) blsp[k][i] = blsp[k-1][i];
   }

   /*---- set INDEX -----*/
   indices[0] = cand1[0][cidx/2];
   indices[1] = cand1[1][cidx%2];
   indices[2] = cand2[0][cidx/2];
   indices[3] = cand2[1][cidx/2];
   indices[4] = cand2[2][cidx%2];
   indices[5] = cand2[3][cidx%2];

 FREE( qqlsp );
 FREE( tlsp );
 FREE( error );
 FREE( error2 );
 FREE( vec_hat );
 FREE( weight );

}

/*======================================================================*/
/*   Function Definition:  mod_nec_psvq                                 */
/*======================================================================*/

static void mod_nec_psvq( float vector[], float p[], float cb[],
	      long size, long order,
	      float weight[], long code[], long num )
{
   long		i, j, k;
   float	mindist, sub, *dist;

   if ((dist = (float *)calloc(size, sizeof(float))) == NULL) {
      printf("\n Memory allocation error in nec_svq \n");
      exit(1);
   }

   for ( i = 0; i < size; i++ ) {
      dist[i] = 0.0;
      for ( j = 0; j < order; j++ ) {
	 sub = vector[j] - p[j] * cb[i*order+j];
	 dist[i] += weight[j] * sub * sub;
      }
   }

   for ( k = 0; k < num; k++ ) {
      code[k] = 0;
      mindist = 1.0e30;
      for ( i = 0; i < size; i++ ) {
	 if ( dist[i] < mindist ) {
	    mindist = dist[i];
	    code[k] = i;
	 }
      }
      dist[code[k]] = 1.0e30;
   }
 FREE( dist );

}


/*======================================================================*/
/*   Function Definition: mod_nec_lsp_sort                              */
/*======================================================================*/

static void mod_nec_lsp_sort( float x[], long order , PHI_PRIV_TYPE *PHI_Priv)
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
         if ( x[j] + PHI_Priv->nec_lsp_minwidth > x[j+1] ) {
            tmp = 0.5 * (x[j] + x[j+1]);
            x[j] = tmp - 0.51 * PHI_Priv->nec_lsp_minwidth;
            x[j+1] = tmp + 0.51 * PHI_Priv->nec_lsp_minwidth;
         }
      }
   }
}


/* for LPC analysis with lag windiwing */
/*======================================================================*/
/*   Function Definition:celp_lpc_analysis                              */
/*======================================================================*/
void celp_lpc_analysis_lag
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
long  window_offsets[],         /* In:  offset for window w.r.t curr. fr*/
long  window_sizes[],           /* In:  LPC Analysis-Window Size        */
float *windows[],               /* In:  Array of LPC Analysis windows   */
float gamma_be[],               /* In:  Bandwidth expansion coefficients*/
long  lpc_order,                /* In:  Order of LPC                    */
long  n_lpc_analysis            /* In:  Number of LP analysis/frame     */
)
{
    int i;
    
    
    for(i = 0; i < (int)n_lpc_analysis; i++)
    {
         PHI_lpc_analysis_lag(PP_InputSignal,lpc_coefficients+lpc_order*(long)i, 
             first_order_lpc_par,
             frame_size, windows[i], window_offsets[i], window_sizes[i],
             gamma_be,
             lpc_order); 
    }
}

/*======================================================================*/
/*   Function Definition:PHI_lpc_analysis                               */
/*======================================================================*/
void PHI_lpc_analysis_lag
(
float PP_InputSignal[],         /* In:  Input Signal                    */
float lpc_coefficients[],       /* Out: LPC Coefficients[0..lpc_order-1]*/
float *first_order_lpc_par,     /* Out: a_parameter for 1st-order fit   */
long  frame_size,               /* In:  Number of samples in frame      */
float HamWin[],                 /* In:  Hamming Window                  */
long  window_offset,            /* In:  offset for window w.r.t curr. fr*/
long  window_size,              /* In:  LPC Analysis-Window Size        */
float gamma_be[],               /* In:  Bandwidth expansion coeffs.     */
long  lpc_order                 /* In:  Order of LPC                    */
)
{

static float lagWin_10[11] = {
 1.0001000F,
 0.9988903F,
 0.9955685F,
 0.9900568F,
 0.9823916F,
 0.9726235F,
 0.9608164F,
 0.9470474F,
 0.9314049F,
 0.9139889F,
 0.8949091F,
};

static float lagWin_20[21] = {
 1.0000100F,
 0.9997225F,
 0.9988903F,
 0.9975049F,
 0.9955685F,
 0.9930844F,
 0.9900568F,
 0.9864905F,
 0.9823916F,
 0.9777667F,
 0.9726235F,
 0.9669703F,
 0.9608164F,
 0.9541719F,
 0.9470474F,
 0.9394543F,
 0.9314049F,
 0.9229120F,
 0.9139889F,
 0.9046499F,
 0.8949091F,
};

    /*------------------------------------------------------------------*/
    /*    Volatile Variables                                            */
    /* -----------------------------------------------------------------*/
    double *acf;                 /* For Autocorrelation Function        */
    double *reflection_coeffs;   /* For Reflection Coefficients         */
    double *LpcAnalysisBlock;    /* For Windowed Input Signal           */
    double *apars;               /* a-parameters of double precision    */
    int k;
 
    /*------------------------------------------------------------------*/
    /* Allocate space for lpc, acf, a-parameters and rcf                */
    /*------------------------------------------------------------------*/

    if 
    (
    (( reflection_coeffs = (double *)malloc((unsigned int)lpc_order * sizeof(double))) == NULL ) ||
    (( acf = (double *)malloc((unsigned int)(lpc_order + 1) * sizeof(double))) == NULL ) ||
    (( apars = (double *)malloc((unsigned int)(lpc_order) * sizeof(double))) == NULL )  ||
    (( LpcAnalysisBlock = (double *)malloc((unsigned int)window_size * sizeof(double))) == NULL )
    )
    {
		printf("MALLOC FAILURE in Routine abs_lpc_analysis \n");
		exit(1);
    }
    
    /*------------------------------------------------------------------*/
    /* Windowing of the input signal                                    */
    /*------------------------------------------------------------------*/
    for(k = 0; k < (int)window_size; k++)
    {
        LpcAnalysisBlock[k] = (double)PP_InputSignal[k + (int)window_offset] * (double)HamWin[k];
    }    

    /*------------------------------------------------------------------*/
    /* Compute Autocorrelation                                          */
    /*------------------------------------------------------------------*/
    PHI_CalcAcf(LpcAnalysisBlock, acf, (int)window_size, (int)lpc_order);

/* lag windowing */
    if(NEC_LPC_ORDER==lpc_order) {
        for(k=0;k<=lpc_order;k++) *(acf+k) *= *(lagWin_10+k);
    }else if(NEC_LPC_ORDER_FRQ16==lpc_order) {
        for(k=0;k<=lpc_order;k++) *(acf+k) *= *(lagWin_20+k);
    }else {
        printf("\n irregular LPC order\n");
    }

    /*------------------------------------------------------------------*/
    /* Levinson Recursion                                               */
    /*------------------------------------------------------------------*/
    {
         double Energy = 0.0;
     
         PHI_LevinsonDurbin(acf, apars, reflection_coeffs,(int)lpc_order,&Energy);  
    }  
    
    /*------------------------------------------------------------------*/
    /* First-Order LPC Fit                                              */
    /*------------------------------------------------------------------*/
    *first_order_lpc_par = (float)reflection_coeffs[0];

    /*------------------------------------------------------------------*/
    /* Bandwidth Expansion                                              */
    /*------------------------------------------------------------------*/    

    for(k = 0; k < (int)lpc_order; k++)
    {
        lpc_coefficients[k] = (float)apars[k];
    }    

    /*------------------------------------------------------------------*/
    /*FREE the arrays that were malloced                               */
    /*------------------------------------------------------------------*/
  FREE(LpcAnalysisBlock);
  FREE(reflection_coeffs);
  FREE(acf);
  FREE(apars);
}

/*======================================================================*/
/*   Function Definition: mod_wb_celp_lsp_quantizer                     */
/*======================================================================*/

static void mod_wb_celp_lsp_quantizer (
    float current_lsp[],		/* in: current LSP to be quantized */
    float previous_Qlsp[],		/* In: previous Quantised LSP */
    float current_Qlsp[],		/* out: quantized LSP */ 
    long lpc_indices[],			/* out: LPC code indices */
    long lpc_order,			/* in: order of LPC */
    long num_lpc_indices,		/* in: number of LPC indices */
    long n_lpc_analysis			/* in: number of LP analysis per frame */
)
{

    #include "inc_lsp46w.tbl"

    float *lsp_coefficients;
    float *Qlsp_coefficients, *prev_Qlsp_coefficients;
    long i;

    float *lsp_weight;
    float *d_lsp;

    float *lsp_tbl;
    float *d_tbl;
    float *pd_tbl;
    long *dim_1;
    long *dim_2;
    long *ncd_1;
    long *ncd_2;

    long orderLsp;
    long offset;

/* Memory allocation */
    if((lsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(2);
	}
    if((prev_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(2);
	}
    if((lsp_weight=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(5);
	}
    if((d_lsp=(float *)calloc((lpc_order+1), sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(6);
	}

    for(i=0;i<lpc_order;i++) lsp_coefficients[i] = current_lsp[i]/PAN_PI;
    for(i=0;i<lpc_order;i++) prev_Qlsp_coefficients[i] = previous_Qlsp[i]/PAN_PI;

/* Weighting factor */
    d_lsp[0] = lsp_coefficients[0];
    for(i=1;i<lpc_order;i++) { 
        d_lsp[i] = lsp_coefficients[i]-lsp_coefficients[i-1];
    }
    d_lsp[lpc_order] = 1.-lsp_coefficients[lpc_order-1];
    for(i=0;i<=lpc_order;i++) {
        if(d_lsp[i]<PAN_MINGAP_CELP_W) d_lsp[i] = PAN_MINGAP_CELP_W;
    }
 
    for(i=0;i<=lpc_order;i++) d_lsp[i] = 1./d_lsp[i];
    for(i=0;i<lpc_order;i++) {
        lsp_weight[i] = d_lsp[i]+d_lsp[i+1];
    }

/* Quantization - lower part */
    orderLsp = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_L;
    d_tbl = d_tbl46w_L;
    pd_tbl = pd_tbl46w_L;
    dim_1 = dim46w_L1;
    dim_2 = dim46w_L2;
    ncd_1 = ncd46w_L1;
    ncd_2 = ncd46w_L2;
    pan_lspqtz2_dd(lsp_coefficients, 
        prev_Qlsp_coefficients, Qlsp_coefficients, 
        lsp_weight, PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, 
        orderLsp, PAN_N_DC_LSP_CELP_W,     
        lpc_indices, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 0);

/* Quantization - upper part */
    orderLsp = dim46w_U1[0]+dim46w_U1[1];
    offset = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_U;
    d_tbl = d_tbl46w_U;
    pd_tbl = pd_tbl46w_U;
    dim_1 = dim46w_U1;
    dim_2 = dim46w_U2;
    ncd_1 = ncd46w_U1;
    ncd_2 = ncd46w_U2;
    pan_lspqtz2_dd(lsp_coefficients+offset, 
        prev_Qlsp_coefficients+offset, Qlsp_coefficients+offset, 
        lsp_weight+offset, PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, 
        orderLsp, PAN_N_DC_LSP_CELP_W,     
        lpc_indices+5, lsp_tbl, d_tbl, pd_tbl, 
        dim_1, ncd_1, dim_2, ncd_2, 0);

    pan_stab(Qlsp_coefficients, PAN_MINGAP_CELP_W, lpc_order);

/* for Testing 
    for(i=0;i<lpc_order;i++) printf("%7.5f ", Qlsp_coefficients[i]);
    printf("\n");
*/
    for(i=0;i<lpc_order;i++) current_Qlsp[i] = Qlsp_coefficients[i]*PAN_PI;

  FREE(lsp_coefficients);
  FREE(Qlsp_coefficients);
  FREE(prev_Qlsp_coefficients);
  FREE(lsp_weight);
  FREE(d_lsp);
}

/*======================================================================*/
/*   Function Definition: mod_wb_celp_lsp_decode                        */
/*======================================================================*/

static void mod_wb_celp_lsp_decode(
    unsigned long lpc_indices[], 		    /* in: LPC code indices */
    float prev_Qlsp[],    		    /* in: previous LSP vector */
    float current_Qlsp[],	        /* out: quantized LSP vector */ 
    long lpc_order,			        /* in: order of LPC */
    long num_lpc_indices            /* in: number of LPC indices */
)
{

    #include "inc_lsp46w.tbl"

    float *Qlsp_coefficients;
    float *prev_Qlsp_coefficients;
    float *tmp_lsp_coefficients;
    long i;

    float *lsp_tbl;
    float *d_tbl;
    float *pd_tbl;
    long *dim_1;
    long *dim_2;
    long *ncd_1;
    long *ncd_2;

    long offset;
    long orderLsp;

/* Memory allocation */
    if((Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((prev_Qlsp_coefficients=(float *)calloc(lpc_order, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(1);
	}
    if((tmp_lsp_coefficients=(float *)calloc(lpc_order+1, sizeof(float)))==NULL) {
		printf("\n Memory allocation error in abs_lpc_quantizer\n");
		exit(3);
	}

    for(i=0;i<lpc_order;i++) prev_Qlsp_coefficients[i] = prev_Qlsp[i]/PAN_PI;

/* LSP decode - lower part */
    orderLsp = dim46w_L1[0]+dim46w_L1[1];
    lsp_tbl = lsp_tbl46w_L;
    d_tbl = d_tbl46w_L;
    pd_tbl = pd_tbl46w_L;
    dim_1 = dim46w_L1;
    dim_2 = dim46w_L2;
    ncd_1 = ncd46w_L1;
    ncd_2 = ncd46w_L2;
    pan_lspdec(prev_Qlsp_coefficients, Qlsp_coefficients, 
            PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, orderLsp, lpc_indices, 
            lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 0, 1);

/* LSP decode - upper part */
    offset = dim46w_L1[0]+dim46w_L1[1];
    orderLsp = dim46w_U1[0]+dim46w_U1[1];
    lsp_tbl = lsp_tbl46w_U;
    d_tbl = d_tbl46w_U;
    pd_tbl = pd_tbl46w_U;
    dim_1 = dim46w_U1;
    dim_2 = dim46w_U2;
    ncd_1 = ncd46w_U1;
    ncd_2 = ncd46w_U2;
    pan_lspdec(prev_Qlsp_coefficients+offset, Qlsp_coefficients+offset, 
            PAN_LSP_AR_R_CELP_W, PAN_MINGAP_CELP_W, orderLsp, lpc_indices+5, 
            lsp_tbl, d_tbl, pd_tbl, dim_1, ncd_1, dim_2, ncd_2, 0, 1);

    pan_stab(Qlsp_coefficients, PAN_MINGAP_CELP_W, lpc_order);

/* for Testing 
    for(i=0;i<lpc_order;i++) printf("%7.5f ", Qlsp_coefficients[i]);
    printf("\n");
*/
    for(i=0;i<lpc_order;i++) current_Qlsp[i] = Qlsp_coefficients[i]*PAN_PI;

  FREE(Qlsp_coefficients);
  FREE(prev_Qlsp_coefficients);
  FREE(tmp_lsp_coefficients);
}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
/* 30-07-96 R. Taori  Modified interface  to meet the MPEG-4 requirement*/
/* 30-08-96 R. Taori  Prefixed "PHI_" to several subroutines(MPEG req.) */
/* 07-11-96 N. Tanaka (Panasonic)                                       */
/*                    Added several modules for narrowband coder (PAN_) */
/* 14-11-96 A. Gerrits Introduction of dynamic threshold                */
/* 08-10-97 A. Gerrits Introduction of NEC VQ with dynamic threshold    */
