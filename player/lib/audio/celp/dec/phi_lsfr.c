/*


This software module was originally developed by

Peter Kroon (Bell Laboratories, Lucent Technologies)
P. Kabal    (McGill University)

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


 * Last modified: May 1, 1996
 *
 */
/*====================================================================*/
/**---------------------------------------------------------------------------- Telecommunications & Signal Processing Lab ---------------
 *                            McGill University
 * Author / revision:
 *  P. Kabal
 *  $Revision: 1.1 $  $Date: 2002/05/13 18:57:42 $
 *  Revised for speechlib library 8/15/94
 *  
 */

/*======================================================================
    Modified by Andy Gerrits, Philips Research Laboratories.
    Date: 1997/03/27

  ======================================================================*/

/*======================================================================*/
/*      I N C L U D E S                                                 */
/*======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "phi_lsfr.h"
#include "att_proto.h"    

/*======================================================================*/
/*     L O C A L     S Y M B O L     D E F I N I T I O N S              */
/*======================================================================*/
#define MAXORDER 20         /* Modified by AG: 16 -> 20 */
#ifndef PI
#define PI 3.1415927
#endif
#define	NBIS		4		/* number of bisections */
#define	DW		(0.01 * PI)	/* resolution for initial search */

/*======================================================================*/
/*     L O C A L    F U N C T I O N S     P R O T O T Y P E S           */ 
/*======================================================================*/

float FNevChebP(		/* result */
float  x,			/* input : value */
const float c[],		/* input : Array of coefficient values */
long n				/* input : Number of coefficients */
);

long pc2lsf_org(		/* ret 1 on succ, 0 on failure */
float lsf[],		/* output: the np line spectral freq. */
const float pc[],	/* input : the np+1 predictor coeff. */
long np			    /* input : order = # of freq to cal. */
);

/*======================================================================*/
/*       F U N C T I O N S     D E F I N I T I O N S                    */ 
/*======================================================================*/

/*----------------------------------------------------------------------------
 * lsf2pc - convert lsf to predictor coefficients
 *
 * FIR filter using the line spectral frequencies
 * can, of course, also be use to get predictor coeff
 * from the line spectral frequencies
 *----------------------------------------------------------------------------
 */
void PHI_lsf2pc
(
long order,            /* input : predictor order                     */
const float lsf[],     /* input : line-spectral freqs [0,pi]          */
float pc[]             /* output: predictor coeff: a_1,a_2...a_order  */
) 
{
   double mem[2*MAXORDER+2];
   float temp_pc[MAXORDER+1];
   long i;
 
   /* Modified by AG: assertion on order */  
   assert(order <= MAXORDER);
   
   for (i=0; i< 2*order+2; i++) mem[i] = 0.0;
   for (i=0; i< order+1; i++) temp_pc[i] = (float)0.0;
   temp_pc[0]=(float)1.0;
   lsffir( temp_pc, lsf, order, mem, order+1);
   
   /* Modified by AG: Alternative definition of a parameters */  
   for (i=0; i < order; i++) pc[i] = (float)-1.0 * temp_pc[i+1];
   
}

/*----------------------------------------------------------------------------
 * lsffir - FIR filter using the line spectral frequencies
 *----------------------------------------------------------------------------
 */
