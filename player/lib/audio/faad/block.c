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
 * $Id: block.c,v 1.8 2003/05/23 18:55:19 wmaycisco Exp $
 */

#include "all.h"
#include "block.h"
#include "kbd_win.h"
#include "transfo.h"
#include "util.h"

#ifndef PI
#define PI 3.14159265359f
#endif



const int windowLeng[N_WINDOW_TYPES] = {
    1024,
    128,
    1024,
    1024
};


#ifndef WIN_TABLE
static float Izero(float x)
{
    float IzeroEPSILON = 1E-37f;  /* Max error acceptable in Izero */
    float sum, u, halfx, temp;
    int n;

    sum = u = 1.f;
    n = 1;
    halfx = x/2.0f;
    do {
        temp = halfx/(float)n;
        n += 1;
        temp *= temp;
        u *= temp;
        sum += u;
    } while (u >= IzeroEPSILON*sum);

    return(sum);
}

static void CalculateKBDWindow(float* win, float alpha, int length)
{
    int i;
    float IBeta;
    float tmp;
    float sum = 0.0;

    alpha *= PI;
    IBeta = 1.0f/Izero(alpha);

    /* calculate lower half of Kaiser Bessel window */
    for(i=0; i<(length>>1); i++) {
        tmp = 4.0f*(float)i/(float)length - 1.0f;
        win[i] = Izero(alpha*(float)sqrt(1.0f-tmp*tmp))*IBeta;
        sum += win[i];
    }

    sum = 1.0f/sum;
    tmp = 0.0f;

    /* calculate lower half of window */
    for(i=0; i<(length>>1); i++) {
        tmp += win[i];
        win[i] = (float)sqrt(tmp*sum);
    }
}
#endif


/*****************************************************************************
*
*   InitBlock
*   calculate windows for use by Window()
*   input: none
*   output: none
*   local static: none
*   globals: shortWindow[], longWindow[]
*
*****************************************************************************/
void InitBlock (faacDecHandle hDecoder)
{

    /* calc half-window data */
    int     i, j;
    float   phaseInc;

    hDecoder->sin_long = AllocMemory(NWINLONG*sizeof(Float));
    hDecoder->sin_short = AllocMemory(NWINSHORT*sizeof(Float));
#ifndef WIN_TABLE
    hDecoder->kbd_long = AllocMemory(NWINLONG*sizeof(Float));
    hDecoder->kbd_short = AllocMemory(NWINSHORT*sizeof(Float));
#endif
    hDecoder->sin_edler = AllocMemory(NWINLONG*sizeof(Float));
    hDecoder->kbd_edler = AllocMemory(NWINLONG*sizeof(Float));
    hDecoder->sin_adv = AllocMemory(NWINADV*sizeof(Float));
    hDecoder->kbd_adv = AllocMemory(NWINADV*sizeof(Float));

/*
Float *windowPtr[N_WINDOW_TYPES][N_WINDOW_SHAPES] = {
    {sin_long,  kbd_long},
    {sin_short, kbd_short},
    {sin_edler, kbd_edler},
    {sin_adv,   kbd_adv}
};
*/
    hDecoder->windowPtr[0][0] = hDecoder->sin_long;
    hDecoder->windowPtr[0][1] = hDecoder->kbd_long;
    hDecoder->windowPtr[1][0] = hDecoder->sin_short;
    hDecoder->windowPtr[1][1] = hDecoder->kbd_short;
    hDecoder->windowPtr[2][0] = hDecoder->sin_edler;
    hDecoder->windowPtr[2][1] = hDecoder->kbd_edler;
    hDecoder->windowPtr[3][0] = hDecoder->sin_adv;
    hDecoder->windowPtr[3][1] = hDecoder->kbd_adv;

    /* init half-windows */

    /* sin long window */
    phaseInc = PI / (2.0f * NWINLONG);
    for (i = 0; i < NWINLONG; i++) {
        hDecoder->sin_long [i]  = (float)sin(phaseInc * (i + 0.5));
    }

    /* sin short window */
    phaseInc = PI / (2.0f * NWINSHORT);
    for (i = 0; i < NWINSHORT; i++) {
        hDecoder->sin_short [i] = (float)sin(phaseInc * (i + 0.5));
    }

#ifndef WIN_TABLE
    CalculateKBDWindow(hDecoder->kbd_long, 4.f, NWINLONG*2);
    CalculateKBDWindow(hDecoder->kbd_short, 6.f, NWINSHORT*2);
#endif

    /* Edler windows */
    for (i = 0, j = 0; i < NFLAT; i++, j++) {
        hDecoder->sin_edler[j] = 0.0;
        hDecoder->kbd_edler[j] = 0.0;
    }
    for (i = 0; i < NWINSHORT; i++, j++) {
        hDecoder->sin_edler [j] = hDecoder->sin_short [i];
        hDecoder->kbd_edler [j] = hDecoder->kbd_short [i];
    }
    for ( ; j < NWINFLAT; j++) {
        hDecoder->sin_edler [j] = 1.0;
        hDecoder->kbd_edler [j] = 1.0;
    }

    /* Advanced Edler windows */
    for (i = 0, j = 0; i < NADV0; i++, j++) {
        hDecoder->sin_adv [j] = 0.0;
        hDecoder->kbd_adv [j] = 0.0;
    }
    for (i = 0; i < NWINSHORT; i++, j++) {
        hDecoder->sin_adv[j] = hDecoder->sin_short[i];
        hDecoder->kbd_adv[j] = hDecoder->kbd_short[i];
    }
    for ( ; j < NWINADV; j++) {
        hDecoder->sin_adv[j] = 1.0;
        hDecoder->kbd_adv[j] = 1.0;
    }
}

