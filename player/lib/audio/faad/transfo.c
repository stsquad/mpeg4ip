/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7,
14496-1,2 and 3. This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio
standards free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing patents.
The original developer of this software module and his/her company, the subsequent
editors and their companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not released for
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the
code to a third party and to inhibit third party from using the code for non
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works."
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
/*
 * $Id: transfo.c,v 1.5 2002/01/11 00:55:17 wmaycisco Exp $
 */

#include <math.h>
#include "all.h"
#include "transfo.h"
#include "fastfft.h"

/*
#define INTEL_SPL
*/

#ifdef INTEL_SPL
#define nsp_UsesAll
#include <nsp.h>
#include <nspwin.h>

/* choose the library that best fits your processor */
#pragma comment (lib, "nsppxl.lib")
#endif


#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923
#endif

void MDCT_Long(faacDecHandle hDecoder, fftw_real *data)
{
#ifdef INTEL_SPL
    SCplx FFT_data[512];
#else
    fftw_complex FFTarray[512];    /* the array for in-place FFT */
#endif
    fftw_real tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
/*  fftw_real freq = 0.0030679616611450911f; */
    fftw_real fac,cosfreq8,sinfreq8;
    int i, n;
    int b = 2048 >> 1;
    int N4 = 2048 >> 2;
    int N2 = 2048 >> 1;
    int a = 2048 - b;
    int a2 = a >> 1;
    int a4 = a >> 2;
    int b4 = b >> 2;
    int unscambled;


    /* Choosing to allocate 2/N factor to Inverse Xform! */
    fac = 2.; /* 2 from MDCT inverse  to forward */

    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.99999529123306274f;
    sfreq = 0.0030679567717015743f;
    cosfreq8 = 0.99999994039535522f;
    sinfreq8 = 0.00038349519824312089f;

    c = cosfreq8;
    s = sinfreq8;

    for (i = 0; i < N4; i++)
    {
        /* calculate real and imaginary parts of g(n) or G(p) */

    n = 2048 / 2 - 1 - 2 * i;
        if (i < b4) {
      tempr = data [a2 + n] + data [2048 + a2 - 1 - n]; /* use second form of e(n) for n = N / 2 - 1 - 2i */
        } else {
            tempr = data [a2 + n] - data [a2 - 1 - n]; /* use first form of e(n) for n = N / 2 - 1 - 2i */
        }
        n = 2 * i;
        if (i < a4) {
            tempi = data [a2 + n] - data [a2 - 1 - n]; /* use first form of e(n) for n=2i */
        } else {
      tempi = data [a2 + n] + data [2048 + a2 - 1 - n]; /* use second form of e(n) for n=2i*/
        }

        /* calculate pre-twiddled FFT input */
#ifdef INTEL_SPL
    FFT_data[i].re = tempr * c + tempi * s;
    FFT_data[i].im = tempi * c - tempr * s;
#else
        FFTarray [i].re = tempr * c + tempi * s;
        FFTarray [i].im = tempi * c - tempr * s;
#endif

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }

    /* Perform in-place complex FFT of length N/4 */
#ifdef INTEL_SPL
    nspcFft(FFT_data, 9, NSP_Forw);
#else
    pfftw_512(FFTarray);
#endif


    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < N4; i++)
    {

      /* get post-twiddled FFT output  */
      /* Note: fac allocates 4/N factor from IFFT to forward and inverse */
#ifdef INTEL_SPL
      tempr = fac * (FFT_data[i].re * c + FFT_data[i].im * s);
      tempi = fac * (FFT_data[i].im * c - FFT_data[i].re * s);
#else
      unscambled = hDecoder->unscambled512[i];

      tempr = fac * (FFTarray [unscambled].re * c + FFTarray [unscambled].im * s);
      tempi = fac * (FFTarray [unscambled].im * c - FFTarray [unscambled].re * s);
#endif

      /* fill in output values */
      data [2 * i] = -tempr;   /* first half even */
      data [N2 - 1 - 2 * i] = tempi;  /* first half odd */
      data [N2 + 2 * i] = -tempi;  /* second half even */
      data [2048 - 1 - 2 * i] = tempr;  /* second half odd */

      /* use recurrence to prepare cosine and sine for next value of i */
      cold = c;
      c = c * cfreq - s * sfreq;
      s = s * cfreq + cold * sfreq;
    }
}

