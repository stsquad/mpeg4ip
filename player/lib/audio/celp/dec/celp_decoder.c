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
14496-3) FREE license to this software module or modifications thereof
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
/*      SOURCE_FILE:    CELP_DECODER.C                                  */
/*                                                                      */
/*======================================================================*/
    
/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"     /* bit stream module                         */
#include "phi_cons.h"           /* Philips Defined Constants            */
#include "phi_priv.h"		/* for PHI private data storage */
#include "bitstream.h"
#include "lpc_common.h"         /* Common LPC core Defined Constants    */
#include "phi_gxit.h"           /* Excitation Generation Prototypes     */
#include "phi_lpc.h"            /* Prototypes of LPC Subroutines        */
#include "phi_post.h"           /* Post Processing Module Prototype     */
#include "phi_fbit.h"           /* Frame bit allocation table           */
#include "nec_abs_proto.h"      /* Prototypes for NEC Routines          */
#include "pan_celp_const.h"     /* Constants for PAN Routines           */
#include "celp_proto_dec.h"     /* Prototypes for CELP Routines         */
#include "celp_decoder.h"       /* Prototypes for CELP Decoder Routines */
#include "celp_bitstream_demux.h"/* Prototypes for CELP Decoder Routines*/
#include "nec_abs_const.h"      /* Constants for NEC Routines           */

int CELPdecDebugLevel = 0;	/* HP 971120 */

/* ---------------------------------------------------------------------*/ 
/* Frame counter                                                        */
/* ---------------------------------------------------------------------*/
static unsigned long frame_ctr = 0;   /* Frame Counter                   */

static long postfilter = 0;           /* Postfilter switch               */
static float *prev_Qlsp_coefficients; /* previous quantized LSP coeff.   */

static long num_enhstages;
static long dec_enhstages;
static long dec_bwsmode;
static long num_indices;

/*======================================================================*/
/*    NarrowBand Data declaration                                       */
/*======================================================================*/
static float *buf_Qlsp_coefficients_bws; /* current quantized LSP coeff. */
static float *prev_Qlsp_coefficients_bws;/* previous quantized LSP coeff. */

static long frame_size_nb, frame_size_bws;
static long n_subframes_bws;
static long sbfrm_size_bws;
static long lpc_order_bws;
static long num_lpc_indices_bws;
static long mp_pos_bits, mp_sgn_bits;
static long enh_pos_bits, enh_sgn_bits;
static long bws_pos_bits, bws_sgn_bits;

/*======================================================================*/
/* Type definitions                                                     */
/*======================================================================*/

typedef	struct
{
  PHI_PRIV_TYPE	*PHI_Priv;
  /* add private data pointers here for other coding varieties */
} INST_CONTEXT_LPC_DEC_TYPE;


