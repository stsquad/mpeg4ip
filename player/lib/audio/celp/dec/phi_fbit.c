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
/*      INCLUDE_FILE:   PHI_FBIT.C                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Frame bit allocation table                      */
/*                                                                      */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "phi_fbit.h"
#include "phi_cons.h"
#include "bitstream.h"
#include "lpc_common.h"
#include "nec_abs_const.h"
#include "pan_celp_const.h"

/*======================================================================*/
/* Function Definition: PHI_init_bit_allocation                         */
/*======================================================================*/

long  * PHI_init_bit_allocation(
    const long SampleRateMode,        /* In: SampleRate Mode           */
    const long RPE_configuration,     /* In: Bit Rate Configuration    */
    const long QuantizationMode,      /* In: Type of Quantization      */
    const long LosslessCodingMode,    /* In: Lossless Coding Mode      */   
    const long FineRateControl,       /* In: FRC flag                  */
    const long num_lpc_indices,       /* In: Number of LPC indices     */  
    const long n_subframes,           /* In: Number of subframes       */
    const long num_shape_cbks,        /* In: Number of Shape Codebooks */
    const long num_gain_cbks          /* In: Number of Gain Codebooks  */
)
{
    long *frame_bit_allocation;
    long allocation_length = 2 + num_lpc_indices + (num_shape_cbks + num_gain_cbks) *n_subframes;
    long k;
    long index = 0;
    
    /* -----------------------------------------------------------------*/
    /* Create bit allocation                                            */
    /* -----------------------------------------------------------------*/
    if
    (
     (( frame_bit_allocation = (long *)malloc((unsigned int)allocation_length * sizeof(long))) == NULL ) 
    )
    {
        fprintf(stderr, "MALLOC FAILURE in PHI_init_bit_allocation\n");
        exit(1);
    }

    /* -----------------------------------------------------------------*/
    /* Allocate header                                                  */
    /* -----------------------------------------------------------------*/
    if (FineRateControl == ON)
    {
	frame_bit_allocation[index++] = 1;   /* Interpolation flag */
	frame_bit_allocation[index++] = 1;   /* send_lpc_flag */
    }
    else
    {
	frame_bit_allocation[index++] = 0;   /* Interpolation flag */
	frame_bit_allocation[index++] = 0;   /* send_lpc_flag */
    }
    
    /* -----------------------------------------------------------------*/
    /* Allocate LPC_codes                                               */
    /* -----------------------------------------------------------------*/
    if (SampleRateMode == fs8kHz)
    {
	frame_bit_allocation[index++] = PAN_BIT_LSP22_0;
	frame_bit_allocation[index++] = PAN_BIT_LSP22_1;
	frame_bit_allocation[index++] = PAN_BIT_LSP22_2;
	frame_bit_allocation[index++] = PAN_BIT_LSP22_3;
	frame_bit_allocation[index++] = PAN_BIT_LSP22_4;	    	    	    	    
    }
    if (SampleRateMode == fs16kHz)
    {
	frame_bit_allocation[index++] = PAN_BIT_LSP_WL_0;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WL_1;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WL_2;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WL_3;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WL_4;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WU_0;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WU_1;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WU_2;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WU_3;
	frame_bit_allocation[index++] = PAN_BIT_LSP_WU_4;
    }

    /* -----------------------------------------------------------------*/
    /* Allocate Excitation parameters                                   */
    /* -----------------------------------------------------------------*/
    /* -----------------------------------------------------------------*/
    /* Allocate Anchor                                                  */
    /* -----------------------------------------------------------------*/
    if ((RPE_configuration == 0) || (RPE_configuration == 1))
    {
    	frame_bit_allocation[index++] = 8;
    	frame_bit_allocation[index++] = 11;
    	frame_bit_allocation[index++] = 6;
    	frame_bit_allocation[index++] = 5;
    }
    
    if ((RPE_configuration == 2) || (RPE_configuration == 3))
    {
    	frame_bit_allocation[index++] = 8;
    	frame_bit_allocation[index++] = 12;
    	frame_bit_allocation[index++] = 6;
    	frame_bit_allocation[index++] = 5;
    }
    
    /* -----------------------------------------------------------------*/
    /* Allocate Update                                                  */
    /* -----------------------------------------------------------------*/

    for (k = 1; k < n_subframes; k++)
    {
	if ((RPE_configuration == 0) || (RPE_configuration == 1))
    	{
    		frame_bit_allocation[index++] = 8;
	    	frame_bit_allocation[index++] = 11;
    		frame_bit_allocation[index++] = 6;
	    	frame_bit_allocation[index++] = 3;
	}

	if ((RPE_configuration == 2) || (RPE_configuration == 3))
	{
		frame_bit_allocation[index++] = 8;
		frame_bit_allocation[index++] = 12;
		frame_bit_allocation[index++] = 6;
		frame_bit_allocation[index++] = 3;
	}
    }
    /* -----------------------------------------------------------------*/
    /* Check if allocation table has the correct length                 */
    /* -----------------------------------------------------------------*/
    if (index != allocation_length)
    {
        fprintf(stderr, "Unable to create the correct allocation bit map %ld %ld\n", index, allocation_length);
        exit(0);
    }
        
    return (frame_bit_allocation);
}

/*======================================================================*/
/* Function Definition: PHI_free_bit_allocation                         */
/*======================================================================*/
void PHI_free_bit_allocation(
    long  *frame_bit_allocation
)
{
    free(frame_bit_allocation);
}
