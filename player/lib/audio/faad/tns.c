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
 * $Id: tns.c,v 1.8 2003/02/18 18:51:31 wmaycisco Exp $
 */

#include "all.h"
#include "util.h"

#define sfb_offset(x) ( ((x) > 0) ? sfb_top[(x)-1] : 0 )

/* Decoder transmitted coefficients for one TNS filter */
static void tns_decode_coef( int order, int coef_res, int *coef, Float *a )
{
    int i, m;
    Float iqfac,  iqfac_m;
    Float tmp[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];

    /* Inverse quantization */
    iqfac   = ((1 << (coef_res-1)) - 0.5f) / (C_PI/2.0f);
    iqfac_m = ((1 << (coef_res-1)) + 0.5f) / (C_PI/2.0f);
    for (i=0; i<order; i++)  {
        tmp[i+1] = (float)sin( coef[i] / ((coef[i] >= 0) ? iqfac : iqfac_m) );
    }
    /* Conversion to LPC coefficients
     * Markel and Gray,  pg. 95
     */
    a[0] = 1;
    for (m=1; m<=order; m++)  {
        b[0] = a[0];
        for (i=1; i<m; i++)  {
            b[i] = a[i] + tmp[m] * a[m-i];
        }
        b[m] = tmp[m];
        for (i=0; i<=m; i++)  {
            a[i] = b[i];
        }
    }
}

/* apply the TNS filter */
static void tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order )
{
    /*
     *  - Simple all-pole filter of order "order" defined by
     *    y(n) =  x(n) - a(2)*y(n-1) - ... - a(order+1)*y(n-order)
     *
     *  - The state variables of the filter are initialized to zero every time
     *
     *  - The output data is written over the input data ("in-place operation")
     *
     *  - An input vector of "size" samples is processed and the index increment
     *    to the next data sample is given by "inc"
     */
    int i, j;
    Float y, state[TNS_MAX_ORDER];

    for (i=0; i<order; i++)
        state[i] = 0;

    if (inc == -1)
        spec += size-1;

    for (i=0; i<size; i++) {
        y = *spec;
        for (j=0; j<order; j++)
            y -= lpc[j+1] * state[j];
        for (j=order-1; j>0; j--)
            state[j] = state[j-1];
        state[0] = y;
        *spec = y;
        spec += inc;
    }
}

/* TNS decoding for one channel and frame */
void tns_decode_subblock(faacDecHandle hDecoder, Float *spec, int nbands,
                         int *sfb_top, int islong, TNSinfo *tns_info)
{
    int f, m, start, stop, size, inc;
    int n_filt, coef_res, order, direction;
    int *coef;
    Float lpc[TNS_MAX_ORDER+1];
    TNSfilt *filt;

    n_filt = tns_info->n_filt;
    for (f=0; f<n_filt; f++)  {
        coef_res = tns_info->coef_res;
        filt = &tns_info->filt[f];
        order = filt->order;
        direction = filt->direction;
        coef = filt->coef;
        start = filt->start_band;
        stop = filt->stop_band;

        m = tns_max_order(hDecoder, islong);
        if (order > m) {
            order = m;
        }
        if (!order)  continue;

        tns_decode_coef(order, coef_res, coef, lpc);

        start = min(start, tns_max_bands(hDecoder, islong));
        start = min(start, nbands);
        start = sfb_offset( start );

        stop = min(stop, tns_max_bands(hDecoder, islong));
        stop = min(stop, nbands);
        stop = sfb_offset( stop );
        if ((size = stop - start) <= 0)  continue;

        if (direction)  {
            inc = -1;
        }  else {
            inc =  1;
        }

        tns_ar_filter( &spec[start], size, inc, lpc, order );
    }
}


/********************************************************/
/* TnsInvFilter:                                        */
/*   Inverse filter the given spec with specified       */
/*   length using the coefficients specified in filter. */
/*   Not that the order and direction are specified     */
/*   withing the TNS_FILTER_DATA structure.             */
/********************************************************/
static void TnsInvFilter(int length, Float *spec, TNSfilt *filter, Float *coef)
{
    int i,j,k = 0;
    int order = filter->order;
    Float *a = coef;
    Float *temp;

    temp = (Float *)AllocMemory(length * sizeof (Float));

    /* Determine loop parameters for given direction */
    if (filter->direction)
    {
        /* Startup, initial state is zero */
        temp[length-1]=spec[length-1];
        for (i=length-2;i>(length-1-order);i--) {
            temp[i]=spec[i];
            k++;
            for (j=1;j<=k;j++) {
                spec[i]+=temp[i+j]*a[j];
            }
        }

        /* Now filter the rest */
        for (i=length-1-order;i>=0;i--) {
            temp[i]=spec[i];
            for (j=1;j<=order;j++) {
                spec[i]+=temp[i+j]*a[j];
            }
        }

    } else {
        /* Startup, initial state is zero */
        temp[0]=spec[0];
        for (i=1;i<order;i++) {
            temp[i]=spec[i];
            for (j=1;j<=i;j++) {
                spec[i]+=temp[i-j]*a[j];
            }
        }

        /* Now filter the rest */
        for (i=order;i<length;i++) {
            temp[i]=spec[i];
            for (j=1;j<=order;j++) {
                spec[i]+=temp[i-j]*a[j];
            }
        }
    }

    FreeMemory(temp);
}


void tns_filter_subblock(faacDecHandle hDecoder, Float *spec, int nbands,
                         int *sfb_top, int islong, TNSinfo *tns_info)
{
    int f, m, start, stop, size;
    int n_filt, coef_res, order;
    int *coef;
    Float lpc[TNS_MAX_ORDER+1];
    TNSfilt *filt;

    n_filt = tns_info->n_filt;
    for (f=0; f<n_filt; f++)
    {
        coef_res = tns_info->coef_res;
        filt = &tns_info->filt[f];
        order = filt->order;
        coef = filt->coef;
        start = filt->start_band;
        stop = filt->stop_band;

        m = tns_max_order(hDecoder, islong);
        if (order > m)
            order = m;

        if (!order)  continue;

        tns_decode_coef(order, coef_res, coef, lpc);

        start = min(start, tns_max_bands(hDecoder, islong));
        start = min(start, nbands);
        start = sfb_offset( start );

        stop = min(stop, tns_max_bands(hDecoder, islong));
        stop = min(stop, nbands);
        stop = sfb_offset( stop );
        if ((size = stop - start) <= 0)
            continue;

        TnsInvFilter(size, spec + start, filt, lpc);
    }
}

