
#include <stdio.h>
#include <stdlib.h>

#include "dolby_def.h"
#include "weave.h"
#include "block.h"

#define NORM_TYPE 0
#define START_TYPE 1
#define SHORT_TYPE 2
#define STOP_TYPE 3

extern int frame_cnt;

/*
*	Interleave Definitions for start and stop blocks
*
*	Start block contains 1 576-pt spectrum (A) and 4 128-pt spectra (B-E)
*	  Input spectra are interleaved in repeating segements of 17 bins,
*		9 bins from A (A0-A8), and 2 bins from each of the shorts.
*	  Within the segments the bins are interleaved as:
*		A0 A1 A2 A3 A4 B0 C0 D0 E0 A5 A6 A7 A8 B1 C1 D1 E1
*
*	Stop block contains 3 128-pt spectra (A-C) and 1 576-pt spectrum (D)
*	  Input spectra are interleaved in repeating segements of 15 bins,
*		2 bins from each of the shorts, and 9 bins from D (D0-D8).
*	  Within the segments the bins are interleaved as:
*		A0 B0 C0 D0 D1 D2 D3 D4 A1 B1 C1 D5 D6 D7 D8
*	  The last 64 bins are (should be) set to 0.
*/

#define N_SHORT_IN_START 4
#define	START_OFFSET 0
#define	SHORT_IN_START_OFFSET 5
#define N_SHORT_IN_STOP	3
#define	STOP_OFFSET 3
#define SHORT_IN_STOP_OFFSET 0
#define N_SHORT_IN_4STOP	4

/*****************************************************************************
*
*	unfold
*	create full spectrum by reflecting-inverting first half over to second
*	input: see below 
*	output: see below
*	local static: none
*	globals: none
*
*****************************************************************************/
void unfold ( 
	Float *data_in,	/* input: 1/2 spectrum */
	Float *data_out,	/* output: full spectrum */
	int inLeng)			/* input: length of input vector */
{
    register int   i;

    /* fill transBuff w/ full MDCT sequence from freqInPtr */
    for (i=0;i<inLeng;i++) {
		data_out[i] = *data_in;
		data_out[2*inLeng-i-1] = -(*data_in);
		data_in++;
    }
} /* end of unfold */


/*****************************************************************************
*
*	freq2time_adapt
*	transform freq. domain data to time domain.  
*	Overlap and add transform output to recreate time sequence.
*	Blocks composed of multiple segments (i.e. all but long) have 
*	  input spectrums interleaved.
*	input: see below
*	output: see below
*	local static:
*	  timeBuff		time domain data fifo
*	globals: none
*
*****************************************************************************/
void freq2time_adapt (

	byte blockType,			/* input: blockType 0-3						*/
	Wnd_Shape *wnd_shape, 	/* input/output								*/
	Float *freqInPtr, 		/* input: interleaved spectrum				*/
	Float *timeBuff, 		/* transform state needed for each channel	*/
	Float *ftimeOutPtr)		/* output: 1/2 block of new time values		*/
{
    static int		dolbyShortOffset = 1;
    static Float	transBuff [2*BLOCK_LEN_LONG], *transBuffPtr;
    int				i, j;
    Float			*timeBuffPtr, *destPtr;
    static Float	timeOutPtr[BLOCK_LEN_LONG];

    if (blockType == NORM_TYPE)  {
		unfold (freqInPtr, transBuff, BLOCK_LEN_LONG);
		/* Do 1 LONG transform */
		ITransformBlock (transBuff, LONG_BLOCK, wnd_shape, timeBuff);	/* ch ); */
/* Add first half and old data */
		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;		/*	 [ch];	*/
		destPtr = timeOutPtr;
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
		}
/* Save second half as old data */
		timeBuffPtr = timeBuff;		/*		 [ch];		*/
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*timeBuffPtr++ = *transBuffPtr++;
		}
    }

    else if (blockType == SHORT_TYPE)  {
		/* Do 8 SHORT transforms */

		if (dolbyShortOffset)
			destPtr = timeBuff + 4 * BLOCK_LEN_SHORT;		/* DBS */
		else
			destPtr = timeBuff + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2;	/*	448	*/

		for (i = 0; i < 8; i++) {
			unfold (freqInPtr, transBuff, BLOCK_LEN_SHORT );
			/*was freqinPtr++, 8 .. mfd */
			freqInPtr += BLOCK_LEN_SHORT;   /*  added mfd   */
			ITransformBlock (transBuff, SHORT_BLOCK, wnd_shape, timeBuff);

			/* Add first half of short window and old data */
			transBuffPtr = transBuff;
			for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
				*destPtr++ += *transBuffPtr++;
			}
			/* Save second half of short window */
			for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
				*destPtr++ = *transBuffPtr++;
			}
			destPtr -= BLOCK_LEN_SHORT;
		}
		/* Copy data to output buffer */
		destPtr = timeOutPtr;
		timeBuffPtr = timeBuff;		/*		 [ch];		*/
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *timeBuffPtr++;
		}
		/* Update timeBuff fifo */
		destPtr = timeBuff;		/*		 [ch];		*/
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *timeBuffPtr++;
		}
    }

    else if (blockType == START_TYPE)  {
		unfold(freqInPtr, transBuff, BLOCK_LEN_LONG);
		ITransformBlock (transBuff, START_FLAT_BLOCK, wnd_shape, timeBuff);
		/* Add first half and old data */
		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;
		destPtr = timeOutPtr;
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
		}
		/* Save second half as old data */
		timeBuffPtr = timeBuff;
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*timeBuffPtr++ = *transBuffPtr++;
		}
		dolbyShortOffset = 0;
    }

    else if (blockType == STOP_TYPE)  {
		unfold (freqInPtr, transBuff, BLOCK_LEN_LONG);
		/* Do 1 LONG transforms */
		ITransformBlock (transBuff, STOP_FLAT_BLOCK, wnd_shape, timeBuff);
		/* Add first half and old data */
		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;
		destPtr = timeOutPtr;
		for (i = 0; i < BLOCK_LEN_LONG - NFLAT; i++)  {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
		}
		for ( ; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *transBuffPtr++;
		}
		/* Save second half as old data */
		timeBuffPtr = timeBuff;
		for (i = 0; i < BLOCK_LEN_LONG; i++ )  {
			*timeBuffPtr++ = *transBuffPtr++;
		}
    }

    for (i = 0; i < BLOCK_LEN_LONG; i++)  {
		*(ftimeOutPtr++) = (timeOutPtr[i]);
    }
}

