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
/*====================================================================*/
/*                                                                    */
/*      INCLUDE_FILE:   PHI_BS2P.H                                    */
/*      PACKAGE:        WDBxx                                         */
/*      COMPONENT:      Bitstream to Parameter converter              */
/*                                                                    */
/*====================================================================*/
/*======================================================================*/
/*      G L O B A L  F U N C T I O N  P R O T O T Y P E S               */
/*======================================================================*/

#ifndef _celp_bitstream_demux_h_
#define _celp_bitstream_demux_h_


#ifdef __cplusplus
extern "C" {
#endif

/*======================================================================*/
/* Function Prototype: read_celp_bitstream_header                       */
/*======================================================================*/
void read_celp_bitstream_header(
    BsBitStream *hdrStream,           /* In: Bitstream                     */

    long * const ExcitationMode, 	 /* In: Excitation Mode 		*/
    long * const SampleRateMode, 	 /* In: SampleRate Mode 		*/
    long * const QuantizationMode,	 /* In: Type of Quantization		*/
    long * const FineRateControl,	 /* In: Fine Rate Control switch	*/
    long * const LosslessCodingMode,	 /* In: Lossless Coding Mode		*/
    long * const RPE_configuration,	 /* In: RPE_configuration  		*/
    long * const Wideband_VQ,		 /* In: Wideband VQ mode		*/
    long * const NB_Configuration,	 /* In: Narrowband configuration	*/
    long * const NumEnhLayers,		 /* In: Number of Enhancement Layers	*/
    long * const BandwidthScalabilityMode, /* In: bandwidth switch		*/
    long * const BWS_Configuration	 /* In: BWS_configuration		*/
    );

/*======================================================================*/
/* Function Prototype: Read_LosslessCoded_LPC                           */
/*======================================================================*/
void Read_LosslessCoded_LPC
(
    BsBitStream * p_bitstream,           /* In: Bitstream                 */
	const long lpc_order,              /* In: Order of LPC              */
	      long lpc_indices[]           /* Out: indices to rfc_table[]   */
);

/*======================================================================*/
/* Function Prototype: Read_WidebandPacked_LPC                          */
/*======================================================================*/
void Read_WidebandPacked_LPC
(
    BsBitStream * p_bitstream,           /* In: Bitstream                 */
	const long num_lpc_indices,        /* In: Number of LPC indices     */
	      long indices[]               /* Out: indices to rfc_table[]   */
);

/*======================================================================*/
/* Function Prototype: Read_NarrowbandPacked_LPC                        */
/*======================================================================*/
void Read_NarrowbandPacked_LPC
(
    BsBitStream * p_bitstream,           /* In: Bitstream                 */
	      long indices[]              /* Out: indices to rfc_table[]   */
);

/*======================================================================*/
/* Function Prototype : Read_Narrowband_LSP                             */
/*======================================================================*/
void Read_NarrowBand_LSP
(
    BsBitStream *bitStream,           /* In: Bitstream                 */
    unsigned long indices[]           /* Out: indices to rfc_table[]   */
);

/*======================================================================*/
/* Function Prototype : Read_BandScalable_LSP                           */
/*======================================================================*/
void Read_BandScalable_LSP
(
    BsBitStream *bitStream,           /* In: Bitstream                 */
    unsigned long indices[]           /* Out: indices to rfc_table[]   */
);

/*======================================================================*/
/* Function Prototype: Read_Wideband_LSP                                */
/*======================================================================*/
void Read_Wideband_LSP
(
    BsBitStream *bitStream,           /* In: Bitstream                 */
    unsigned long indices[]           /* Out: indices to rfc_table[]   */
);


#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _celp_bitstream_demux_h_ */
