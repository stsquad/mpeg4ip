#include <math.h>
#include "dolby_def.h"
#include "dolby_win.h"
#include "transfo.h"

#ifndef PI
#define PI                   3.14159265359
#endif

Float	fhg_long [NWINLONG];
Float	fhg_short [NWINSHORT];
Float	fhg_edler [NWINLONG];
Float	dol_edler [NWINLONG];
Float	fhg_adv [NWINADV];
Float	dol_adv [NWINADV];

Float *windowPtr [N_WINDOW_TYPES] [N_WINDOW_SHAPES] = {
    {fhg_long,	dol_long},
    {fhg_short,	dol_short},
    {fhg_edler,	dol_edler},
    {fhg_adv,	dol_adv}
};	


int		windowLeng [N_WINDOW_TYPES] = {
	1024,
	128,
	1024,	
	1024
};


/*****************************************************************************
*
*	InitBlock
*	calculate windows for use by Window()
*	input: none
*	output: none
*	local static: none
*	globals: shortWindow[], longWindow[]
*
*****************************************************************************/
void InitBlock (void)
{

	/* calc half-window data */
    int		i, j;
    float	phaseInc;

	/* init half-windows */

	/* FhG long window */
    phaseInc = PI / (2.0 * NWINLONG);
    for (i = 0; i < NWINLONG; i++) {
		fhg_long [i]  = (float)sin(phaseInc * (i + 0.5));
	}

	/* FhG short window */
    phaseInc = PI / (2.0 * NWINSHORT);
    for (i = 0; i < NWINSHORT; i++) {
		fhg_short [i] = (float)sin(phaseInc * (i + 0.5));
	}

	/* Edler windows */
    for (i = 0, j = 0; i < NFLAT; i++, j++) {
		fhg_edler[j] = 0.0;
		dol_edler[j] = 0.0;
	}
    for (i = 0; i < NWINSHORT; i++, j++) {
		fhg_edler [j] = fhg_short [i];
		dol_edler [j] = dol_short [i];
	}
    for ( ; j < NWINFLAT; j++) {
		fhg_edler [j] = 1.0;
		dol_edler [j] = 1.0;
	}

	/* Advanced Edler windows */
    for (i = 0, j = 0; i < NADV0; i++, j++) {
		fhg_adv [j] = 0.0;
		dol_adv [j] = 0.0;
	}
    for (i = 0; i < NWINSHORT; i++, j++) {
		fhg_adv[j] = fhg_short[i];
		dol_adv[j] = dol_short[i];
	}
    for ( ; j < NWINADV; j++) {
		fhg_adv[j] = 1.0;
		dol_adv[j] = 1.0;
	}
}


/*****************************************************************************
*
*	Window
*	window input sequence based on window type
*	input: see below
*	output: see below
*	local static:
*	  firstTime				flag = need to initialize data structures
*	globals: shortWindow[], longWindow[]
*
*****************************************************************************/

void ITransformBlock (
	Float* dataPtr,				/* vector to be windowed in place	*/
	BLOCK_TYPE bT,				/* input: window type				*/
	Wnd_Shape *wnd_shape,
	Float *state				/* input/output						*/
	)
{
    static int	firstTime = 1;
	int leng0, leng1;
    int			i,leng;
    Float		*windPtr;
    WINDOW_TYPE	beginWT, endWT;

    if (firstTime)  {
		InitBlock();			/*	calculate windows	*/
		firstTime = 0;
	}

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

	IMDCT (dataPtr, leng0 + leng1);

	/*	first half of window */
	/*     windPtr = windowPtr [0] [0];  */
	windPtr = windowPtr [beginWT] [wnd_shape->prev_bk];  


    for (i = 0; i < windowLeng [beginWT]; i++)  {
		*dataPtr++ *= *windPtr++;
	}

	/*	second half of window */
    leng = windowLeng [endWT];
    windPtr = windowPtr [endWT] [wnd_shape->this_bk] + leng - 1;
    for (i = 0; i < leng; i++) {
		*dataPtr++ *= *windPtr--;
	}

	wnd_shape->prev_bk = wnd_shape->this_bk;
}

/*****************************************************************************
*
*	MDCT
*	window input sequence based on window type and perform MDCT
*	This is adapted from ITransformBlock().
*	(long term prediction needs this routine) 
*	input: see below
*	output: see below
*	local static:
*	  firstTime				flag = need to initialize data structures
*	globals: -
*
*****************************************************************************/
void TransformBlock (
	Float* dataPtr,				/* time domain signal	*/
	BLOCK_TYPE bT,				/* input: window type				*/
	Wnd_Shape *wnd_shape
	)
{
    static int	leng0, leng1;
    int			i,leng;
    Float		*windPtr;
    WINDOW_TYPE	beginWT, endWT;

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

    leng0 = windowLeng [beginWT];
    leng1 = windowLeng [endWT];

/*	first half of window */
/*     windPtr = windowPtr [0] [0];  */
     windPtr = windowPtr [beginWT] [wnd_shape->prev_bk];  


    for (i = 0; i < windowLeng [beginWT]; i++)  {
		*dataPtr++ *= *windPtr++;
		}

/*	second half of window */
    leng = windowLeng [endWT];
    windPtr = windowPtr [endWT] [wnd_shape->this_bk] + leng - 1;
    for (i = 0; i < leng; i++) {
		*dataPtr++ *= *windPtr--;
		}
    dataPtr -= (windowLeng [beginWT] + leng);

    MDCT(dataPtr, (leng0 + leng1));
	wnd_shape->prev_bk = wnd_shape->this_bk;
}