void MDCT_Short(faacDecHandle hDecoder, fftw_real *data)
{
#ifdef INTEL_SPL
    SCplx FFT_data[64];
#else
    fftw_complex FFTarray[64];    /* the array for in-place FFT */
#endif
    fftw_real tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
/*  fftw_real freq = 0.024543693289160728f; */
    fftw_real fac,cosfreq8,sinfreq8;
    int i, n;
    int b = 256 >> 1;
    int N4 = 256 >> 2;
    int N2 = 256 >> 1;
    int a = 256 - b;
    int a2 = a >> 1;
    int a4 = a >> 2;
    int b4 = b >> 2;
    int unscambled;


    /* Choosing to allocate 2/N factor to Inverse Xform! */
    fac = 2.; /* 2 from MDCT inverse  to forward */

    /* prepare for recurrence relation in pre-twiddle */
    cfreq = 0.99969881772994995f;
    sfreq = 0.024541229009628296f;
    cosfreq8 = 0.99999529123306274f;
    sinfreq8 = 0.0030679568483393833f;

    c = cosfreq8;
    s = sinfreq8;

    for (i = 0; i < N4; i++)
    {
        /* calculate real and imaginary parts of g(n) or G(p) */

    n = 256 / 2 - 1 - 2 * i;
        if (i < b4) {
      tempr = data [a2 + n] + data [256 + a2 - 1 - n]; /* use second form of e(n) for n = N / 2 - 1 - 2i */
        } else {
            tempr = data [a2 + n] - data [a2 - 1 - n]; /* use first form of e(n) for n = N / 2 - 1 - 2i */
        }
        n = 2 * i;
        if (i < a4) {
            tempi = data [a2 + n] - data [a2 - 1 - n]; /* use first form of e(n) for n=2i */
        } else {
      tempi = data [a2 + n] + data [256 + a2 - 1 - n]; /* use second form of e(n) for n=2i*/
        }

        /* calculate pre-twiddled FFT input */
#ifdef INTEL_SPL
    FFT_data[i].re = tempr * c + tempi * s;
    FFT_data[i].im = tempi * c - tempr * s;
#else
        FFTarray [i].re = tempr * c + tempi * s;
        FFTarray [i].im = tempi * c - tempr * s;
#endif

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }

    /* Perform in-place complex FFT of length N/4 */
#ifdef INTEL_SPL
    nspcFft(FFT_data, 6, NSP_Forw);
#else
    pfftw_64(FFTarray);
#endif

    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < N4; i++) {

        /* get post-twiddled FFT output  */
        /* Note: fac allocates 4/N factor from IFFT to forward and inverse */
#ifdef INTEL_SPL
    tempr = fac * (FFT_data[i].re * c + FFT_data[i].im * s);
    tempi = fac * (FFT_data[i].im * c - FFT_data[i].re * s);
#else
        unscambled = hDecoder->unscambled64[i];

    tempr = fac * (FFTarray [unscambled].re * c + FFTarray [unscambled].im * s);
    tempi = fac * (FFTarray [unscambled].im * c - FFTarray [unscambled].re * s);
#endif

        /* fill in output values */
        data [2 * i] = -tempr;   /* first half even */
        data [N2 - 1 - 2 * i] = tempi;  /* first half odd */
        data [N2 + 2 * i] = -tempi;  /* second half even */
    data [256 - 1 - 2 * i] = tempr;  /* second half odd */

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }
}