/*****************************************************************************
*
*	time2freq_adapt
*	transform to time domain data to freq. domain.  
*	Blocks composed of multiple segments (i.e. all but long) have 
*	  input spectrums interleaved.
*	Notice: currently verified only for certain blocktypes
*	input: see below
*	output: see below
*	local static:
*	  none
*	globals: none
*
*****************************************************************************/
void time2freq_adapt (

	byte blockType,			/* input: blockType 0-3						*/
	Wnd_Shape *wnd_shape, 	/* input/output								*/
	Float *timeInPtr, 		/* input: time domain data				*/
	Float *ffreqOutPtr)		/* output: 1/2 block of new freq values		*/ 
{
    static int		dolbyShortOffset = 1;
    static Float	transBuff [2*BLOCK_LEN_LONG], *transBuffPtr;
    int				i, j;
    Float			*srcPtr;
    Float			*destPtr;
    static Float	freqOutPtr[BLOCK_LEN_LONG];


#if !defined(DOLBYBS)
    /*	mapping from old FhG blocktypes to new window sequence types */

    switch (blockType)
    {
    case NORM_TYPE:
	blockType = ONLY_LONG;
	break;
    case START_TYPE:
	blockType = OLD_START;
	break;
    case SHORT_TYPE:
	blockType = EIGHT_SHORT;
	break;
    case STOP_TYPE:
	blockType = OLD_STOP;
	break;
    default:
	printf("dolby_adapt.c: Illegal block type %d - aborting\n",
	    blockType);
	exit(1);
	break;
    }
#endif





    if (blockType == ONLY_LONG)  {
		srcPtr = timeInPtr;
		destPtr = transBuff;
		for (i = 0; i < 2 * BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *srcPtr++;
		}
		/* Do 1 LONG transform */
		TransformBlock (transBuff, LONG_BLOCK, wnd_shape);

		srcPtr = transBuff;
		destPtr = freqOutPtr;
		for (i = 0; i < BLOCK_LEN_LONG; i++)  {
			*destPtr++ = *srcPtr++;
		}
    }

    else if (blockType == EIGHT_SHORT)  {
	/* Do 8 SHORT transforms */

        srcPtr = timeInPtr + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2;
	destPtr = freqOutPtr;

	for (i = 0; i < 8; i++) {
	    transBuffPtr = transBuff;
	    for (i = 0; i < 2 * BLOCK_LEN_SHORT; i++)  {
	      *transBuffPtr++ = *srcPtr++;
	    }
	    srcPtr -= BLOCK_LEN_SHORT;
	    TransformBlock (transBuff, SHORT_BLOCK, wnd_shape);

	    /* Copy data to output buffer */
	    transBuffPtr = transBuff;
	    for (j = 0; j < BLOCK_LEN_SHORT; j++)  {
		*destPtr++ = *transBuffPtr++;
	    }
	}
    }

    else if (blockType == OLD_START)  {
        srcPtr = timeInPtr;
	destPtr = transBuff;
	for (i = 0; i < 2 * BLOCK_LEN_LONG; i++)  {
	  *destPtr++ = *srcPtr++;
	}
	TransformBlock (transBuff, START_FLAT_BLOCK, wnd_shape);

	srcPtr = transBuff;
	destPtr = freqOutPtr;
	for (i = 0; i < BLOCK_LEN_LONG; i++)  {
	  *destPtr++ = *srcPtr++;
	}
	dolbyShortOffset = 0;
    }

    else if (blockType == OLD_STOP)  {
        srcPtr = timeInPtr;
	destPtr = transBuff;
	for (i = 0; i < 2 * BLOCK_LEN_LONG; i++)  {
	  *destPtr++ = *srcPtr++;
	}
	TransformBlock (transBuff, STOP_FLAT_BLOCK, wnd_shape);

	srcPtr = transBuff;
	destPtr = freqOutPtr;
	for (i = 0; i < BLOCK_LEN_LONG; i++)  {
	  *destPtr++ = *srcPtr++;
	}
    }

    else {
	printf( "Illegal Block_type %d in time2freq_adapt(), aborting ...\n", blockType );
	exit( 1 );
    }

    for (i = 0; i < BLOCK_LEN_LONG; i++)  {

	ffreqOutPtr [i] = (float) freqOutPtr [i];
	/*		ftimeOutPtr [i] = 1.;	*/

    }

}
