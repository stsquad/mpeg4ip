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


#ifndef _att_proto_h_
#define _att_proto_h_

#ifdef __cplusplus
extern "C" {
#endif

void att_abs_pitch_estimation(
  float PP_InputSignal[],	/* input */
  long frame_size,			/* configuration input */
  long lpc_order,			/* configuration input */
  float min_pitch_frequency,	/* input */
  float max_pitch_frequency,	/* input */
  long  sampling_frequency,	/* coufiguration input */
  long  lag_candidates[],	/* output */
  long  n_lag_candidates	/* configuration input */
);

void att_abs_weighting_module( 
 float *a,		/* input : lpc coefficients 1 + a1z^-1 + a2 */
 float *ax,		/* output: weighted coefficients for numerator  */
 long num_order,	/* input : numerator order */
 float *bx,		/* output: weighted coefficients for denominator */
 long den_order,	/* input : denominator order */
 float gamma_num,   /* weighting factor for numerator */
 float gamma_den    /* weighting factor for denominator */
);

void att_abs_postprocessing( 
 float *input,		/* input : signal */
 float *output,		/* output: signal */
 float *a,		/* input : lpc coefficients 1 + a1z^-1 + a2 */
 long order,		/* input : lpc order */
 long len,		/* input : frame size */
 long acb_delay,	/* input : delay (samples) for pitch postfilter  */
 float acb_gain		/* input : adaptive codebook gain */
);

void att_abs_postprocessing16( 
 float *input,		/* input : signal */
 float *output,		/* output: signal */
 float *a,		/* input : lpc coefficients 1 + a1z^-1 + a2 */
 long order,		/* input : lpc order */
 long len,		/* input : frame size */
 long acb_delay,	/* input : delay (samples) for pitch postfilter  */
 float acb_gain		/* input : adaptive codebook gain */
);

extern void bwx(
 float *bw,		/* ouput : bw expanded coefficients */
 const float *a,	/* input : lpc coefficients */
 const float w,		/* input : gamma factor */
 const long order	/* input : LPC order */
)
;
extern void firfilt(
float *output,		/* output: filtered signal */
const float *input,	/* input : signal */
const float *a,		/* input : filter coefficients */
float *mem,		/* in/out: filter memory */
long order,		/* input : filter order */
long length		/* input : number of samples to filter */
)
;
extern void iirfilt(
float *output,		/* output: filtered signal */
const float *input,	/* input : signal */
const float *a,		/* input : filter coefficients */
float *mem,		/* in/out: filter memory */
long order,		/* input : filter order */
long length		/* input : size of data array */
)
;
extern void  pitchpf(
 float *output,			/* output: filtered signal */
 float *input,			/* input : unfiltered signal */
 long len,			/* input : size of input/output frame */
 long  acb_delay,		/* input : acb delay */
 float acb_gain,		/* input : acb gain  */
 float *pitchmem,		/* in/out: signal buffer  */
 long pitchmemsize		/* input : size of pitch buffer */
)
;
extern void  pitchpf16(
 float *output,			/* output: filtered signal */
 float *input,			/* input : unfiltered signal */
 long len,			/* input : size of input/output frame */
 long  acb_delay,		/* input : acb delay */
 float acb_gain,		/* input : acb gain  */
 float *pitchmem,		/* in/out: signal buffer  */
 long pitchmemsize		/* input : size of pitch buffer */
)
;
extern void testbound(
const long a,		/* input: value to be tested */
const long b,		/* input: boundary value */
const char *text	/* input: program name */
)
;

/* weighting module */
long pc2rc(		/* output: 1 = normal return; 0 = rc[] clipped */
float *rc,		/* output: reflection coefficients */
const float *a,		/* input : LPC coefficients */
long order		/* input : LPC order */
);

long pc2lsf(		/* ret 1 on succ, 0 on failure */
float lsf[],		/* output: the np line spectral freq. */
const float pc[],	/* input : the np+1 predictor coeff. */
long np			/* input : order = # of freq to cal. */
);

void lsf2pc(
float *pc,		/* output: predictor coeff: 1,a1,a2...*/
const float *lsf,	/* input : line-spectral freqs [0,pi] */
long order		/* input : predictor order */
);

void lsffir(
float *x,		/* out/in: input and output signals */
const float *lsf,	/* input : line-spectral freqs, range [0:pi)*/
long order,		/* input : order of lsf */
double *w,		/* input : filter state (2*(order+1) */
long framel		/* input : length of array */
);

float FNevChebP(		/* result */
float  x,			/* input : value */
const float c[],		/* input : Array of coefficient values */
long n				/* input : Number of coefficients */
);

#ifdef __cplusplus
}
#endif

#endif
