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
/*      SOURCE_FILE:    PHI_COD.H                                       */
/*      PACKAGE:        WDBXX_1.0                                       */
/*      COMPONENT:      Analysis-by-Synthesis CELP Framework (CODER)    */  
/*                                                                      */
/*======================================================================*/
#ifndef _phi_cod_h_
#define _phi_cod_h_

#ifdef __cplusplus
extern "C" {
#endif
/**
#include "lpc_common.h"
**/     
/*======================================================================*/
/* Function prototype: celp_coder                                       */
/*======================================================================*/
void celp_coder
(
float     **InputSignal,           /* In: Multichannel Speech           */
BsBitStream *bitStream,           /* Out: Bitstream                     */
long        sampling_frequency,    /* In:  Sampling Frequency           */
long        bit_rate,              /* In:  Bit rate                     */
long     ExcitationMode,	     /* In: Excitation Mode 		*/
long        SampleRateMode,
long        QuantizationMode,      /* In: Type of Quantization  	    */
long        FineRateControl,	   /* In: Fine Rate Control switch      */
long        LosslessCodingMode,    /* In: Lossless Coding Mode  	    */   
long        RPE_configuration,      /* In: Wideband configuration 	    */
long        Wideband_VQ,		   /* In: Wideband VQ mode			    */
long        MPE_Configuration,      /* In: Narrowband configuration      */
long        NumEnhLayers,  	       /* In: Number of Enhancement Layers  */
long        BandwidthScalabilityMode, /* In: bandwidth switch           */
long        BWS_configuration,     /* In: BWS_configuration 		    */
long        PreProcessingSW,       /* In: PreProcessingSW	    */
long        frame_size,            /* In:  Frame size                   */
long        n_subframes,           /* In:  Number of subframes          */
long        sbfrm_size,            /* In:  Subframe size                */
long        lpc_order,             /* In:  Order of LPc                 */
long        num_lpc_indices,       /* In:  Number of LPC indices        */
long        num_shape_cbks,        /* In:  Number of Shape Codebooks    */
long        num_gain_cbks,         /* In:  Number of Gain Codebooks     */
long        n_lpc_analysis,        /* In:  Number of LPCs per frame     */
long        window_offsets[],      /* In:  Offset for LPC-frame v.window*/
long        window_sizes[],        /* In:  LPC Analysis Window size     */
long        max_n_lag_candidates,  /* In:  Maximum search candidates    */
float       min_pitch_frequency,   /* IN:  Min Pitch Frequency          */
float       max_pitch_frequency,   /* IN:  Max Pitch Frequency          */
long        org_frame_bit_allocation[], /* In: Frame BIt alolocation      */
void        *InstanceContext	   /* In/Out: instance context */
);

/*======================================================================*/
/*   Function Prototype:celp_initialisation_encoder                     */
/*======================================================================*/
void celp_initialisation_encoder
(
BsBitStream *hdrStream,           /* Out: Bitstream                     */
long	 bit_rate,  	         /* In: bit rate                        */
long	 sampling_frequency,     /* In: sampling frequency              */
long     ExcitationMode,	     /* In: Excitation Mode 		*/
long     SampleRateMode,	     /* In: SampleRate Mode 		*/
long     QuantizationMode,       /* In: Type of Quantization		*/
long     FineRateControl,	     /* In: Fine Rate Control switch	 */
long     LosslessCodingMode,     /* In: Lossless Coding Mode	*/
long     *RPE_configuration,      /* In: RPE_configuration 	*/
long     Wideband_VQ,		     /* Out: Wideband VQ mode			*/
long     *MPE_Configuration,      /* Out: Multi-Pulse Exc. configuration   */
long     NumEnhLayers,  	 /* In: Number of Enhancement Layers for NB */
long     BandwidthScalabilityMode, /* In: bandwidth switch   */
long     *BWS_Configuration,     /* Out: BWS_configuration 		*/
long     BWS_nb_bitrate,  	     /* In: narrowband bitrate for BWS */
long     InputConfiguration,     /* In: RPE/MPE Configuration for FRC   */
long	 *frame_size,	         /* Out: frame size                     */
long	 *n_subframes,           /* Out: number of  subframes           */
long	 *sbfrm_size,	         /* Out: subframe size                  */ 
long	 *lpc_order,	         /* Out: LP analysis order              */
long	 *num_lpc_indices,       /* Out: number of LPC indices          */
long	 *num_shape_cbks,	     /* Out: number of Shape Codebooks      */
long	 *num_gain_cbks,	     /* Out: number of Gain Codebooks       */ 
long	 *n_lpc_analysis,	     /* Out: number of LP analysis per frame*/
long	 **window_offsets,       /* Out: window offset for each LP ana  */
long	 **window_sizes,         /* Out: window size for each LP ana    */
long	 *n_lag_candidates,      /* Out: number of pitch candidates     */
float	 *min_pitch_frequency,   /* Out: minimum pitch frequency        */
float	 *max_pitch_frequency,   /* Out: maximum pitch frequency        */
long	 **org_frame_bit_allocation, /* Out: bit num. for each index      */
void	 **InstanceContext,	 /* Out: handle to initialised instance context */
int      sysFlag                 /* In: system interface(flexmux) flag */
);

/*======================================================================*/
/*   Function  Prototype: celp_close_encoder                            */
/*======================================================================*/
void celp_close_encoder
(
    long ExcitationMode,	     /* In: Excitation Mode 		*/
    long SampleRateMode,
    long BandwidthScalabilityMode,
    long sbfrm_size,              /* In: subframe size                  */
    long frame_bit_allocation[],  /* In: bit num. for each index        */
    long window_offsets[],        /* In: window offset for each LP ana  */
    long window_sizes[],          /* In: window size for each LP ana    */
    long n_lpc_analysis,          /* In: number of LP analysis/frame    */
    void **InstanceContext	  /* In/Out: handle to instance context */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_cod_h_*/