/* Modified by AG: doubles used instead of floats */  
void lsffir(
float *x,            /* out/in: input and output signals           */
const float *lsf,    /* input : line-spectral freqs, range [0:pi)  */
long order,          /* input : order of lsf                       */
double *w,           /* input : filter state (2*(order+1)          */
long framel          /* input : length of array                    */
)
{
   long j, i;
   long n1, n2, n3, n4;
   long odd;
   double xin1, xin2, xout1, xout2;

   odd = order % 2;
   for( j=0; j<framel; j++){
     xin1 = (double)x[j];
     xin2 = (double)x[j];
     for( i=0; i<(order>>1); i++){
        n1 = i*4;
        n2 = n1+1;
        n3 = n2+1;
        n4 = n3+1;
        xout1 = -2. * cos((double)lsf[i*2+0]) * w[n1] + w[n2] + xin1;
        xout2 = -2. * cos((double)lsf[i*2+1]) * w[n3] + w[n4] + xin2;
        w[n2] = w[n1];
        w[n1] = xin1;
        w[n4] = w[n3];
        w[n3] = xin2;
        xin1 = xout1;
        xin2 = xout2;
     }
     /* if odd, finish with extra odd term */
     if(odd == 1) {
        n1 = i*4;
        n2 = n1+1;
        n4 = n2;
        xout1 = -2. * cos((double)lsf[i*2+0]) * w[n1] + w[n2] + xin1;
        w[n2] = w[n1];
        w[n1] = xin1;
      } else            /* else even */
        xout1 = xin1 + w[n4+1];

      xout2 = xin2 - w[n4+2];
      x[j] =  (float)(0.5 * (xout1 + xout2));

      if(odd == 1) {
       w[n4+2] = w[n4+1];
       w[n4+1] = xin2;
      } else {
       w[n4+1] = xin1;
       w[n4+2] = xin2;
      }
   }
}


void PHI_pc2lsf(		/* ret 1 on succ, 0 on failure */
long np,			    /* input : order = # of freq to cal. */
const float pc[],	    /* input : the np predictor coeff. */
float lsf[] 		    /* output: the np line spectral freq. */
)
{
   float temp_pc[MAXORDER+1];
   long i;
 
   /* Modified by AG: assertion on order */  
   assert(np <= MAXORDER);
   
   for (i=1; i < np+1; i++) temp_pc[i] = (float)-1.0 * pc[i-1];
   temp_pc[0]=(float)1.0;

   if (!pc2lsf_org(lsf, temp_pc, np))
   {
       fprintf(stderr,"FATAL ERROR in PHI_pc2lsf\n");
       exit(0);
   }
}
  
/*----------------------------------------------------------------------------
 * pc2lsf -  Convert predictor coefficients to line spectral frequencies
 *
 * Description:
 *  The transfer function of the prediction error filter is formed from the
 *  predictor coefficients.  This polynomial is transformed into two reciprocal
 *  polynomials having roots on the unit circle. The roots of these polynomials
 *  interlace.  It is these roots that determine the line spectral frequencies.
 *  The two reciprocal polynomials are expressed as series expansions in
 *  Chebyshev polynomials with roots in the range -1 to +1.  The inverse cosine
 *  of the roots of the Chebyshev polynomial expansion gives the line spectral
 *  frequencies.  If np line spectral frequencies are not found, this routine
 *  stops with an error message.  This error occurs if the input coefficients
 *  do not give a prediction error filter with minimum phase.
 *
 *  Line spectral frequencies and predictor coefficients are usually expressed
 *  algebraically as vectors.
 *    lsf[0]     first (lowest frequency) line spectral frequency
 *    lsf[i]     1 <= i < np
 *    pc[0]=1.0  predictor coefficient corresponding to lag 0
 *    pc[i]      1 <= 1 <= np
 *
 * Parameters:
 *  ->  float pc[]
 *      Vector of predictor coefficients (Np+1 values).  These are the 
 *      coefficients of the predictor filter, with pc[0] being the predictor 
 *      coefficient corresponding to lag 0, and pc[Np] corresponding to lag Np.
 *      The predictor coeffs. must correspond to a minimum phase prediction
 *      error filter.
 *  <-  float lsf[]
 *      Array of Np line spectral frequencies (in ascending order).  Each line
 *      spectral frequency lies in the range 0 to pi.
 *  ->  long Np
 *      Number of coefficients (at most MAXORDER = 50)
 *----------------------------------------------------------------------------
 */
