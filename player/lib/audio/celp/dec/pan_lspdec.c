/*
This software module was originally developed by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)

and edited by

Yuji Maeda (Sony Corporation)

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
/*----------------------------------------------------------------------*
 *    MPEG-4 Audio Verification Model (VM)                              * 
 *                                                                      *
 *	CELP based coder                                                *
 * 	  Module: pan_lspdec.c                                          *
 *                                                                      *
 *  Last modified: Jan. 07, 1998                                        *
 *----------------------------------------------------------------------*/

#include	<stdio.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "pan_celp_proto.h"
#include "pan_celp_const.h"

/* ------------------------------------------------------------------ */

void pan_lspdec(
    float out_p[], /* in: previous output LSPs */
    float out[], /* out: output LSPs */
    float p_factor, /* in: prediction factor */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    unsigned long idx[], /* in: LSP indicies */
    float tbl[], /* in: VQ table */
    float d_tbl[], /* in: DVQ table */
    float rd_tbl[], /* in: PVQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long dim_2[], /* in: dimensions of the 2nd VQ vectors */
    long ncd_2[], /* in: numbers od the 2nd VQ codes */
    long flagStab, /* in: 0 - stabilization OFF, 1 - stabilization ON */
    long flagPred /* in: 0 - LSP prediction OFF, 1 - LSP prediction ON */
    )
{
	long	i;

/* 1st stage quantized value */

	for(i=0;i<dim_1[0];i++) {
		out[i] = tbl[dim_1[0]*idx[0]+i];
	}

	for(i=0;i<dim_1[1];i++) {
		out[dim_1[0]+i] 
			= tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx[1]+i];
	}

/* 2nd stage quantized value */

	if(idx[4]==0) {

		if(idx[2]<ncd_2[0]) {
		    for(i=0;i<dim_2[0];i++) {
			out[i] = out[i]+d_tbl[idx[2]*dim_2[0]+i];
		    }
		}else {
		    for(i=0;i<dim_2[0];i++) {
			out[i] = out[i]-d_tbl[(idx[2]-ncd_2[0])*dim_2[0]+i];
		    }
		}
		if(idx[3]<ncd_2[1]) {
		    for(i=0;i<dim_2[1];i++) {
			out[dim_2[0]+i] = out[dim_2[0]+i]
				+ d_tbl[dim_2[0]*ncd_2[0]+idx[3]*dim_2[1]+i];
		    }
		}else {
		    for(i=0;i<dim_2[1];i++) {
			out[dim_2[0]+i] = out[dim_2[0]+i]
				- d_tbl[dim_2[0]*ncd_2[0]
					+ (idx[3]-ncd_2[1])*dim_2[1]+i];
		    }
		}

	}else if(idx[4]==1) {

		if(flagPred != 0) {

		    if(idx[2]<ncd_2[0]) {
		        for(i=0;i<dim_2[0];i++) {
			    out[i] = (p_factor*out_p[i]+(1.-p_factor)*out[i])
			      + rd_tbl[idx[2]*dim_2[0]+i];
			}
		    }else {
		        for(i=0;i<dim_2[0];i++) {
			    out[i] = (p_factor*out_p[i]+(1.-p_factor)*out[i])
			      - rd_tbl[(idx[2]-ncd_2[0])*dim_2[0]+i];
			}
		    }
		    if(idx[3]<ncd_2[1]) {
		        for(i=0;i<dim_2[1];i++) {
			    out[dim_2[0]+i] = (p_factor*out_p[dim_2[0]+i]
					       + (1.-p_factor)*out[dim_2[0]+i])
			      + rd_tbl[dim_2[0]*ncd_2[0]
				      + idx[3]*dim_2[1]+i];
			}
		    }else {
		        for(i=0;i<dim_2[1];i++) {
			    out[dim_2[0]+i] = (p_factor*out_p[dim_2[0]+i]
					       + (1.-p_factor)*out[dim_2[0]+i])
			      - rd_tbl[dim_2[0]*ncd_2[0]
				      +(idx[3]-ncd_2[1])*dim_2[1]+i];
			}
		    }
		}
		
	}

	if(flagStab) pan_stab(out, min_gap, lpc_order);
}


