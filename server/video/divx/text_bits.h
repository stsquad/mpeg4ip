
#ifndef _TEXT_BITS_H_
#define _TEXT_BITS_H_


#include "momusys.h"
#include "text_defs.h"



/* struct for counting bits */

typedef struct {
  Int Y;
  Int C;
  Int vec;
  Int CBPY;
  Int CBPC; 
  Int MCBPC;
  Int MODB;
  Int CBPB;
  Int MBTYPE;
  Int COD;
  Int MB_Mode; 
  Int header;
  Int DQUANT;
  Int total;
  Int no_inter;
  Int no_inter4v;
  Int no_intra;
  Int no_GMC;	/* NTT for GMC coding */
  Int ACpred_flag;
  Int G;	/* HYUNDAI : (Grayscale) */
  Int CODA;	/* HYUNDAI : (Grayscale) */
  Int CBPA;	/* HYUNDAI : (Grayscale) */
  Int g_ACpred_flag;	/* HYUNDAI : (Grayscale) */
  Int no_field;
  Int no_skipped;
  Int no_Pskip;
  Int no_noDCT;
  Int fieldDCT;
  Int interlaced;
  Int Btype[7];
  Int Nmvs[3];
} Bits;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Void  	MB_CodeCoeff _P_((	Bits *bits,
			Int *qcoeff,
			Int Mode,
			Int CBP,
			Int ncoeffs,
			Int intra_dcpred_disable,
			Image *DCbitstream,
			Image *bitstream,
			Int transp_pattern[],
			Int direction[],
			Int error_res_disable,
			Int reverse_vlc,
			Int switched,
			Int alternate_scan
	));

void  	Bits_Reset _P_((	Bits *bits
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 


#endif /* _TEXT_BITS_H_ */