long pc2lsf_org(		/* ret 1 on succ, 0 on failure */
float lsf[],		/* output: the np line spectral freq. */
const float pc[],	/* input : the np+1 predictor coeff. */
long np			    /* input : order = # of freq to cal. */
)
{
  float fa[(MAXORDER+1)/2+1], fb[(MAXORDER+1)/2+1];
  float ta[(MAXORDER+1)/2+1], tb[(MAXORDER+1)/2+1];
  float *t;
  float xlow, xmid, xhigh;
  float ylow, ymid, yhigh;
  float xroot;
  float dx;
  long i, j, nf;
  long odd;
  long na, nb, n;
  float ss, aa;

  assert(np <= MAXORDER);

/* Determine the number of coefficients in ta(.) and tb(.) */
  odd = (long)(np % 2 != 0);
  if (odd) {
    nb = (np + 1) / 2;
    na = nb + 1;
  }
  else {
    nb = np / 2 + 1;
    na = nb;
  }

/*
*   Let D=z**(-1), the unit delay; the predictor coefficients form the
*
*             N         n 
*     P(D) = SUM pc[n] D  .   ( pc[0] = 1.0 )
*            n=0
*
*   error filter polynomial, A(D)=P(D) with N+1 terms.  Two auxiliary
*   polynomials are formed from the error filter polynomial,
*     Fa(D) = A(D) + D**(N+1) A(D**(-1))  (N+2 terms, symmetric)
*     Fb(D) = A(D) - D**(N+1) A(D**(-1))  (N+2 terms, anti-symmetric)
*/
  fa[0] = (float)1.0;
  for (i = 1, j = np; i < na; ++i, --j)
    fa[i] = pc[i] + pc[j];

  fb[0] = (float)1.0;
  for (i = 1, j = np; i < nb; ++i, --j)
    fb[i] = pc[i] - pc[j];

/*
* N even,  Fa(D)  includes a factor 1+D,
*          Fb(D)  includes a factor 1-D
* N odd,   Fb(D)  includes a factor 1-D**2
* Divide out these factors, leaving even order symmetric polynomials, M is the
* total number of terms and Nc is the number of unique terms,
*
*   N       polynomial            M         Nc=(M+1)/2
* even,  Ga(D) = F1(D)/(1+D)     N+1          N/2+1
*        Gb(D) = F2(D)/(1-D)     N+1          N/2+1
* odd,   Ga(D) = F1(D)           N+2        (N+1)/2+1
*        Gb(D) = F2(D)/(1-D**2)   N          (N+1)/2
*/
  if (odd) {
    for (i = 2; i < nb; ++i)
      fb[i] = fb[i] + fb[i-2];
  }
  else {
    for (i = 1; i < na; ++i) {
      fa[i] = fa[i] - fa[i-1];
      fb[i] = fb[i] + fb[i-1];
    }
  }

/*
*   To look for roots on the unit circle, Ga(D) and Gb(D) are evaluated for
*   D=exp(jw).  Since Gz(D) and Gb(D) are symmetric, they can be expressed in
*   terms of a series in cos(nw) for D on the unit circle.  Since M is odd and
*   D=exp(jw)
*
*           M-1        n 
*   Ga(D) = SUM fa(n) D             (symmetric, fa(n) = fa(M-1-n))
*           n=0
*                                    Mh-1
*         = exp(j Mh w) [ f1(Mh) + 2 SUM fa(n) cos((Mh-n)w) ]
*                                    n=0
*                       Mh
*         = exp(j Mh w) SUM ta(n) cos(nw) ,
*                       n=0
*
*   where Mh=(M-1)/2=Nc-1.  The Nc=Mh+1 coefficients ta(n) are defined as
*
*   ta(n) =   fa(Nc-1) ,    n=0,
*         = 2 fa(Nc-1-n) ,  n=1,...,Nc-1.
*   The next step is to identify cos(nw) with the Chebyshev polynomial T(n,x).
*   The Chebyshev polynomials satisfy the relationship T(n,cos(w)) = cos(nw).
*   Omitting the exponential delay term, the series expansion in terms of
*   Chebyshev polynomials is
*
*           Nc-1
*   Ta(x) = SUM ta(n) T(n,x)
*           n=0
*
*   The domain of Ta(x) is -1 < x < +1.  For a given root of Ta(x), say x0,
*   the corresponding position of the root of Fa(D) on the unit circle is
*   exp(j arccos(x0)).
*/
  ta[0] = fa[na-1];
  for (i = 1, j = na - 2; i < na; ++i, --j)
    ta[i] = (float)2.0 * fa[j];

  tb[0] = fb[nb-1];
  for (i = 1, j = nb - 2; i < nb; ++i, --j)
    tb[i] = (float)2.0 * fb[j];

/*
*   To find the roots, we sample the polynomials Ta(x) and Tb(x) looking for
*   sign changes.  An interval containing a root is successively bisected to
*   narrow the interval and then linear interpolation is used to estimate the
*   root.  For a given root at x0, the line spectral frequency is w0=acos(x0).
*
*   Since the roots of the two polynomials interlace, the search for roots
*   alternates between the polynomials Ta(x) and Tb(x).  The sampling interval
*   must be small enough to avoid having two cancelling sign changes in the
*   same interval.  Consider specifying the resolution in the LSF domain.  For
*   an interval [xl, xh], the corresponding interval in frequency is [w1, w2],
*   with xh=cos(w1) and xl=cos(w2) (note the reversal in order).  Let dw=w2-w1,
*     dx = xh-xl = xh [1-cos(dw)] + sqrt(1-xh*xh) sin(dw).
*   However, the calculation of the square root is overly time-consuming.  If
*   instead, we use equal steps in the x-domain, the resolution in the LSF
*   domain is best at at pi/2 and worst at 0 and pi.  As a compromise we fit a
*   quadratic to the step size relationship.  At x=1, dx=(1-cos(dw); at x=0,
*   dx=sin(dw).  Then the approximation is
*     dx' = (a(1-cos(dw))-sin(dw)) x**2 + sin(dw)).
*   For a=1, this value underestimates the step size in the range of interest.
*   However, the step size for just below x=1 and just above x=-1 fall well
*   below the desired step sizes.  To compensate for this, we use a=4.  Then at
*   x=+1 and x=-1, the step sizes are too large by a factor of 4, but rapidly
*   fall to about 60% of the desired values and then rise slowly to become 
*   equal to the desired step size at x=0.
*/
  nf = 0;
  t = ta;
  n = na;
  xroot = (float)2.0;
  xlow = (float)1.0;
  ylow = FNevChebP(xlow, t, n);

/*
*   Define the step-size function parameters
*   The resolution in the LSF domain is at least DW/2**NBIS, not counting the
*   increase in resolution due to the linear interpolation step.  For
*   DW=0.02*Pi, and NBIS=4, and a sampling frequency of 8000, this corresponds
*   to 5 Hz.
*/
  ss = (float)sin (DW);
  aa = (float)(4.0 - 4.0 * cos (DW)  - (double)ss);

/* Root search loop */
  while (xlow > (float)-1.0 && nf < np) {

    /* New trial point */
    xhigh = xlow;
    yhigh = ylow;
    dx = aa * xhigh * xhigh + ss;
    xlow = xhigh - dx;
    if (xlow < (float)-1.0)
      xlow = (float)-1.0;
    ylow = FNevChebP(xlow, t, n);
    if (ylow * yhigh <= (float)0.0) {

    /* Bisections of the interval containing a sign change */
      dx = xhigh - xlow;
      for (i = 1; i <= NBIS; ++i) {
	dx = (float)0.5 * dx;
	xmid = xlow + dx;
	ymid = FNevChebP(xmid, t, n);
	if (ylow * ymid <= (float)0.0) {
	  yhigh = ymid;
	  xhigh = xmid;
	}
	else {
	  ylow = ymid;
	  xlow = xmid;
	}
      }

      /*
       * Linear interpolation in the subinterval with a sign change
       * (take care if yhigh=ylow=0)
       */
      if (yhigh != ylow)
	xmid = xlow + dx * ylow / (ylow - yhigh);
      else
	xmid = xlow + dx;

      /* New root position */
      lsf[nf] = (float)acos((double) xmid);
      ++nf;

      /* Start the search for the roots of the next polynomial at the estimated
       * location of the root just found.  We have to catch the case that the
       * two polynomials have roots at the same place to avoid getting stuck at
       * that root.
       */
      if (xmid >= xroot) {
	xmid = xlow - dx;
      }
      xroot = xmid;
      if (t == ta) {
	t = tb;
	n = nb;
      }
      else {
	t = ta;
	n = na;
      }
      xlow = xmid;
      ylow = FNevChebP(xlow, t, n);
    }
  }

/* Error if np roots have not been found */
  if (nf != np) {
    printf("\nWARNING: pc2lsf failed to find all lsf nf=%ld np=%ld\n", nf, np);
    return(0);
  }
  return(1);
}

