/*
This software module was originally developed by

Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)

and edited by

Heiko Purnhagen (University of Hannover),
Kazuyuki Iijima (Sony Corporation) and
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

/* Function prototype difinitions */
/*  Last modified: 11/07/96  N.Tanaka */
/*                 06/18/97  N.Tanaka */

/* 12-nov-96   HP    moved pan_lspqtz2_dd() prototype to libLPC_pan.h */
/* 06-feb-97   HP    removed #include "libLPC_pan.h" */
/* 16-jun-99   YM    added pan_lspdecEP() prototype */

#ifndef _pan_celp_proto_h_
#define _pan_celp_proto_h_

#include "lpc_common.h"

#ifdef __cplusplus
extern "C" {
#endif 

void pan_lspqtz2_dd(float in[], float out_p[], float out[], 
    float weight[], float p_factor, float min_gap, 
    long lpc_order, long num_dc, long idx[], 
    float tbl[], float d_tbl[], float rd_tbl[], 
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[], long flagStab);
void pan_lspqtz2_ddVR(float in[], float out_p[], float out[],
    float weight[], float p_factor, float min_gap,
    long lpc_order, long num_dc, long idx[],
    float tbl[], float d_tbl[], float rd_tbl[],
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[], int level,
    int qMode);
void pan_v_qtz_w_dd(float in[], long code[], long cnum, float tbl[], long dim, 
    float wt[], long nd);
void pan_rd_qtz2_w(float in[], float out_p[], float out_v[], long *code, 
    long cnum, float tbl[], long dim, float wt[], float p_factor);
void pan_d_qtz_w(float in[], float out_v[], long *code, long cnum, float tbl[],
    long dim, float wt[]);

void pan_lspdec(float out_p[], float out[], 
    float p_factor, float min_gap, long lpc_order, unsigned long idx[], 
    float tbl[], float d_tbl[], float rd_tbl[], 
    long dim_1[], long ncd_1[], long dim_2[], long ncd_2[], long flagStab, 
    long flagPred);
void pan_stab(float lsp[], float min_gap, long n);
void pan_sort(float x[], long n);
void pan_lsp_interpolation(float PrevLSPCoef[], float LSPCoef[], 
    float IntLSPCoef[], long lpc_order, long n_subframes, long c_subframe);

void pan_mv_cdata(char in[], char out[], long num);
void pan_mv_sdata(short in[], short out[], long num);
void pan_mv_ldata(long in[], long out[], long num);
void pan_mv_fdata(float in[], float out[], long num);
void pan_mv_ddata(double in[], double out[], long num);


#ifdef __cplusplus
}
#endif

#endif

