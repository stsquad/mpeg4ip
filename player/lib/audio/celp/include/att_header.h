/*


This software module was originally developed by

Peter Kroon (Bell Laboratories, Lucent Technologies)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.


 */


#ifndef _att_header_h_
#define _att_header_h_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Revised 06/26/96 These are defined as configuration input
#define  FRAME_SIZE 200
#define  MAX_N_LAG_CANDIDATES 2

#define  MAXLAG 156
*/

#define  mmax(x,y) ((x > y) ? x : y)
#define  mmin(x,y) ((x > y) ? y : x)

typedef signed short Int16;
typedef struct {
  float val;
  int   idx;
} lag_amdf;

#ifdef __cplusplus
extern "C" {
#endif

void Int2Float(Int16 *int_sig, float *flt_sig, long n);
void MoveFData(float *des, float *start, long len);
void MoveIData(Int16 *des, Int16 *start, long len);

void LPF_800(float inbuf[], float lpbuf[], long len);
void Inv_Filt(float lpbuf[], float ivbuf[], long len);
void AMDF(float speech[], lag_amdf amdf[], long min_lag, long max_lag, long len);
void MakeAmdfWeight(float weight[], long min_lag, long max_lag);
void Weighted_amdf(lag_amdf amdf[], float weight[], long min_lag, long max_lag);
void Sort_Amdf(lag_amdf amdf[], long len, long n_lag);
void Assign_candidate(long candidates[], lag_amdf amdf[], long n_cand);

#ifdef __cplusplus
}
#endif

#endif
