/*
This software module was originally developed by
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
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
/*----------------------------------------------------------------------*
 *  Panasonic                                                           *
 *    MPEG-4 Audio Verification Model (VM)                              * 
 *                                                                      *
 *	CELP based coder                                                *
 * 	  Module: pan_lspqtz2_dd.c                                      *
 *                                                                      *
 *  Last modified: Jan. 06, 1998                                        *
 *----------------------------------------------------------------------*/

/* 2-mode, 2-stage LSP vector quantizer w/ delayed decision */
/*  - This module supports (1+2) and (2+2) structures only! */

#include	<stdio.h>
#include	<stdlib.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "pan_celp_proto.h"
#include "pan_celp_const.h"

void pan_lspqtz2_dd(
    float in[], /* in: input LSPs */
    float out_p[], /* in: previous output LSPs */
    float out[], /* out: output LSPs */
    float weight[], /* in: weighting factor */
    float p_factor, /* in: prediction coefficient */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    long num_dc, /* in: number of delayed candidates for DD */
    long idx[],  /* out: LSP indicies */
    float tbl[], /* in: VQ table */
    float d_tbl[], /* in: DVQ table */
    float rd_tbl[], /* in: PVQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long dim_2[], /* in: dimensions of the 2nd VQ vectors */
    long ncd_2[], /* in: numbers of the 2nd VQ codes */
    long flagStab /* in: 0 - stabilization OFF, 1 - stabilization ON */
    )
{
    float out_v[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float qv_rd[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float qv_d[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float dist_rd1, dist_d1;
    float dist_rd2, dist_d2;
    float dist_rd_min1, dist_d_min1;
    float dist_rd_min2, dist_d_min2;
    long i, j;
    long d_idx[2*PAN_N_DC_LSP_MAX];
    long rd_idx[2*PAN_N_DC_LSP_MAX];
    long d_idx_tmp1, rd_idx_tmp1;
    long d_idx_tmp2, rd_idx_tmp2;

    long idx_tmp[2*PAN_N_DC_LSP_MAX];
    long i_tmp[PAN_N_DC_LSP_MAX];

    if(lpc_order>PAN_DIM_MAX) {
        printf("\n LSP quantizer: LPC order exceeds the limit\n");
        exit(1);
    }
    if(num_dc>PAN_N_DC_LSP_MAX) {
        printf("\n LSP quantizer: number of delayed candidates exceeds the limit\n");
        exit(1);
    }

/* 1st stage vector quantization */
	pan_v_qtz_w_dd(&in[0], i_tmp, ncd_1[0], &tbl[0], dim_1[0], 
		&weight[0], num_dc);
	for(i=0;i<num_dc;i++) idx_tmp[2*i] = i_tmp[i];

	pan_v_qtz_w_dd(&in[dim_1[0]], i_tmp, ncd_1[1], 
		&tbl[dim_1[0]*ncd_1[0]], dim_1[1], &weight[dim_1[0]], num_dc);
	for(i=0;i<num_dc;i++) idx_tmp[2*i+1] = i_tmp[i];
 
/* quantized value */
for(j=0;j<num_dc;j++) {
	for(i=0;i<dim_1[0];i++) {
		out_v[lpc_order*j+i] = tbl[dim_1[0]*idx_tmp[2*j]+i];
	}

	for(i=0;i<dim_1[1];i++) {
		out_v[lpc_order*j+dim_1[0]+i] 
			= tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx_tmp[2*j+1]+i];
	}
}

/* ------------------------------------------------------------------ */
/* 2nd stage vector quantization */

for(j=0;j<num_dc;j++) {	
	pan_d_qtz_w(&in[0], &out_v[lpc_order*j], &d_idx[2*j], ncd_2[0], 
		&d_tbl[0], dim_2[0], &weight[0]);
	
	pan_d_qtz_w(&in[dim_2[0]], &out_v[lpc_order*j+dim_2[0]], &d_idx[2*j+1], 
		ncd_2[1], &d_tbl[dim_2[0]*ncd_2[0]], 
		dim_2[1], &weight[dim_2[0]]);

/* quantized value */

	if(d_idx[2*j]<ncd_2[0]) {
	    for(i=0;i<dim_2[0];i++) {
		qv_d[lpc_order*j+i] = out_v[lpc_order*j+i]+d_tbl[d_idx[2*j]*dim_2[0]+i];
	    }
	}else {
	    for(i=0;i<dim_2[0];i++) {
		qv_d[lpc_order*j+i] = out_v[lpc_order*j+i]-d_tbl[(d_idx[2*j]-ncd_2[0])*dim_2[0]+i];
	    }
	}
	if(d_idx[2*j+1]<ncd_2[1]) {
	    for(i=0;i<dim_2[1];i++) {
		qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i]
			+ d_tbl[dim_2[0]*ncd_2[0]+d_idx[2*j+1]*dim_2[1]+i];
	    }
	}else {
	    for(i=0;i<dim_2[1];i++) {
		qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i]
			- d_tbl[dim_2[0]*ncd_2[0]
				+ (d_idx[2*j+1]-ncd_2[1])*dim_2[1]+i];
	    }
	}

}
/* ------------------------------------------------------------------ */
/* 2nd stage predictive vector quantization */
for(j=0;j<num_dc;j++) {

	pan_rd_qtz2_w(&in[0], &out_p[0], &out_v[lpc_order*j], &rd_idx[2*j], 
		ncd_2[0], &rd_tbl[0], dim_2[0], &weight[0], p_factor);
	
	pan_rd_qtz2_w(&in[dim_2[0]], &out_p[dim_2[0]], &out_v[lpc_order*j+dim_2[0]], 
		&rd_idx[2*j+1], ncd_2[1], &rd_tbl[dim_2[0]*ncd_2[0]], 
		dim_2[1], &weight[dim_2[0]], p_factor);
	
/* quantized value */

	if(rd_idx[2*j]<ncd_2[0]) {
	    for(i=0;i<dim_2[0];i++) {
		qv_rd[lpc_order*j+i] = (p_factor*out_p[i]
			+ (1.-p_factor)*out_v[lpc_order*j+i])
			+ rd_tbl[rd_idx[2*j]*dim_2[0]+i];
	    }
	}else {
	    for(i=0;i<dim_2[0];i++) {
		qv_rd[lpc_order*j+i] = (p_factor*out_p[i]
			+ (1.-p_factor)*out_v[lpc_order*j+i])
			- rd_tbl[(rd_idx[2*j]-ncd_2[0])*dim_2[0]+i];
	    }
	}
	if(rd_idx[2*j+1]<ncd_2[1]) {
	    for(i=0;i<dim_2[1];i++) {
		qv_rd[lpc_order*j+dim_2[0]+i] = (p_factor*out_p[dim_2[0]+i]
				+ (1.-p_factor)*out_v[lpc_order*j+dim_2[0]+i])
				+ rd_tbl[dim_2[0]*ncd_2[0]
					+ rd_idx[2*j+1]*dim_2[1]+i];
	    }
	}else {
	    for(i=0;i<dim_2[1];i++) {
		qv_rd[lpc_order*j+dim_2[0]+i] = (p_factor*out_p[dim_2[0]+i]
				+ (1.-p_factor)*out_v[lpc_order*j+dim_2[0]+i])
				- rd_tbl[dim_2[0]*ncd_2[0]
					+(rd_idx[2*j+1]-ncd_2[1])*dim_2[1]+i];
	    }
	}

}
/* ------------------------------------------------------------------ */
/* Quantization mode selection */
if(0==dim_1[1]) {
/* PVQ */
	dist_rd_min1 = 1.0e9;
	dist_rd_min2 = 1.0e9;
	rd_idx_tmp1 = 0;
	rd_idx_tmp2 = 0;
	for(j=0;j<num_dc;j++) {
		dist_rd1 = 0.;
		for(i=0;i<dim_2[0];i++) 
			dist_rd1 += (in[i]-qv_rd[lpc_order*j+i])
				*(in[i]-qv_rd[lpc_order*j+i])*weight[i];
		dist_rd2 = 0.;
		for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++) 
			dist_rd2 += (in[i]-qv_rd[lpc_order*j+i])
				*(in[i]-qv_rd[lpc_order*j+i])*weight[i];
		if(dist_rd1+dist_rd2<dist_rd_min1+dist_rd_min2) {
			dist_rd_min1 = dist_rd1;
			dist_rd_min2 = dist_rd2;
			rd_idx_tmp1 = j;
			rd_idx_tmp2 = j;
		}
	}

/* DVQ */
	dist_d_min1 = 1.0e9;
	dist_d_min2 = 1.0e9;
	d_idx_tmp1 = 0;
	d_idx_tmp2 = 0;
	for(j=0;j<num_dc;j++) {
		dist_d1 = 0.;
		for(i=0;i<dim_2[0];i++)
			dist_d1 += (in[i]-qv_d[lpc_order*j+i])
				*(in[i]-qv_d[lpc_order*j+i])*weight[i];
		dist_d2 = 0.;
		for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
			dist_d2 += (in[i]-qv_d[lpc_order*j+i])
				*(in[i]-qv_d[lpc_order*j+i])*weight[i];
		if(dist_d1+dist_d2<dist_d_min1+dist_d_min2) {
			dist_d_min1 = dist_d1;
			dist_d_min2 = dist_d2;
			d_idx_tmp1 = j;
			d_idx_tmp2 = j;
		}
	}


}else {
/* Lower Part of PVQ */
	dist_rd_min1 = 1.0e9;
	rd_idx_tmp1 = 0;
	for(j=0;j<num_dc;j++) {
		dist_rd1 = 0.;
		for(i=0;i<dim_2[0];i++) 
			dist_rd1 += (in[i]-qv_rd[lpc_order*j+i])
				*(in[i]-qv_rd[lpc_order*j+i])*weight[i];
		if(dist_rd1<dist_rd_min1) {
			dist_rd_min1 = dist_rd1;
			rd_idx_tmp1 = j;
		}
	}

/* Upper Part of PVQ */
	dist_rd_min2 = 1.0e9;
	rd_idx_tmp2 = 0;
	for(j=0;j<num_dc;j++) {
		dist_rd2 = 0.;
		for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++) 
			dist_rd2 += (in[i]-qv_rd[lpc_order*j+i])
				*(in[i]-qv_rd[lpc_order*j+i])*weight[i];
		if(dist_rd2<dist_rd_min2) {
			dist_rd_min2 = dist_rd2;
			rd_idx_tmp2 = j;
		}
	}