/**---------------------------------------------------------------------------- Telecommunications & Signal Processing Lab ---------------
*                             McGill University
*
* Module:
*  float FNevChebP(float x,  float c[], long N)
*
* Purpose:
*  Evaluate a series expansion in Chebyshev polynomials
*
* Description:
*  The series expansion in Chebyshev polynomials is defined as
*
*              N-1
*       Y(x) = SUM c(i) T(i,x) ,
*              i=0
*
*  where N is the order of the expansion, c(i) is the coefficient for the i'th
*  Chebyshev polynomial, and T(i,x) is the i'th order Chebyshev polynomial
*  evaluated at x.
*
*  The Chebyshev polynomials satisfy the recursion
*    T(i,x) = 2x T(i-1,x) - T(i-2,x),
*  with the initial conditions T(0,x)=1 and T(1,x)=x.  This routine evaluates
*  the expansion using a backward recursion.
*
* Parameters:
*  <-  float
*  FNevChebP -	Resulting value
*  ->  float
*  x -		Input value
*  ->  float []
*  c -		Array of coefficient values.  c[i] is the coefficient of the
*		i'th order Chebyshev polynomial.
*  ->  long
*  N -		Number of coefficients
*
* Author / revision:
*  P. Kabal
*  $Revision: 1.1 $  $Date: 2002/05/13 18:57:42 $
*
-------------------------------------------------------------------------*/
float FNevChebP(		/* result */
float  x,			/* input : value */
const float c[],		/* input : Array of coefficient values */
long n				/* input : Number of coefficients */
)
{
  long i;
  float b0, b1, b2;

/*****************
*   Consider the backward recursion
*     b(i,x) = 2xb(i+1,x) - b(i+2,x) + c(i),
*   with initial conditions b(n,x)=0 and b(n+1,x)=0.  Then dropping the
*   dependence on x,
*     c(i) = b(i) - 2xb(i+1) + b(i+2).
*
*          n-1
*   Y(x) = SUM c(i) T(i)
*          i=0
*
*          n-1
*        = SUM [b(i)-2xb(i+1)+b(i+2)] T(i)
*          i=0
*                                             n-1
*        = b(0)T(0) + b(1)T(1) - 2xb(1)T(0) + SUM b(i) [T(i)-2xT(i-1)+T(i-2)] 
*                                             i=2
* The term inside the sum is zero because of the recursive relationship
* satisfied by the Chebyshev polynomials.  Then substituting the values T(0)=1
* and T(1)=x, Y(x) is expressed in terms of the diff. between b(0) and b(2)
* (errors in b(0) and b(2) tend to cancel),
*   Y(x) = b(0)-xb(1) = [b(0)-b(2)+c(0)] / 2
*******************
*/
  b1 = (float)0.0;
  b0 = (float)0.0;
  for (i = n - 1; i >= 0; --i) {
    b2 = b1;
    b1 = b0;
    b0 = (float)2.0 * x * b1 - b2 + c[i];
  }

  return ((float)0.5 * (b0 - b2 + c[0]));
}