/*======================================================================*/
/* Function definition: abs_coder                                       */
/*======================================================================*/
void celp_decoder
(
BsBitStream *bitStream,           /* In: Bitstream                     */
float     **OutputSignal,          /* Out: Multichannel Output          */
long        ExcitationMode,	   /* In: Excitation Mode	        */
long        SampleRateMode,	   /* In: SampleRate Mode		*/
long        QuantizationMode,	   /* In: Type of Quantization	        */
long        FineRateControl,	   /* In: Fine Rate Control switch   	*/
long        LosslessCodingMode,    /* In: Lossless Coding Mode	        */
long        RPE_configuration,	   /* In: RPE_configuration             */
long        Wideband_VQ,	   /* In: Wideband VQ mode		*/
long        MPE_Configuration,	   /* In: Narrowband configuration	*/
long        NumEnhLayers,	   /* In: Number of Enhancement Layers  */
long        BandwidthScalabilityMode, /* In: bandwidth switch  	        */
long        BWS_configuration,     /* In: BWS_configuration  	        */
long        frame_size,            /* In:  Frame size                   */
long        n_subframes,           /* In:  Number of subframes          */
long        sbfrm_size,            /* In:  Subframe size                */
long        lpc_order,             /* In:  Order of LPC                 */
long        num_lpc_indices,       /* In:  Number of LPC indices        */
long        num_shape_cbks,        /* In:  Number of Shape Codebooks    */
long        num_gain_cbks,         /* In:  Number of Gain Codebooks     */
long        *org_frame_bit_allocation, /* In: bit num. for each index     */
void        *InstanceContext	   /* In: pointer to instance context */
)
{
    /*==================================================================*/
    /*      L O C A L   D A T A   D E F I N I T I O N S                 */
    /*==================================================================*/
    float *int_ap;                                    /*Interpolated ap */
    unsigned long *shape_indices;                     /* Shape indices  */
    unsigned long *gain_indices;                      /* Gain indices   */
    unsigned long *indices;                           /* LPC codes*/
    long sbfrm_ctr = 0;
    long dum = 0;

    float *int_ap_bws;                               /*Interpolated ap */
    unsigned long *shape_indices_bws;                /* Shape indices  */
    unsigned long *gain_indices_bws;                 /* Gain indices   */
    unsigned long *indices_bws;                      /* LPC codes*/
    long *bws_nb_acb_index;                          /* ACB codes      */

    unsigned long interpolation_flag = 0;           /* Interpolation Flag*/
    unsigned long LPC_Present = 1;
    unsigned long signal_mode;
    unsigned long rms_index;

    PHI_PRIV_TYPE *PHI_Priv;	

    /* -----------------------------------------------------------------*/
    /* Set up pointers to private data                                  */
    /* -----------------------------------------------------------------*/
    PHI_Priv = ((INST_CONTEXT_LPC_DEC_TYPE *)InstanceContext)->PHI_Priv;
	
    /* -----------------------------------------------------------------*/
    /* Create Arrays for frame processing                               */
    /* -----------------------------------------------------------------*/
    if
    (
    (( int_ap = (float *)malloc((unsigned int)(n_subframes * lpc_order) * sizeof(float))) == NULL )||
    (( shape_indices = (unsigned long *)malloc((unsigned int)((num_enhstages+1)*num_shape_cbks * n_subframes) * sizeof(unsigned long))) == NULL )||
    (( gain_indices = (unsigned long *)malloc((unsigned int)((num_enhstages+1)*num_gain_cbks * n_subframes) * sizeof(unsigned long))) == NULL )||
    (( indices = (unsigned long *)malloc((unsigned int)num_lpc_indices * sizeof(unsigned long))) == NULL )
    )
    {
        fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
        exit(1);
    }

    if (BandwidthScalabilityMode == ON) {

      if
	(
	 (( int_ap_bws = (float *)malloc((unsigned int)(n_subframes_bws * lpc_order_bws) * sizeof(float))) == NULL )||
	 (( shape_indices_bws = (unsigned long *)malloc((unsigned int)(num_shape_cbks * n_subframes_bws) * sizeof(unsigned long))) == NULL )||
	 (( gain_indices_bws = (unsigned long *)malloc((unsigned int)(num_gain_cbks * n_subframes_bws) * sizeof(unsigned long))) == NULL )||
	 (( indices_bws = (unsigned long *)malloc((unsigned int)num_lpc_indices_bws * sizeof(unsigned long))) == NULL ) ||
	 (( bws_nb_acb_index = (long *)malloc((unsigned int)n_subframes_bws * sizeof(long))) == NULL )
	 )
	  {
	    fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
	    exit(1);
	  }
    }

    /* CelpBaseFrame() */
    /* Celp_LPC() */
    /*==================================================================*/
    /* CELP Lpc demux                                                   */
    /*==================================================================*/
   
    if (FineRateControl == ON)
    {
	/* ---------------------------------------------------------*/
	/* Step I: Read interpolation_flag and LPC_present flag     */
	/* ---------------------------------------------------------*/
	BsGetBit(bitStream, &interpolation_flag, 1);
	BsGetBit(bitStream, &LPC_Present, 1);

	/* ---------------------------------------------------------*/
	/* Step II: If LPC is present                               */
	/* ---------------------------------------------------------*/
	if (LPC_Present == YES)
	{
	    if (SampleRateMode == fs8kHz)
	    {
		Read_NarrowBand_LSP(bitStream, indices);
	    } 
	    else
	    {
		Read_Wideband_LSP(bitStream, indices);
	    }
	}
    }
    else
    {
	if (SampleRateMode == fs8kHz)
	{
	    Read_NarrowBand_LSP(bitStream, indices);
	} 
	else
	{
	    Read_Wideband_LSP(bitStream, indices);
	}
    }

    /*==================================================================*/
    /* CELP Excitation decoding                                         */
    /*==================================================================*/
    if ( ExcitationMode == RegularPulseExc )
    {  /* RPE_Frame() */
	long subframe;

	/*--------------------------------------------------------------*/
	/* Regular Pulse Excitation                                     */ 
	/*--------------------------------------------------------------*/
	for(subframe = 0; subframe < n_subframes; subframe++)
	{
	    /* ---------------------------------------------------------*/
	    /* Read the Adaptive Codebook Lag                           */
	    /* ---------------------------------------------------------*/
	    BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks], 8);

	    /* ---------------------------------------------------------*/
	    /*Read the Fixed Codebook Index (function of bit-rate)      */
	    /* ---------------------------------------------------------*/

	    switch (RPE_configuration)
	    {
		case     0   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 11);
		                 break;
		case     1   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 11);
		                 break;
		case     2   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 12);
		                 break;
		case     3   :   BsGetBit(bitStream, &shape_indices[subframe * num_shape_cbks + 1], 12);
		                 break;
	    }

	    /* ---------------------------------------------------------*/
	    /*Read the Adaptive Codebook Gain                           */
	    /* ---------------------------------------------------------*/
	    BsGetBit(bitStream, &gain_indices[subframe * num_gain_cbks], 6);
			
	    /* ---------------------------------------------------------*/
	    /*Read the Fixed Codebook Gain (function of subframe)       */
	    /*Later subframes are encoded differentially w.r.t previous */
	    /* ---------------------------------------------------------*/
	    if (subframe == 0)
	    {
		BsGetBit(bitStream, &gain_indices[subframe * num_gain_cbks + 1], 5);
	    }
	    else
	    {
		BsGetBit(bitStream, &gain_indices[subframe * num_gain_cbks + 1], 3);
	    }
			
	}
    }
    
    if (ExcitationMode == MultiPulseExc) { /* MPE_frame() */
      /*--------------------------------------------------------------*/
      /* Multi-Pulse Excitation                                       */ 
      /*--------------------------------------------------------------*/
      long i;

      BsGetBit(bitStream, &signal_mode, NEC_BIT_MODE);
      BsGetBit(bitStream, &rms_index, NEC_BIT_RMS);
      if (SampleRateMode == fs8kHz) {
	for ( i = 0; i < n_subframes; i++ ) {
	  BsGetBit(bitStream,
		 &shape_indices[i*num_shape_cbks+0], NEC_BIT_ACB);
	  BsGetBit(bitStream,
		 &shape_indices[i*num_shape_cbks+1], mp_pos_bits);
	  BsGetBit(bitStream,
		 &shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
	  BsGetBit(bitStream,
		 &gain_indices[i*num_gain_cbks+0], NEC_BIT_GAIN);
	}
      } else {
	for ( i = 0; i < n_subframes; i++ ) {
	  BsGetBit(bitStream,
		 &shape_indices[i*num_shape_cbks+0], NEC_ACB_BIT_WB);
	  BsGetBit(bitStream,
		 &shape_indices[i*num_shape_cbks+1], mp_pos_bits);
	  BsGetBit(bitStream,
		 &shape_indices[i*num_shape_cbks+2], mp_sgn_bits);
	  BsGetBit(bitStream,
		 &gain_indices[i*num_gain_cbks+0], NEC_BIT_GAIN_WB);
	}
      }
    }
    /* end of CelpBaseFrame() */

    if (ExcitationMode == MultiPulseExc) {
      long i, j;

      /* CelpBRSenhFrame() */
      if ( num_enhstages >= 1 ) {
	for ( j = 1; j <= num_enhstages; j++ ) {
	  for ( i = 0; i < n_subframes; i++ ) {
	    shape_indices[(j*n_subframes+i)*num_shape_cbks+0] = 0;
	    BsGetBit(bitStream,
		   &shape_indices[(j*n_subframes+i)*num_shape_cbks+1],
		   enh_pos_bits);
	    BsGetBit(bitStream,
		   &shape_indices[(j*n_subframes+i)*num_shape_cbks+2],
		   enh_sgn_bits);
	    BsGetBit(bitStream,
		   &gain_indices[(j*n_subframes+i)*num_gain_cbks+0],
		   NEC_BIT_ENH_GAIN);
	  }
	}
      }

      /* CelpBWSenhFrame() */
      if (SampleRateMode == fs8kHz && 
	  FineRateControl == OFF && QuantizationMode == VectorQuantizer &&
	  BandwidthScalabilityMode == ON) {
	Read_BandScalable_LSP(bitStream, indices_bws);

	for ( i = 0; i < n_subframes_bws; i++ ) {
	  BsGetBit(bitStream,
		 &shape_indices_bws[i*num_shape_cbks+0],
		 NEC_BIT_ACB_FRQ16);
	  BsGetBit(bitStream,
		 &shape_indices_bws[i*num_shape_cbks+1], bws_pos_bits);
	  BsGetBit(bitStream,
		 &shape_indices_bws[i*num_shape_cbks+2], bws_sgn_bits);
	  BsGetBit(bitStream,
		 &gain_indices_bws[i*num_gain_cbks+0],
		 NEC_BIT_GAIN_FRQ16);
	}
      }
    }

    /* -----------------------------------------------------------------*/
    /* Decode LPC coefficients                                          */
    /* -----------------------------------------------------------------*/
  
    if (FineRateControl == ON)
    {
	VQ_celp_lpc_decode(indices, int_ap, lpc_order, num_lpc_indices, n_subframes,
	                       interpolation_flag, Wideband_VQ, PHI_Priv);
    }	
    else
    {  /*  FineRate Control OFF  */
	if (SampleRateMode == fs8kHz)
	{
	    long k;

	    if (ExcitationMode == RegularPulseExc)
	    {
	      fprintf (stderr, "Combination of RPE + 8 kHz sampling rate not supported.\n");
	      exit (1);
	    }

	    if (ExcitationMode == MultiPulseExc)
	    {
		nb_abs_lpc_decode(indices, int_ap, lpc_order, n_subframes,
				  prev_Qlsp_coefficients);
	    }

	    if ( BandwidthScalabilityMode == ON)
	    {
		for ( k = 0; k < lpc_order; k++ )
		{
		    buf_Qlsp_coefficients_bws[k] = PAN_PI * prev_Qlsp_coefficients[k];
		}
		bws_lpc_decoder(indices_bws, int_ap_bws, lpc_order, lpc_order_bws,
				n_subframes_bws, buf_Qlsp_coefficients_bws, 
				prev_Qlsp_coefficients_bws );
	    }
	}
			
	if (SampleRateMode == fs16kHz)
	{
	    if (ExcitationMode == RegularPulseExc)
	    {
		VQ_celp_lpc_decode(indices, int_ap, lpc_order, num_lpc_indices, n_subframes,
		                   interpolation_flag, Wideband_VQ, PHI_Priv);
	    }
	    if (ExcitationMode == MultiPulseExc)
	    {
		wb_celp_lsp_decode(indices, int_ap, lpc_order, n_subframes,
		                   prev_Qlsp_coefficients);
	    }	    
	} 
    }

    if (ExcitationMode==RegularPulseExc)
    {
	/* -----------------------------------------------------------------*/
	/* Subframe processing                                              */
	/* -----------------------------------------------------------------*/
	for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++)
	{
	    long  m = sbfrm_ctr * lpc_order;
	    float *cbk_sig;
	    float *syn_speech;
	    float dumf;
		    
	    /* -------------------------------------------------------------*/
	    /* Create Arrays for subframe processing                        */
	    /* -------------------------------------------------------------*/
	    if
	    (
	    (( cbk_sig = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )||
	    (( syn_speech = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL )
	    )
	    {
		fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
		exit(1);
	    }
    
	    /* ------------------------------------------------------------*/
	    /* Excitation Generation                                       */
	    /* ------------------------------------------------------------*/
	    celp_excitation_generation(&(shape_indices[sbfrm_ctr*num_shape_cbks]),
	                               &(gain_indices[sbfrm_ctr*num_gain_cbks]), num_shape_cbks,
				       num_gain_cbks, dum, &int_ap[m], lpc_order, sbfrm_size,
				       n_subframes, dum, org_frame_bit_allocation, cbk_sig, &dum,
				       &dumf, PHI_Priv);
		    
	    /* ------------------------------------------------------------*/
	    /* Speech Generation                                           */
	    /* ------------------------------------------------------------*/
	    celp_lpc_synthesis_filter(cbk_sig, syn_speech, &int_ap[m], lpc_order, sbfrm_size, PHI_Priv);
    
	    if (postfilter)
	    {
		    /* --------------------------------------------------------*/
		    /* Post Processing                                         */
		    /* --------------------------------------------------------*/
		    celp_postprocessing(syn_speech, &(OutputSignal[0][sbfrm_ctr*sbfrm_size]), &int_ap[m], lpc_order, sbfrm_size, dum, dumf, PHI_Priv); 
    
	    }
	    else
	    {
		long k;
		float *psyn_speech   =  syn_speech;
		float *pOutputSignal =  &(OutputSignal[0][sbfrm_ctr*sbfrm_size]);
    
		for(k=0; k < sbfrm_size; k++)
		{
		    *pOutputSignal++ = *psyn_speech++;    
		}
	    }
		    
	    /* -------------------------------------------------------------*/
	    /* Free   Arrays for subframe processing                        */
	    /* -------------------------------------------------------------*/
	    FREE ( syn_speech );
	    FREE ( cbk_sig );
	}    
      } 

    if (ExcitationMode==MultiPulseExc)
    {
      float *dec_sig;
      float *bws_mp_sig;
      long  *acb_delay;
      float *adaptive_gain;
      float *dec_sig_bws;
      long  *bws_delay;
      float *bws_adpt_gain;

      float *syn_sig;	/* HP 971112 */
      long  kk;

      if (SampleRateMode == fs8kHz) {
	if(
	   (( dec_sig   = (float *)malloc((unsigned int)frame_size_nb * sizeof(float))) == NULL )||
	   (( bws_mp_sig   = (float *)malloc((unsigned int)frame_size_nb * sizeof(float))) == NULL )
	   ) {
	  fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
	  exit(1);
	}
      } else {
	if(
	   (( dec_sig   = (float *)malloc((unsigned int)frame_size * sizeof(float))) == NULL )||
	   (( bws_mp_sig   = (float *)malloc((unsigned int)frame_size * sizeof(float))) == NULL )
	   ) {
	  fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
	  exit(1);
	}
      }
      if((( acb_delay   = (long *)malloc(n_subframes * sizeof(long))) == NULL )||
		   (( adaptive_gain   = (float *)malloc(n_subframes * sizeof(float))) == NULL ) ) {
			fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
			exit(1);
      }

      for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++) {
	nb_abs_excitation_generation(shape_indices, gain_indices, 
				     num_shape_cbks, num_gain_cbks, rms_index,
				     int_ap+lpc_order*sbfrm_ctr, lpc_order, 
				     sbfrm_size, n_subframes, signal_mode, 
				     org_frame_bit_allocation,
				     dec_sig+sbfrm_size*sbfrm_ctr, 
				     bws_mp_sig+sbfrm_size*sbfrm_ctr, 
				     acb_delay+sbfrm_ctr,
				     adaptive_gain+sbfrm_ctr,
				     dec_enhstages,postfilter,
				     SampleRateMode);
      }

      if (BandwidthScalabilityMode == ON) {
	if(
	   (( dec_sig_bws   = (float *)malloc((unsigned int)frame_size_bws * sizeof(float))) == NULL )
	   ) {
	  fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
	  exit(1);
	}
	if((( bws_delay   = (long *)malloc(n_subframes_bws * sizeof(long))) == NULL )||
		   (( bws_adpt_gain   = (float *)malloc(n_subframes_bws * sizeof(float))) == NULL ) ) {
			fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
			exit(1);
	}

	for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++) {
	  bws_nb_acb_index[sbfrm_ctr] = shape_indices[sbfrm_ctr*n_subframes/n_subframes_bws*num_shape_cbks];
	}

	for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++) {
	  bws_excitation_generation(shape_indices_bws, gain_indices_bws, 
				    num_shape_cbks, num_gain_cbks, rms_index,
				    int_ap_bws+lpc_order_bws*sbfrm_ctr,
				    lpc_order_bws, 
				    sbfrm_size_bws, n_subframes_bws,
				    signal_mode, 
				    &org_frame_bit_allocation[num_indices-n_subframes_bws*(num_shape_cbks+num_gain_cbks)],
				    dec_sig_bws+sbfrm_size_bws*sbfrm_ctr, 
				    bws_mp_sig+sbfrm_size*n_subframes/n_subframes_bws*sbfrm_ctr,
				    bws_nb_acb_index,
				    bws_delay+sbfrm_ctr,bws_adpt_gain+sbfrm_ctr,postfilter);
	}
      }

      if ( (BandwidthScalabilityMode==ON) && (dec_bwsmode) ) {
        if(( syn_sig = (float *)malloc((unsigned int)sbfrm_size_bws * sizeof(float))) == NULL ){
	  fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
	  exit(1);
	}

	for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes_bws; sbfrm_ctr++) {
	  celp_lpc_synthesis_filter(dec_sig_bws+sbfrm_size_bws*sbfrm_ctr,
				    syn_sig,
				    int_ap_bws+lpc_order_bws*sbfrm_ctr,
				    lpc_order_bws, sbfrm_size_bws, PHI_Priv);
	  if (postfilter) {
	    nb_abs_postprocessing(syn_sig,
				  &(OutputSignal[0][sbfrm_ctr*sbfrm_size_bws]),
				  int_ap_bws+lpc_order_bws*sbfrm_ctr,
				  lpc_order_bws, sbfrm_size_bws,
				  bws_delay[sbfrm_ctr],
				  bws_adpt_gain[sbfrm_ctr]);
	  } else {
            for(kk=0; kk < sbfrm_size_bws; kk++)
	      OutputSignal[0][sbfrm_ctr*sbfrm_size_bws+kk] = syn_sig[kk];
	  }
	}
	FREE( syn_sig );
      } else {
        if(( syn_sig = (float *)malloc((unsigned int)sbfrm_size * sizeof(float))) == NULL ){
	  fprintf(stderr, "MALLOC FAILURE in abs_decoder  \n");
	  exit(1);
	}

	for (sbfrm_ctr = 0; sbfrm_ctr < n_subframes; sbfrm_ctr++) {
	  celp_lpc_synthesis_filter(dec_sig+sbfrm_size*sbfrm_ctr,
				    syn_sig,
				    int_ap+lpc_order*sbfrm_ctr,
				    lpc_order, sbfrm_size, PHI_Priv);
	  if (postfilter) {
	    nb_abs_postprocessing(syn_sig,
				  &(OutputSignal[0][sbfrm_ctr*sbfrm_size]),
				  int_ap+lpc_order*sbfrm_ctr,
				  lpc_order, sbfrm_size,
				  acb_delay[sbfrm_ctr],
				  adaptive_gain[sbfrm_ctr]);
	  } else {
            for(kk=0; kk < sbfrm_size; kk++)
	      OutputSignal[0][sbfrm_ctr*sbfrm_size+kk] = syn_sig[kk];
	  }
	}
	FREE( syn_sig );
      }

     FREE ( dec_sig );
      FREE ( bws_mp_sig );
      FREE ( acb_delay );
      FREE ( adaptive_gain );
      if (BandwidthScalabilityMode == ON) {
	FREE ( dec_sig_bws );
	FREE ( bws_delay );
	FREE ( bws_adpt_gain );
      }
    }

    /* -----------------------------------------------------------------*/
    /* Free   Arrays for Frame processing                               */
    /* -----------------------------------------------------------------*/
    FREE ( int_ap );
    FREE ( shape_indices );
    FREE ( gain_indices );
    FREE ( indices );
    
    if (SampleRateMode == fs8kHz) {
      if ( BandwidthScalabilityMode == ON ) {
	FREE ( int_ap_bws );
	FREE ( shape_indices_bws );
	FREE ( gain_indices_bws );
	FREE ( indices_bws );
	FREE ( bws_nb_acb_index );
      }
    }

    /* ----------------------------------------------------------------*/
    /* Report on the current frame count                               */
    /* ----------------------------------------------------------------*/
    frame_ctr++;
    if (frame_ctr % 10 == 0)
    {
      if (CELPdecDebugLevel) {	/* HP 971120 */
        fprintf(stderr, "Frame Counter: %ld \r", frame_ctr);
      }
    }

}