/* Lower Part of DVQ */
	dist_d_min1 = 1.0e9;
	d_idx_tmp1 = 0;
	for(j=0;j<num_dc;j++) {
		dist_d1 = 0.;
		for(i=0;i<dim_2[0];i++)
			dist_d1 += (in[i]-qv_d[lpc_order*j+i])
				*(in[i]-qv_d[lpc_order*j+i])*weight[i];
		if(dist_d1<dist_d_min1) {
			dist_d_min1 = dist_d1;
			d_idx_tmp1 = j;
		}
	}

/* Upper Part of DVQ */
	dist_d_min2 = 1.0e9;
	d_idx_tmp2 = 0;
	for(j=0;j<num_dc;j++) {
		dist_d2 = 0.;
		for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
			dist_d2 += (in[i]-qv_d[lpc_order*j+i])
				*(in[i]-qv_d[lpc_order*j+i])*weight[i];
		if(dist_d2<dist_d_min2) {
			dist_d_min2 = dist_d2;
			d_idx_tmp2 = j;
		}
	}
}

	if((dist_rd_min1+dist_rd_min2)<(dist_d_min1+dist_d_min2)) {
		for(i=0;i<dim_2[0];i++) out[i] = qv_rd[lpc_order*rd_idx_tmp1+i];
		for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++) 
			out[i] = qv_rd[lpc_order*rd_idx_tmp2+i];
		idx[0] = idx_tmp[2*rd_idx_tmp1];
		idx[1] = idx_tmp[2*rd_idx_tmp2+1];
		idx[2] = rd_idx[2*rd_idx_tmp1];
		idx[3] = rd_idx[2*rd_idx_tmp2+1];
		idx[4] = 1;
	}else{
		for(i=0;i<dim_2[0];i++) out[i] = qv_d[lpc_order*d_idx_tmp1+i];
		for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++) 
			out[i] = qv_d[lpc_order*d_idx_tmp2+i];
		idx[0] = idx_tmp[2*d_idx_tmp1];
		idx[1] = idx_tmp[2*d_idx_tmp2+1];
		idx[2] = d_idx[2*d_idx_tmp1];
		idx[3] = d_idx[2*d_idx_tmp2+1];
		idx[4] = 0;
	}

	if(flagStab) pan_stab(out, min_gap, lpc_order);

}

