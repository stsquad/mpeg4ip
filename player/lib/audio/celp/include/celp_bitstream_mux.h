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
/*      INCLUDE_FILE:   CELP_BITSTREAM_MUX.H                          */
/*      PACKAGE:        Wdbxx                                         */
/*      COMPONENT:      Parameter to Bitstream converter              */
/*                                                                    */
/*====================================================================*/

/*======================================================================*/
/*      G L O B A L  F U N C T I O N  P R O T O T Y P E S               */
/*======================================================================*/

#ifndef _celp_bitstream_mux_h_
#define _celp_bitstream_mux_h_


#ifdef __cplusplus
extern "C" {
#endif
/*======================================================================*/
/* Function Prototype : write_celp_bitstream_header                     */
/*======================================================================*/
void write_celp_bitstream_header(
    BsBitStream *hdrStream,          /* Out: Bitstream                  */
    const long ExcitationMode,       /* In: Excitation Mode             */
    const long SampleRateMode,       /* In: SampleRate Mode             */
    const long QuantizationMode,     /* In: Type of Quantization        */
    const long FineRateControl,      /* In: Fine Rate Control switch    */
    const long LosslessCodingMode,   /* In: Lossless Coding Mode        */   
    const long RPE_configuration,    /* In: RPE_configuration           */
    const long Wideband_VQ,          /* In: Wideband VQ mode            */
    const long NB_Configuration,     /* In: Narrowband configuration    */
    const long NumEnhLayers,         /* In: Number of Enhancement Layers*/
    const long BandwidthScalabilityMode, /* In: bandwidth switch        */
    const long BWS_Configuration     /* In: BWS_configuration           */
);

/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP                            */
/*======================================================================*/
void Write_NarrowBand_LSP
(
    BsBitStream *bitStream,            /* Out: Bitstream                */
	const long indices[]               /* In: indices                   */
);
/*======================================================================*/
/* Function Definition: Write_Narrowband_LSP                            */
/*======================================================================*/
void Write_BandScalable_LSP
(
    BsBitStream *bitStream,            /* Out: Bitstream                */
	   const long indices[]            /* In: indices                   */
);

/*======================================================================*/
/* Function Definition: Write_Wideband_LSP                              */
/*======================================================================*/
void Write_Wideband_LSP
(
    BsBitStream *bitStream,            /* Out: Bitstream                */
	const long indices[]               /* In: indices                   */
);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _celp_bitstream_mux_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 07-06-96  R. Taori & A.Gerrits    Initial Version                    */
/* 18-09-96  R. Taori & A.Gerrits    MPEG bitstream used                */