/*======================================================================*/
/*   Function Definition: PHI_Postfilter                                */
/*======================================================================*/
static void PHI_Postfilter
(
    const long flag                      /* In: Postfilter On/Off flag  */
)
{
    postfilter = flag;
}

/*======================================================================*/
/*   Function Definition:celp_initialisation_decoder                    */
/*======================================================================*/
void celp_initialisation_decoder
(
BsBitStream *hdrStream,           /* In: Bitstream                     */
long     bit_rate,		                 /* in: bit rate */
long     complexity_level,               /* In: complexity level decoder*/
long     reduced_order,                  /* In: reduced order decoder   */
long     DecEnhStage,
long     DecBwsMode,
long     PostFilterSW,
long     *frame_size,                    /* Out: frame size             */
long     *n_subframes,                   /* Out: number of  subframes   */
long     *sbfrm_size,                    /* Out: subframe size          */ 
long     *lpc_order,                     /* Out: LP analysis order      */
long     *num_lpc_indices,               /* Out: number of LPC indices  */
long     *num_shape_cbks,                /* Out: number of Shape Codeb. */    
long     *num_gain_cbks,                 /* Out: number of Gain Codeb.  */    
long     **org_frame_bit_allocation,     /* Out: bit num. for each index*/
long     * ExcitationMode,               /* Out: Excitation Mode	    */
long     * SampleRateMode,               /* Out: SampleRate Mode	    */
long     * QuantizationMode,             /* Out: Type of Quantization	*/
long     * FineRateControl,              /* Out: Fine Rate Control switch*/
long     * LosslessCodingMode,           /* Out: Lossless Coding Mode  	*/
long     * RPE_configuration,             /* Out: Wideband configuration */
long     * Wideband_VQ, 	             /* Out: Wideband VQ mode		*/
long     * MPE_Configuration,             /* Out: Narrowband configuration*/
long     * NumEnhLayers,	             /* Out: Number of Enhancement Layers*/
long     * BandwidthScalabilityMode,     /* Out: bandwidth switch	    */
long     * BWS_configuration,            /* Out: BWS_configuration		*/
void	 **InstanceContext,		 /* Out: handle to initialised instance context */
int      mp4ffFlag
)
{

   INST_CONTEXT_LPC_DEC_TYPE	*InstCtxt;
   frame_ctr = 0;   /* Frame Counter                   */

   postfilter = 0;           /* Postfilter switch               */
   prev_Qlsp_coefficients = NULL; /* previous quantized LSP coeff.   */

   buf_Qlsp_coefficients_bws = NULL; /* current quantized LSP coeff. */
   prev_Qlsp_coefficients_bws = NULL;/* previous quantized LSP coeff. */

    /* -----------------------------------------------------------------*/
    /* Create & initialise private storage for instance context         */
    /* -----------------------------------------------------------------*/
    if (( InstCtxt = (INST_CONTEXT_LPC_DEC_TYPE*)malloc(sizeof(INST_CONTEXT_LPC_DEC_TYPE))) == NULL )
    {
      fprintf(stderr, "MALLOC FAILURE in celp_initialisation_decoder  \n");
      exit(1);
    }

    if (( InstCtxt->PHI_Priv = (PHI_PRIV_TYPE*)malloc(sizeof(PHI_PRIV_TYPE))) == NULL )
    {
      fprintf(stderr, "MALLOC FAILURE in celp_initialisation_decoder  \n");
      exit(1);
    }
    PHI_Init_Private_Data(InstCtxt->PHI_Priv);

    *InstanceContext = InstCtxt;

    /* -----------------------------------------------------------------*/
    /*                                                                  */
    /* -----------------------------------------------------------------*/
  
    postfilter = PostFilterSW;
    
    /* -----------------------------------------------------------------*/
    /* Read bitstream header                                            */
    /* -----------------------------------------------------------------*/ 
    /* read from object descriptor */

    if (mp4ffFlag==0) 
      read_celp_bitstream_header(hdrStream, ExcitationMode, SampleRateMode, 
                               QuantizationMode, FineRateControl, LosslessCodingMode,
                               RPE_configuration, Wideband_VQ, MPE_Configuration,
                               NumEnhLayers, BandwidthScalabilityMode, BWS_configuration);


    if (*ExcitationMode == RegularPulseExc)
    {
	if (*SampleRateMode == fs8kHz)
	{
	    fprintf (stderr, "Combination of RPE + 8 kHz sampling rate not supported.\n");
	    exit (1);
	}
    
	/* -----------------------------------------------------------------*/
	/*Check if a bit rate is a set of allowed bit rates                 */
	/* -----------------------------------------------------------------*/ 
	if (*RPE_configuration == 0)
	{
		*frame_size = FIFTEEN_MS;
		*n_subframes = 6;        
	}
	else
	if (*RPE_configuration == 1)
	{
		*frame_size  = TEN_MS;
		*n_subframes = 4;        
	}
	else
	if (*RPE_configuration == 2)
	{
		*frame_size  = FIFTEEN_MS;
		*n_subframes = 8;
	}
	else
	if (*RPE_configuration == 3)
	{
		*frame_size  = FIFTEEN_MS;
		*n_subframes = 10;
	}
	else
	{
		fprintf(stderr, "ERROR: Illegal RPE Configuration\n");
		exit(1); 
	}

	*sbfrm_size          = (*frame_size)/(*n_subframes);

	*num_shape_cbks      = 2;     
	*num_gain_cbks       = 2;     

	*lpc_order       = ORDER_LPC_16;
	*num_lpc_indices = N_INDICES_VQ16;
		
	PHI_init_excitation_generation( Lmax, *sbfrm_size, *RPE_configuration, InstCtxt->PHI_Priv );
	PHI_InitLpcAnalysisDecoder(ORDER_LPC_16, ORDER_LPC_8, InstCtxt->PHI_Priv);
	PHI_InitPostProcessor(*lpc_order, InstCtxt->PHI_Priv );
		
	*org_frame_bit_allocation = PHI_init_bit_allocation(*SampleRateMode, *RPE_configuration,
							    *QuantizationMode, *LosslessCodingMode,
							    *FineRateControl, *num_lpc_indices,
							    *n_subframes, *num_shape_cbks, *num_gain_cbks); 
   }	
 
   if (*ExcitationMode == MultiPulseExc) {
     if (*SampleRateMode == fs8kHz) {
       int i, j;
       long	ctr;
		
       num_enhstages = *NumEnhLayers;
       dec_enhstages = DecEnhStage;
       dec_bwsmode = DecBwsMode;

       if ( *MPE_Configuration >= 0 && *MPE_Configuration < 3 ) {
	 frame_size_nb = NEC_FRAME40MS;
	 *n_subframes = NEC_NSF4;
       }
       if ( *MPE_Configuration >= 3 && *MPE_Configuration < 6 ) {
	 frame_size_nb = NEC_FRAME30MS;
	 *n_subframes = NEC_NSF3;
       }
       if ( *MPE_Configuration >= 6 && *MPE_Configuration < 13 ) {
	 frame_size_nb = NEC_FRAME20MS;
	 *n_subframes = NEC_NSF2;
       }
       if ( *MPE_Configuration >= 13 && *MPE_Configuration < 22 ) {
	 frame_size_nb = NEC_FRAME20MS;
	 *n_subframes = NEC_NSF4;
       }
       if ( *MPE_Configuration >= 22 && *MPE_Configuration < 27 ) {
	 frame_size_nb = NEC_FRAME10MS;
	 *n_subframes = NEC_NSF2;
       }
       if ( *MPE_Configuration == 27 ) {
	 frame_size_nb = NEC_FRAME30MS;
	 *n_subframes = NEC_NSF4;
       }
       if ( *MPE_Configuration > 27 ) {
	 fprintf(stderr,"Error: Illegal BitRate configuration.\n");
	 exit(1); 
       }

       *sbfrm_size = frame_size_nb/(*n_subframes);
       *lpc_order = NEC_LPC_ORDER;
       *num_shape_cbks = NEC_NUM_SHAPE_CBKS;
       *num_gain_cbks = NEC_NUM_GAIN_CBKS;
       if (*QuantizationMode == ScalarQuantizer) {
	 *num_lpc_indices = 4;
       } else {
	 *num_lpc_indices = PAN_NUM_LPC_INDICES;
       }

       num_indices = NEC_NUM_OTHER_INDICES + PAN_NUM_LPC_INDICES
	 + (num_enhstages + 1) * (*n_subframes) *
	   (NEC_NUM_SHAPE_CBKS+NEC_NUM_GAIN_CBKS);

       switch ( *MPE_Configuration ) {
       case 0:
	 mp_pos_bits = 14; mp_sgn_bits =  3; break;
       case 1:
	 mp_pos_bits = 17; mp_sgn_bits =  4; break;
       case 2:
	 mp_pos_bits = 20; mp_sgn_bits =  5; break;
       case 3:
	 mp_pos_bits = 20; mp_sgn_bits =  5; break;
       case 4:
	 mp_pos_bits = 22; mp_sgn_bits =  6; break;
       case 5:
	 mp_pos_bits = 24; mp_sgn_bits =  7; break;
       case 6:
	 mp_pos_bits = 22; mp_sgn_bits =  6; break;
       case 7:
	 mp_pos_bits = 24; mp_sgn_bits =  7; break;
       case 8:
	 mp_pos_bits = 26; mp_sgn_bits =  8; break;
       case 9:
	 mp_pos_bits = 28; mp_sgn_bits =  9; break;
       case 10:
	 mp_pos_bits = 30; mp_sgn_bits = 10; break;
       case 11:
	 mp_pos_bits = 31; mp_sgn_bits = 11; break;
       case 12:
	 mp_pos_bits = 32; mp_sgn_bits = 12; break;
       case 13:
	 mp_pos_bits = 13; mp_sgn_bits =  4; break;
       case 14:
	 mp_pos_bits = 15; mp_sgn_bits =  5; break;
       case 15:
	 mp_pos_bits = 16; mp_sgn_bits =  6; break;
       case 16:
	 mp_pos_bits = 17; mp_sgn_bits =  7; break;
       case 17:
	 mp_pos_bits = 18; mp_sgn_bits =  8; break;
       case 18:
	 mp_pos_bits = 19; mp_sgn_bits =  9; break;
       case 19:
	 mp_pos_bits = 20; mp_sgn_bits =  10; break;
       case 20:
	 mp_pos_bits = 20; mp_sgn_bits =  11; break;
       case 21:
	 mp_pos_bits = 20; mp_sgn_bits =  12; break;
       case 22:
	 mp_pos_bits = 18; mp_sgn_bits =  8; break;
       case 23:
	 mp_pos_bits = 19; mp_sgn_bits =  9; break;
       case 24:
	 mp_pos_bits = 20; mp_sgn_bits =  10; break;
       case 25:
	 mp_pos_bits = 20; mp_sgn_bits =  11; break;
       case 26:
	 mp_pos_bits = 20; mp_sgn_bits =  12; break;
       case 27:
	 mp_pos_bits = 19; mp_sgn_bits =  6; break;
       }

       if ( *sbfrm_size == (NEC_FRAME20MS/NEC_NSF4) ) {
	 enh_pos_bits = NEC_BIT_ENH_POS40_2;
	 enh_sgn_bits = NEC_BIT_ENH_SGN40_2;
       } else {
	 enh_pos_bits = NEC_BIT_ENH_POS80_4;
	 enh_sgn_bits = NEC_BIT_ENH_SGN80_4;
       }

       if (*BandwidthScalabilityMode==ON) {
	 frame_size_bws = frame_size_nb * 2;
	 n_subframes_bws = frame_size_bws/80;
	 sbfrm_size_bws = frame_size_bws / n_subframes_bws;
	 lpc_order_bws = NEC_LPC_ORDER_FRQ16;

	 num_lpc_indices_bws = NEC_NUM_LPC_INDICES_FRQ16 ;
	 num_indices += NEC_NUM_LPC_INDICES_FRQ16 
	   + n_subframes_bws * (NEC_NUM_SHAPE_CBKS
				+NEC_NUM_GAIN_CBKS);
	 switch ( *BWS_configuration ) {
	 case 0:
	   bws_pos_bits = 22; bws_sgn_bits = 6; break;
	 case 1:
	   bws_pos_bits = 26; bws_sgn_bits = 8; break;
	 case 2:
	   bws_pos_bits = 30; bws_sgn_bits =10; break;
	 case 3:
	   bws_pos_bits = 32; bws_sgn_bits =12; break;
	 }
       }

       if ( (*BandwidthScalabilityMode==ON) && (dec_bwsmode) ) {
	 *frame_size = frame_size_bws;
       } else {
	 *frame_size = frame_size_nb;
       }

       if((*org_frame_bit_allocation=(long *)calloc(num_indices, 
						    sizeof(long)))==NULL) {
	 fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
	 exit(3);
       }

       ctr = 0;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_0;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_1;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_2;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_3;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP22_4;
       *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_MODE;
       *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_RMS;
       for ( i = 0; i < *n_subframes; i++ ) {
	 *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ACB;
	 *(*org_frame_bit_allocation+(ctr++)) =  mp_pos_bits;
	 *(*org_frame_bit_allocation+(ctr++)) =  mp_sgn_bits;
	 *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_GAIN;
       }

       for ( i = 0; i < num_enhstages; i++ ) {
	 for ( j = 0; j < *n_subframes; j++ ) {
	   *(*org_frame_bit_allocation+(ctr++)) =  0;
	   *(*org_frame_bit_allocation+(ctr++)) =  enh_pos_bits;
	   *(*org_frame_bit_allocation+(ctr++)) =  enh_sgn_bits;
	   *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ENH_GAIN;
	 }
       }

       if (*BandwidthScalabilityMode==ON) {
	 *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_0;
	 *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_1;
	 *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_2;
	 *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_3; 
	 *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_4; 
	 *(*org_frame_bit_allocation+(ctr++)) = NEC_BIT_LSP1620_5;
	 for ( i = 0; i < n_subframes_bws; i++ ) {
	   *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ACB_FRQ16;
	   *(*org_frame_bit_allocation+(ctr++)) =  bws_pos_bits;
	   *(*org_frame_bit_allocation+(ctr++)) =  bws_sgn_bits;
	   *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_GAIN_FRQ16;
	 }
       }

       if((prev_Qlsp_coefficients=(float *)calloc(*lpc_order,
						  sizeof(float)))==NULL) {
	 fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
	 exit(5);
       }

       for(i=0;i<(*lpc_order);i++) 
	 *(prev_Qlsp_coefficients+i) = (i+1)/(float)((*lpc_order)+1);

       if (*BandwidthScalabilityMode==ON) {
	 if((buf_Qlsp_coefficients_bws=(float *)calloc(*lpc_order,
						      sizeof(float)))==NULL) {
	   fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
	   exit(5);
	 }

	 if((prev_Qlsp_coefficients_bws=(float *)calloc(lpc_order_bws,
							sizeof(float)))==NULL) {
	   fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
	   exit(5);
	 }
	 for(i=0;i<(lpc_order_bws);i++) 
	   *(prev_Qlsp_coefficients_bws+i) = PAN_PI * (i+1)
	     / (float)(lpc_order_bws+1);
       }

       /* submodules for initialization */
       if ((*BandwidthScalabilityMode==ON)&&(dec_bwsmode)) {
	 PHI_InitLpcAnalysisDecoder(lpc_order_bws, *lpc_order, InstCtxt->PHI_Priv);
       } else {
	 PHI_InitLpcAnalysisDecoder(*lpc_order, *lpc_order, InstCtxt->PHI_Priv);
       }
       if (*LosslessCodingMode == ON) {
	 *num_lpc_indices     = 10;   
       }
     }

     if (*SampleRateMode == fs16kHz) {
       int i, j;
       long	ctr;
		
       num_enhstages = *NumEnhLayers;
       dec_enhstages = DecEnhStage;

       if ((*MPE_Configuration>=0) && (*MPE_Configuration<7)) {
	 *frame_size = NEC_FRAME20MS_FRQ16;
	 *n_subframes = NEC_NSF4;
       } else if((*MPE_Configuration>=8)&&(*MPE_Configuration<16)) {
	 *frame_size = NEC_FRAME20MS_FRQ16;
	 *n_subframes = NEC_NSF8;
       } else if((*MPE_Configuration>=16)&&(*MPE_Configuration<23)) {
	 *frame_size = NEC_FRAME10MS_FRQ16;
	 *n_subframes = NEC_NSF2;
       } else if((*MPE_Configuration>=24)&&(*MPE_Configuration<32)) {
	 *frame_size = NEC_FRAME10MS_FRQ16;
	 *n_subframes = NEC_NSF4;
       } else {
	 fprintf(stderr,"Error: Illegal BitRate configuration.\n");
	 exit(1); 
       }

       *sbfrm_size = *frame_size/(*n_subframes);
       *lpc_order = NEC_LPC_ORDER_FRQ16;
       *num_shape_cbks = NEC_NUM_SHAPE_CBKS;
       *num_gain_cbks = NEC_NUM_GAIN_CBKS;
       *num_lpc_indices = PAN_NUM_LPC_INDICES_W;
       
       num_indices = NEC_NUM_OTHER_INDICES + PAN_NUM_LPC_INDICES_W
	 + (num_enhstages + 1) * (*n_subframes) *
	   (NEC_NUM_SHAPE_CBKS+NEC_NUM_GAIN_CBKS);

       switch ( *MPE_Configuration ) {
       case 0:
	 mp_pos_bits = 20; mp_sgn_bits =  5; break;
       case 1:
	 mp_pos_bits = 22; mp_sgn_bits =  6; break;
       case 2:
	 mp_pos_bits = 24; mp_sgn_bits =  7; break;
       case 3:
	 mp_pos_bits = 26; mp_sgn_bits =  8; break;
       case 4:
	 mp_pos_bits = 28; mp_sgn_bits =  9; break;
       case 5:
	 mp_pos_bits = 30; mp_sgn_bits =  10; break;
       case 6:
	 mp_pos_bits = 31; mp_sgn_bits =  11; break;
       case 7:
	 break;
       case 8:
	 mp_pos_bits = 11; mp_sgn_bits =  3; break;
       case 9:
	 mp_pos_bits = 13; mp_sgn_bits =  4; break;
       case 10:
	 mp_pos_bits = 15; mp_sgn_bits =  5; break;
       case 11:
	 mp_pos_bits = 16; mp_sgn_bits =  6; break;
       case 12:
	 mp_pos_bits = 17; mp_sgn_bits =  7; break;
       case 13:
	 mp_pos_bits = 18; mp_sgn_bits =  8; break;
       case 14:
	 mp_pos_bits = 19; mp_sgn_bits =  9; break;
       case 15:
	 mp_pos_bits = 20; mp_sgn_bits =  10; break;
       case 16:
	 mp_pos_bits = 20; mp_sgn_bits =  5; break;
       case 17:
	 mp_pos_bits = 22; mp_sgn_bits =  6; break;
       case 18:
	 mp_pos_bits = 24; mp_sgn_bits =  7; break;
       case 19:
	 mp_pos_bits = 26; mp_sgn_bits =  8; break;
       case 20:
	 mp_pos_bits = 28; mp_sgn_bits =  9; break;
       case 21:
	 mp_pos_bits = 30; mp_sgn_bits =  10; break;
       case 22:
	 mp_pos_bits = 31; mp_sgn_bits =  11; break;
       case 23:
	 break;
       case 24:
	 mp_pos_bits = 11; mp_sgn_bits =  3; break;
       case 25:
	 mp_pos_bits = 13; mp_sgn_bits =  4; break;
       case 26:
	 mp_pos_bits = 15; mp_sgn_bits =  5; break;
       case 27:
	 mp_pos_bits = 16; mp_sgn_bits =  6; break;
       case 28:
	 mp_pos_bits = 17; mp_sgn_bits =  7; break;
       case 29:
	 mp_pos_bits = 18; mp_sgn_bits =  8; break;
       case 30:
	 mp_pos_bits = 19; mp_sgn_bits =  9; break;
       case 31:
	 mp_pos_bits = 20; mp_sgn_bits =  10; break;
       }

       if ( *sbfrm_size == NEC_SBFRM_SIZE40 ) {
	 enh_pos_bits = NEC_BIT_ENH_POS40_2;
	 enh_sgn_bits = NEC_BIT_ENH_SGN40_2;
       } else {
	 enh_pos_bits = NEC_BIT_ENH_POS80_4;
	 enh_sgn_bits = NEC_BIT_ENH_SGN80_4;
       }

       if((*org_frame_bit_allocation=(long *)calloc(num_indices, 
						    sizeof(long)))==NULL) {
	 fprintf(stderr,"\n memory allocation error in initialization_encoder\n");
	 exit(3);
       }

       ctr = 0;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_0;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_1;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_2;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_3;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WL_4;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_0;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_1;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_2;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_3;
       *(*org_frame_bit_allocation+(ctr++)) =  PAN_BIT_LSP_WU_4;
       *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_MODE;
       *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_RMS;
       for ( i = 0; i < *n_subframes; i++ ) {
	 *(*org_frame_bit_allocation+(ctr++)) =  NEC_ACB_BIT_WB;
	 *(*org_frame_bit_allocation+(ctr++)) =  mp_pos_bits;
	 *(*org_frame_bit_allocation+(ctr++)) =  mp_sgn_bits;
	 *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_GAIN_WB;
       }

       for ( i = 0; i < num_enhstages; i++ ) {
	 for ( j = 0; j < *n_subframes; j++ ) {
	   *(*org_frame_bit_allocation+(ctr++)) =  0;
	   *(*org_frame_bit_allocation+(ctr++)) =  enh_pos_bits;
	   *(*org_frame_bit_allocation+(ctr++)) =  enh_sgn_bits;
	   *(*org_frame_bit_allocation+(ctr++)) =  NEC_BIT_ENH_GAIN;
	 }
       }

       if((prev_Qlsp_coefficients=(float *)calloc(*lpc_order,
						  sizeof(float)))==NULL) {
	 fprintf(stderr,"\n memory allocation error in initialization_decoder\n");
	 exit(5);
       }

       for(i=0;i<(*lpc_order);i++) 
	 *(prev_Qlsp_coefficients+i) = (i+1)/(float)((*lpc_order)+1);

       /* submodules for initialization */
       PHI_InitLpcAnalysisDecoder(*lpc_order, *lpc_order, InstCtxt->PHI_Priv);
     }
   }
}