/* ------------------------------------------------------------------ */

void pan_lspqtz2_ddVR(
    float in[], /* in: input LSPs */
    float out_p[], /* in: previous output LSPs */
    float out[], /* out: output LSPs */
    float weight[], /* in: weighting factor */
    float p_factor, /* in: prediction coefficient */
    float min_gap, /* in: minimum gap between adjacent LSPs */
    long lpc_order, /* in: LPC order */
    long num_dc, /* in: number of delayed candidates for DD */
    long idx[],  /* out: LSP indicies */
    float tbl[], /* in: VQ table */
    float d_tbl[], /* in: DVQ table */
    float rd_tbl[], /* in: PVQ table */
    long dim_1[], /* in: dimensions of the 1st VQ vectors */
    long ncd_1[], /* in: numbers of the 1st VQ codes */
    long dim_2[], /* in: dimensions of the 2nd VQ vectors */
    long ncd_2[],  /* in: numbers of the 2nd VQ codes */
    int level,
    int qMode
    )
{
    float out_v[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float qv_rd[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float qv_d[PAN_DIM_MAX*PAN_N_DC_LSP_MAX];
    float dist_rd1, dist_d1;
    float dist_rd2, dist_d2;
    float dist_rd_min1, dist_d_min1;
    float dist_rd_min2, dist_d_min2;
    long i, j;
    long d_idx[2*PAN_N_DC_LSP_MAX];
    long rd_idx[2*PAN_N_DC_LSP_MAX];
    long d_idx_tmp1, rd_idx_tmp1;
    long d_idx_tmp2, rd_idx_tmp2;

    long idx_tmp[2*PAN_N_DC_LSP_MAX];
    long i_tmp[PAN_N_DC_LSP_MAX];

    if(lpc_order>PAN_DIM_MAX) {
        printf("\n LSP quantizer: LPC order exceeds the limit\n");
        exit(1);
    }
    if(num_dc>PAN_N_DC_LSP_MAX) {
        printf("\n LSP quantizer: number of delayed candidates exceeds the limit\n");
        exit(1);
    }

/* 1st stage vector quantization */
        pan_v_qtz_w_dd(&in[0], i_tmp, ncd_1[0], &tbl[0], dim_1[0],
                &weight[0], num_dc);
        for(i=0;i<num_dc;i++) idx_tmp[2*i] = i_tmp[i];

        pan_v_qtz_w_dd(&in[dim_1[0]], i_tmp, ncd_1[1],
                &tbl[dim_1[0]*ncd_1[0]], dim_1[1], &weight[dim_1[0]], num_dc);
        for(i=0;i<num_dc;i++) idx_tmp[2*i+1] = i_tmp[i];

/* quantized value */
for(j=0;j<num_dc;j++) {
        for(i=0;i<dim_1[0];i++) {
                out_v[lpc_order*j+i] = tbl[dim_1[0]*idx_tmp[2*j]+i];
        }

        for(i=0;i<dim_1[1];i++) {
                out_v[lpc_order*j+dim_1[0]+i]
                        = tbl[dim_1[0]*ncd_1[0]+dim_1[1]*idx_tmp[2*j+1]+i];
        }
}

/* ------------------------------------------------------------------ */
/* 2nd stage vector quantization */

for(j=0;j<num_dc;j++) {
        pan_d_qtz_w(&in[0], &out_v[lpc_order*j], &d_idx[2*j], ncd_2[0],
                &d_tbl[0], dim_2[0], &weight[0]);

        pan_d_qtz_w(&in[dim_2[0]], &out_v[lpc_order*j+dim_2[0]], &d_idx[2*j+1],
                ncd_2[1], &d_tbl[dim_2[0]*ncd_2[0]],
                dim_2[1], &weight[dim_2[0]]);

/* quantized value */

        if(d_idx[2*j]<ncd_2[0]) {
            for(i=0;i<dim_2[0];i++) {
                qv_d[lpc_order*j+i] = out_v[lpc_order*j+i]+d_tbl[d_idx[2*j]*dim_2[0]+i];
            }
        }else {
            for(i=0;i<dim_2[0];i++) {
                qv_d[lpc_order*j+i] = out_v[lpc_order*j+i]-d_tbl[(d_idx[2*j]-ncd_2[0])*dim_2[0]+i];
            }
        }
        if(d_idx[2*j+1]<ncd_2[1]) {
            for(i=0;i<dim_2[1];i++) {
                qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i]
                        + d_tbl[dim_2[0]*ncd_2[0]+d_idx[2*j+1]*dim_2[1]+i];
            }
        }else {
            for(i=0;i<dim_2[1];i++) {
                qv_d[lpc_order*j+dim_2[0]+i] = out_v[lpc_order*j+dim_2[0]+i]
                        - d_tbl[dim_2[0]*ncd_2[0]
                                + (d_idx[2*j+1]-ncd_2[1])*dim_2[1]+i];
            }
        }

}
/* ------------------------------------------------------------------ */
/* 2nd stage predictive vector quantization */
for(j=0;j<num_dc;j++) {

        pan_rd_qtz2_w(&in[0], &out_p[0], &out_v[lpc_order*j], &rd_idx[2*j],
                ncd_2[0], &rd_tbl[0], dim_2[0], &weight[0], p_factor);

        pan_rd_qtz2_w(&in[dim_2[0]], &out_p[dim_2[0]], &out_v[lpc_order*j+dim_2[0]],
                &rd_idx[2*j+1], ncd_2[1], &rd_tbl[dim_2[0]*ncd_2[0]],
                dim_2[1], &weight[dim_2[0]], p_factor);

/* quantized value */

        if(rd_idx[2*j]<ncd_2[0]) {
            for(i=0;i<dim_2[0];i++) {
                qv_rd[lpc_order*j+i] = (p_factor*out_p[i]
                        + (1.-p_factor)*out_v[lpc_order*j+i])
                        + rd_tbl[rd_idx[2*j]*dim_2[0]+i];
            }
        }else {
            for(i=0;i<dim_2[0];i++) {
                qv_rd[lpc_order*j+i] = (p_factor*out_p[i]
                        + (1.-p_factor)*out_v[lpc_order*j+i])
                        - rd_tbl[(rd_idx[2*j]-ncd_2[0])*dim_2[0]+i];
            }
        }
        if(rd_idx[2*j+1]<ncd_2[1]) {
            for(i=0;i<dim_2[1];i++) {
                qv_rd[lpc_order*j+dim_2[0]+i] = (p_factor*out_p[dim_2[0]+i]
                                + (1.-p_factor)*out_v[lpc_order*j+dim_2[0]+i])
                                + rd_tbl[dim_2[0]*ncd_2[0]
                                        + rd_idx[2*j+1]*dim_2[1]+i];
            }
        }else {
            for(i=0;i<dim_2[1];i++) {
                qv_rd[lpc_order*j+dim_2[0]+i] = (p_factor*out_p[dim_2[0]+i]
                                + (1.-p_factor)*out_v[lpc_order*j+dim_2[0]+i])
                                - rd_tbl[dim_2[0]*ncd_2[0]
                                        +(rd_idx[2*j+1]-ncd_2[1])*dim_2[1]+i];
            }
        }

}
/* ------------------------------------------------------------------ */
/* Quantization mode selection */
if(0==dim_1[1]) {
/* PVQ */
        dist_rd_min1 = 1.0e9;
        dist_rd_min2 = 1.0e9;
        rd_idx_tmp1 = 0;
        rd_idx_tmp2 = 0;
        for(j=0;j<num_dc;j++) {
                dist_rd1 = 0.;
                for(i=0;i<dim_2[0];i++)
                        dist_rd1 += (in[i]-qv_rd[lpc_order*j+i])
                                *(in[i]-qv_rd[lpc_order*j+i])*weight[i];
                dist_rd2 = 0.;
                for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
                        dist_rd2 += (in[i]-qv_rd[lpc_order*j+i])
                                *(in[i]-qv_rd[lpc_order*j+i])*weight[i];
                if(dist_rd1+dist_rd2<dist_rd_min1+dist_rd_min2) {
                        dist_rd_min1 = dist_rd1;
                        dist_rd_min2 = dist_rd2;
                        rd_idx_tmp1 = j;
                        rd_idx_tmp2 = j;
                }
        }

/* DVQ */
        dist_d_min1 = 1.0e9;
        dist_d_min2 = 1.0e9;
        d_idx_tmp1 = 0;
        d_idx_tmp2 = 0;
        for(j=0;j<num_dc;j++) {
                dist_d1 = 0.;
                for(i=0;i<dim_2[0];i++)
                        dist_d1 += (in[i]-qv_d[lpc_order*j+i])
                                *(in[i]-qv_d[lpc_order*j+i])*weight[i];
                dist_d2 = 0.;
                for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
                        dist_d2 += (in[i]-qv_d[lpc_order*j+i])
                                *(in[i]-qv_d[lpc_order*j+i])*weight[i];
                if(dist_d1+dist_d2<dist_d_min1+dist_d_min2) {
                        dist_d_min1 = dist_d1;
                        dist_d_min2 = dist_d2;
                        d_idx_tmp1 = j;
                        d_idx_tmp2 = j;
                }
        }


}else {
/* Lower Part of PVQ */
        dist_rd_min1 = 1.0e9;
        rd_idx_tmp1 = 0;
        for(j=0;j<num_dc;j++) {
                dist_rd1 = 0.;
                for(i=0;i<dim_2[0];i++)
                        dist_rd1 += (in[i]-qv_rd[lpc_order*j+i])
                                *(in[i]-qv_rd[lpc_order*j+i])*weight[i];
                if(dist_rd1<dist_rd_min1) {
                        dist_rd_min1 = dist_rd1;
                        rd_idx_tmp1 = j;
                }
        }

/* Upper Part of PVQ */
        dist_rd_min2 = 1.0e9;
        rd_idx_tmp2 = 0;
        for(j=0;j<num_dc;j++) {
                dist_rd2 = 0.;
                for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
                        dist_rd2 += (in[i]-qv_rd[lpc_order*j+i])
                                *(in[i]-qv_rd[lpc_order*j+i])*weight[i];
                if(dist_rd2<dist_rd_min2) {
                        dist_rd_min2 = dist_rd2;
                        rd_idx_tmp2 = j;
                }
        }

/* Lower Part of DVQ */
        dist_d_min1 = 1.0e9;
        d_idx_tmp1 = 0;
        for(j=0;j<num_dc;j++) {
                dist_d1 = 0.;
                for(i=0;i<dim_2[0];i++)
                        dist_d1 += (in[i]-qv_d[lpc_order*j+i])
                                *(in[i]-qv_d[lpc_order*j+i])*weight[i];
                if(dist_d1<dist_d_min1) {
                        dist_d_min1 = dist_d1;
                        d_idx_tmp1 = j;
                }
        }

/* Upper Part of DVQ */
        dist_d_min2 = 1.0e9;
        d_idx_tmp2 = 0;
        for(j=0;j<num_dc;j++) {
                dist_d2 = 0.;
                for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
                        dist_d2 += (in[i]-qv_d[lpc_order*j+i])
                                *(in[i]-qv_d[lpc_order*j+i])*weight[i];
                if(dist_d2<dist_d_min2) {
                        dist_d_min2 = dist_d2;
                        d_idx_tmp2 = j;
                }
        }
}

        if((qMode != 1) &&
           (dist_rd_min1+dist_rd_min2)<(dist_d_min1+dist_d_min2)) {
                for(i=0;i<dim_2[0];i++) out[i] = qv_rd[lpc_order*rd_idx_tmp1+i];
                for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
                        out[i] = qv_rd[lpc_order*rd_idx_tmp2+i];
                idx[0] = idx_tmp[2*rd_idx_tmp1];
                idx[1] = idx_tmp[2*rd_idx_tmp2+1];
                idx[2] = rd_idx[2*rd_idx_tmp1];
                idx[3] = rd_idx[2*rd_idx_tmp2+1];
                idx[4] = 1;
        }else{
                for(i=0;i<dim_2[0];i++) out[i] = qv_d[lpc_order*d_idx_tmp1+i];
                for(i=dim_2[0];i<dim_2[0]+dim_2[1];i++)
                        out[i] = qv_d[lpc_order*d_idx_tmp2+i];
                idx[0] = idx_tmp[2*d_idx_tmp1];
                idx[1] = idx_tmp[2*d_idx_tmp2+1];
                idx[2] = d_idx[2*d_idx_tmp1];
                idx[3] = d_idx[2*d_idx_tmp2+1];
                idx[4] = 0;
        }



    if(level == 0)
    {
        for(i = 0; i < lpc_order; i++)
        {
            out[i] = out_v[i];
        }
    }


        pan_stab(out, min_gap, lpc_order);

}

/* ------------------------------------------------------------------ */


void pan_v_qtz_w_dd(float in[], long code[], long cnum, float tbl[], long dim, 
	float wt[], long nd)
{
	float	dist, dmin[PAN_N_DC_LSP_MAX];
	long	i, j, k;
	
	for(i=0;i<nd;i++) dmin[i] = 1e9;
	for(i=0;i<cnum;i++) {
		dist = 0.;
		for(j=0;j<dim;j++) {
			dist += (in[j]-tbl[dim*i+j])
				* (in[j]-tbl[dim*i+j])*wt[j];
		}
		for(j=0;j<nd;j++) {
			if(dist<dmin[j]) {
				for(k=nd-1;k>j;k--) {
					dmin[k] = dmin[k-1];	
					code[k] = code[k-1];
				}
				dmin[j] = dist;
				code[j] = i;
				break;
			}
		}
	}
}

/* ------------------------------------------------------------------ */

void pan_rd_qtz2_w(float in[], float out_p[], float out_v[], long *code, long cnum, 
	float tbl[], long dim, float wt[], float p_factor)
{
	long	i, j;
	float	c[PAN_DIM_MAX];
	float	dist, dmin;

	for(j=0;j<dim;j++) {
		c[j] = in[j]-(p_factor*out_p[j]+(1.-p_factor)*out_v[j]);
	}

	dmin = 1e9;
	for(i=0;i<cnum;i++) {
		dist = 0.;
		for(j=0;j<dim;j++) {
			dist += (tbl[i*dim+j]-c[j])*(tbl[i*dim+j]-c[j])*wt[j];
		}
		if(dist<dmin) {
			dmin = dist;	
			*code = i;
		}
		dist = 0.;
		for(j=0;j<dim;j++) {
			dist += (-tbl[i*dim+j]-c[j])*(-tbl[i*dim+j]-c[j])*wt[j];
		}
		if(dist<dmin) {
			dmin = dist;	
			*code = cnum+i;
		}
	}
}

/* ------------------------------------------------------------------ */

void pan_d_qtz_w(float in[], float out_v[], long *code, long cnum, float tbl[], long dim, 
	float wt[])
{
	long	i, j;
	float	c[PAN_DIM_MAX];
	float	dist, dmin;

	for(j=0;j<dim;j++) {
		c[j] = in[j]-out_v[j];
	}

	dmin = 1e9;
	for(i=0;i<cnum;i++) {
		dist = 0.;
		for(j=0;j<dim;j++) {
			dist += (tbl[i*dim+j]-c[j])*(tbl[i*dim+j]-c[j])*wt[j];
		}
		if(dist<dmin) {
			dmin = dist;	
			*code = i;
		}
		dist = 0.;
		for(j=0;j<dim;j++) {
			dist += (-tbl[i*dim+j]-c[j])*(-tbl[i*dim+j]-c[j])*wt[j];
		}
		if(dist<dmin) {
			dmin = dist;	
			*code = cnum+i;
		}
	}
}

