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
/*      SOURCE_FILE:    CELP_DECODER.H                                  */
/*      PACKAGE:        MPEG-4 CELP decoder                             */
/*                                                                      */
/*======================================================================*/

#ifndef _celp_decoder_h_
#define _celp_decoder_h_

#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: celp_decoder                                     */
/*======================================================================*/
void celp_decoder
(
BsBitStream *bitStream,           /* In: Bitstream                     */
float     **OutputSignal,          /* Out: Multichannel Output          */
long        ExcitationMode,         /* In: Excitation Mode */
long        SampleRateMode,	       /* In: SampleRate Mode		    	*/
long        QuantizationMode,	   /* In: Type of Quantization	        */
long        FineRateControl,	   /* In: Fine Rate Control switch   	*/
long        LosslessCodingMode,    /* In: Lossless Coding Mode	        */
long        RPE_configuration,	   /* In: Wideband configuration        */
long        Wideband_VQ,		   /* In: Wideband VQ mode		        */
long        NB_Configuration,	   /* In: Narrowband configuration	    */
long        NumEnhLayers,		   /* In: Number of Enhancement Layers  */
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
void        *InstanceContext	   /* In/Out: instance context */
);

/*======================================================================*/
/*   Function Prototype: celp_initialisation_decoder                    */
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
long     * ExcitationMode,	         /* Out: Excitation Mode */
long     * SampleRateMode,               /* Out: SampleRate Mode	    */
long     * QuantizationMode,             /* Out: Type of Quantization	*/
long     * FineRateControl,              /* Out: Fine Rate Control switch*/
long     * LosslessCodingMode,           /* Out: Lossless Coding Mode  	*/
long     * RPE_configuration,             /* Out: RPE_configuration */
long     * Wideband_VQ, 	             /* Out: Wideband VQ mode		*/
long     * NB_Configuration,             /* Out: Narrowband configuration*/
long     * NumEnhLayers,	             /* Out: Number of Enhancement Layers*/
long     * BandwidthScalabilityMode,     /* Out: bandwidth switch	    */
long     * BWS_configuration,            /* Out: BWS_configuration		*/
void	 **InstanceContext,		 /* Out: handle to initialised instance context */
int      mp4ffFlag
);

/*======================================================================*/
/*   Function Prototype: celp_close_decoder                             */
/*======================================================================*/
void celp_close_decoder
(
   long ExcitationMode,
   long SampleRateMode,
   long BandwidthScalabilityMode,
   long frame_bit_allocation[],         /* In: bit num. for each index */
   void **InstanceContext               /* In/Out: handle to instance context */
);


#ifdef __cplusplus
}
#endif

#endif 

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 10-11-97 A. Gerrits  Initial Version                                 */