/*======================================================================*/
/*   Function Definition: celp_close_decoder                            */
/*======================================================================*/
void celp_close_decoder
(
   long ExcitationMode,
   long SampleRateMode,
   long BandwidthScalabilityMode,
   long frame_bit_allocation[],         /* In: bit num. for each index */
   void **InstanceContext               /* In/Out: handle to instance context */
)
{
  INST_CONTEXT_LPC_DEC_TYPE *InstCtxt;
  PHI_PRIV_TYPE *PHI_Priv;
  
  /* -----------------------------------------------------------------*/
  /* Set up pointers to private data                                  */
  /* -----------------------------------------------------------------*/
  PHI_Priv = ((INST_CONTEXT_LPC_DEC_TYPE *) *InstanceContext)->PHI_Priv;

  /* -----------------------------------------------------------------*/
  /*                                                                  */
  /* -----------------------------------------------------------------*/
 

  if (ExcitationMode == RegularPulseExc)
  {
  	PHI_ClosePostProcessor(PHI_Priv);
    	PHI_close_excitation_generation(PHI_Priv);
    	PHI_FreeLpcAnalysisDecoder(PHI_Priv);
     	PHI_free_bit_allocation(frame_bit_allocation);
  }
	
  if (ExcitationMode == MultiPulseExc)
  {
    if (prev_Qlsp_coefficients != NULL) {
      FREE(prev_Qlsp_coefficients);
      prev_Qlsp_coefficients = NULL;
    }

 	PHI_FreeLpcAnalysisDecoder(PHI_Priv);
	if (BandwidthScalabilityMode == ON)
	{
	  if (buf_Qlsp_coefficients_bws != NULL) {
	    FREE(buf_Qlsp_coefficients_bws);
	    buf_Qlsp_coefficients_bws = NULL;
	  }
	  if (prev_Qlsp_coefficients_bws != NULL) {
	    FREE(prev_Qlsp_coefficients_bws);
	    prev_Qlsp_coefficients_bws = NULL;
	  }
	}
  }
	
    /* -----------------------------------------------------------------*/
    /* Print Total Frames Processed                                     */
    /* -----------------------------------------------------------------*/ 
    if (CELPdecDebugLevel) {	/* HP 971120 */
      fprintf(stderr,"\n");
      fprintf(stderr,"Total Frames:  %ld \n", frame_ctr);
    }

    /* -----------------------------------------------------------------*/
    /* Dispose of private storage for instance context                  */
    /* -----------------------------------------------------------------*/
    InstCtxt = (INST_CONTEXT_LPC_DEC_TYPE *)*InstanceContext;
    if (InstCtxt->PHI_Priv != NULL) {
      FREE(InstCtxt->PHI_Priv);
      InstCtxt->PHI_Priv = NULL;
    }
    FREE(InstCtxt);
    *InstanceContext = NULL;

}

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 01-09-96 R. Taori  Initial Version                                   */
/* 18-09-96 R. Taori  Brought in line with MPEG-4 Interface             */
/* 05-05-98 R. Funken Brought in line with MPEG-4 FCD: 3 complexity levels */