void EndBlock(faacDecHandle hDecoder)
{
    FreeMemory(hDecoder->sin_long);
    FreeMemory(hDecoder->sin_short);
#ifndef WIN_TABLE
    FreeMemory(hDecoder->kbd_long);
    FreeMemory(hDecoder->kbd_short);
#endif
    FreeMemory(hDecoder->sin_edler);
    FreeMemory(hDecoder->kbd_edler);
    FreeMemory(hDecoder->sin_adv);
    FreeMemory(hDecoder->kbd_adv);
}

/*****************************************************************************
*
*   Window
*   window input sequence based on window type
*   input: see below
*   output: see below
*   local static:
*     firstTime             flag = need to initialize data structures
*   globals: shortWindow[], longWindow[]
*
*****************************************************************************/

void ITransformBlock (faacDecHandle hDecoder,
    Float* dataPtr,             /* vector to be windowed in place   */
    BLOCK_TYPE bT,              /* input: window type               */
    Wnd_Shape *wnd_shape,
    Float *state                /* input/output                     */
    )
{
    int leng0, leng1;
    int         i,leng;
    Float       *windPtr;
    WINDOW_TYPE beginWT, endWT;

    if((bT==LONG_BLOCK) || (bT==START_FLAT_BLOCK)) {
        beginWT = WT_LONG;
    } else if(bT==STOP_FLAT_BLOCK) {
        beginWT = WT_FLAT;
    } else
        beginWT = WT_SHORT;

    if ((bT == LONG_BLOCK) || (bT == STOP_FLAT_BLOCK)) {
        endWT = WT_LONG;
    } else if (bT == START_FLAT_BLOCK)  {
        endWT = WT_FLAT;
    } else
        endWT = WT_SHORT;

    leng0 = windowLeng [beginWT];
    leng1 = windowLeng [endWT];

    switch(leng0 + leng1) {
    case 2048: IMDCT_Long(hDecoder, dataPtr); break;
    case 256: IMDCT_Short(hDecoder, dataPtr);
    }

    /*  first half of window */
    /*     windPtr = windowPtr [0] [0];  */
    windPtr = hDecoder->windowPtr [beginWT] [wnd_shape->prev_bk];

  /* idimkovic: should be optimized with SSE memcpy() */
    for (i = windowLeng [beginWT]/16 - 1; i>=0; --i)  {
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    }

    /*  second half of window */
    leng = windowLeng [endWT];
    windPtr = hDecoder->windowPtr [endWT] [wnd_shape->this_bk] + leng - 1;
  for (i = leng/16-1; i>=0; --i) {
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    }

    wnd_shape->prev_bk = wnd_shape->this_bk;
}

/*****************************************************************************
*
*   MDCT
*   window input sequence based on window type and perform MDCT
*   This is adapted from ITransformBlock().
*   (long term prediction needs this routine)
*   input: see below
*   output: see below
*   local static:
*     firstTime             flag = need to initialize data structures
*   globals: -
*
*****************************************************************************/
void TransformBlock(faacDecHandle hDecoder,
    Float* dataPtr,             /* time domain signal   */
    BLOCK_TYPE bT,              /* input: window type               */
    Wnd_Shape *wnd_shape
    )
{
    int leng0, leng1;
    int i,leng;
    Float *windPtr;
    WINDOW_TYPE beginWT, endWT;

    if((bT==LONG_BLOCK) || (bT==START_BLOCK) || (bT==START_FLAT_BLOCK)
        || (bT==START_ADV_BLOCK))  {
        beginWT = WT_LONG;
        }
    else if(bT==STOP_FLAT_BLOCK) {
        beginWT = WT_FLAT;
        }
    else if(bT==STOP_ADV_BLOCK) {
        beginWT = WT_ADV;
        }
    else
        beginWT = WT_SHORT;

    if ((bT == LONG_BLOCK) || (bT == STOP_BLOCK) || (bT == STOP_FLAT_BLOCK)
        || (bT == STOP_ADV_BLOCK)) {
        endWT = WT_LONG;
        }
    else if (bT == START_FLAT_BLOCK)  {
        endWT = WT_FLAT;
        }
    else if (bT == START_ADV_BLOCK)  {
        endWT = WT_ADV;
        }
    else
        endWT = WT_SHORT;

    leng0 = windowLeng[beginWT];
    leng1 = windowLeng[endWT];

/*  first half of window */
/*     windPtr = windowPtr [0] [0];  */
     windPtr = hDecoder->windowPtr [beginWT] [wnd_shape->prev_bk];

    for (i = windowLeng [beginWT]/16 - 1; i>=0; --i)  {
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    *dataPtr++ *= *windPtr++;  *dataPtr++ *= *windPtr++;
    }


/*  second half of window */
    leng = windowLeng[endWT];
    windPtr = hDecoder->windowPtr[endWT][wnd_shape->this_bk] + leng - 1;

    for (i = leng/16-1; i>=0; --i) {
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
      *dataPtr++ *= *windPtr--; *dataPtr++ *= *windPtr--;
    }

    dataPtr -= (windowLeng [beginWT] + leng);

    switch(leng0 + leng1) {
    case 2048: MDCT_Long(hDecoder, dataPtr); break;
    case 256: MDCT_Short(hDecoder, dataPtr);
    }
    wnd_shape->prev_bk = wnd_shape->this_bk;
}