void IMDCT_Long(faacDecHandle hDecoder, fftw_real *data)
{
#ifdef INTEL_SPL
  SCplx FFT_data[512];    /* the array for in-place FFT */
#else
    fftw_complex FFTarray[512];    /* the array for in-place FFT */
#endif
    fftw_real tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */

/*  fftw_real freq = 0.0030679616611450911f; */
    fftw_real fac, cosfreq8, sinfreq8;
    int i;
  int Nd2 = 2048 >> 1;
  int Nd4 = 2048 >> 2;
  int Nd8 = 2048 >> 3;
    int unscambled;

    /* Choosing to allocate 2/N factor to Inverse Xform! */
  fac = 0.0009765625f;

    /* prepare for recurrence relation in pre-twiddle */
  cfreq = 0.99999529123306274f;
  sfreq = 0.0030679567717015743f;
  cosfreq8 = 0.99999994039535522f;
  sinfreq8 = 0.00038349519824312089f;

    c = cosfreq8;
    s = sinfreq8;


    for (i = 0; i < Nd4; i++) {

        /* calculate real and imaginary parts of g(n) or G(p) */
        tempr = -data [2 * i];
        tempi = data [Nd2 - 1 - 2 * i];

        /* calculate pre-twiddled FFT input */
#ifdef INTEL_SPL
    FFT_data[i].re = tempr * c - tempi * s;
    FFT_data[i].im = tempi * c + tempr * s;
#else
        unscambled = hDecoder->unscambled512[i];

        FFTarray [unscambled].re = tempr * c - tempi * s;
        FFTarray [unscambled].im = tempi * c + tempr * s;
#endif

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }

    /* Perform in-place complex IFFT of length N/4 */
#ifdef INTEL_SPL
  nspcFft(FFT_data, 9, NSP_Inv | NSP_NoScale);
#else
  pfftwi_512(FFTarray);
#endif

    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < Nd4; i++) {

        /* get post-twiddled FFT output  */
#ifdef INTEL_SPL
  tempr = fac * (FFT_data[i].re * c - FFT_data[i].im * s);
      tempi = fac * (FFT_data[i].im * c + FFT_data[i].re * s);
#else
    tempr = fac * (FFTarray[i].re * c - FFTarray[i].im * s);
        tempi = fac * (FFTarray[i].im * c + FFTarray[i].re * s);
#endif

        /* fill in output values */
        data [Nd2 + Nd4 - 1 - 2 * i] = tempr;
        if (i < Nd8) {
            data [Nd2 + Nd4 + 2 * i] = tempr;
        } else {
            data [2 * i - Nd4] = -tempr;
        }
        data [Nd4 + 2 * i] = tempi;
        if (i < Nd8) {
            data [Nd4 - 1 - 2 * i] = -tempi;
        } else {
      data [Nd4 + 2048 - 1 - 2*i] = tempi;
        }

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
  }
}


void IMDCT_Short(faacDecHandle hDecoder, fftw_real *data)
{
#ifdef INTEL_SPL
  SCplx FFT_data[64];    /* the array for in-place FFT */
#else
    fftw_complex FFTarray[64];    /* the array for in-place FFT */
#endif
    fftw_real tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
/*  fftw_real freq = 0.024543693289160728f; */
    fftw_real fac, cosfreq8, sinfreq8;
    int i;
    int Nd2 = 256 >> 1;
    int Nd4 = 256 >> 2;
    int Nd8 = 256 >> 3;
    int unscambled;

    /* Choosing to allocate 2/N factor to Inverse Xform! */
  fac = 0.0078125f; /* remaining 2/N from 4/N IFFT factor */

    /* prepare for recurrence relation in pre-twiddle */
  cfreq = 0.99969881772994995f;
  sfreq = 0.024541229009628296f;
  cosfreq8 = 0.99999529123306274f;
  sinfreq8 = 0.0030679568483393833f;

    c = cosfreq8;
    s = sinfreq8;

    for (i = 0; i < Nd4; i++)
  {

        /* calculate real and imaginary parts of g(n) or G(p) */
        tempr = -data [2 * i];
        tempi = data [Nd2 - 1 - 2 * i];

        /* calculate pre-twiddled FFT input */
#ifdef INTEL_SPL
    FFT_data[i].re = tempr * c - tempi * s;
    FFT_data[i].im = tempi * c + tempr * s;
#else
        unscambled = hDecoder->unscambled64[i];

        FFTarray [unscambled].re = tempr * c - tempi * s;
        FFTarray [unscambled].im = tempi * c + tempr * s;
#endif

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }

    /* Perform in-place complex IFFT of length N/4 */

#ifdef INTEL_SPL
  nspcFft(FFT_data, 6, NSP_Inv | NSP_NoScale);
#else
    pfftwi_64(FFTarray);
#endif

    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < Nd4; i++) {

        /* get post-twiddled FFT output  */
#ifdef INTEL_SPL
      tempr = fac * (FFT_data[i].re * c - FFT_data[i].im * s);
      tempi = fac * (FFT_data[i].im * c + FFT_data[i].re * s);
#else
    tempr = fac * (FFTarray[i].re * c - FFTarray[i].im * s);
        tempi = fac * (FFTarray[i].im * c + FFTarray[i].re * s);
#endif

        /* fill in output values */
        data [Nd2 + Nd4 - 1 - 2 * i] = tempr;
        if (i < Nd8) {
            data [Nd2 + Nd4 + 2 * i] = tempr;
        } else {
            data [2 * i - Nd4] = -tempr;
        }
        data [Nd4 + 2 * i] = tempi;
        if (i < Nd8) {
            data [Nd4 - 1 - 2 * i] = -tempi;
        } else {
      data [Nd4 + 256 - 1 - 2*i] = tempi;
        }

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }
}


