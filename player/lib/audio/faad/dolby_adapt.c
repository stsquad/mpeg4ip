#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "all.h"
#include "block.h"
#include "util.h"

#define NORM_TYPE 0
#define START_TYPE 1
#define SHORT_TYPE 2
#define STOP_TYPE 3

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
// wmay - made non-static
void unfold ( 
	Float *data_in,	/* input: 1/2 spectrum */
	Float *data_out,	/* output: full spectrum */
	int inLeng)			/* input: length of input vector */
{
    register int   i;

    /* fill transBuff w/ full MDCT sequence from freqInPtr */
    i=0;
    do
    {

      data_out[i] = *data_in;
      data_out[2*inLeng-i-1] = -(*data_in);
      data_in++;
      i++;

      data_out[i] = *data_in;
      data_out[2*inLeng-i-1] = -(*data_in);
      data_in++;
      i++;

      data_out[i] = *data_in;
      data_out[2*inLeng-i-1] = -(*data_in);
      data_in++;
      i++;

      data_out[i] = *data_in;
      data_out[2*inLeng-i-1] = -(*data_in);
      data_in++;
      i++;

    }while (i<inLeng);
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
void freq2time_adapt(faacDecHandle hDecoder,
	byte blockType,			/* input: blockType 0-3						*/
	Wnd_Shape *wnd_shape, 	/* input/output								*/
	Float *freqInPtr, 		/* input: interleaved spectrum				*/
	Float *timeBuff, 		/* transform state needed for each channel	*/
	Float *ftimeOutPtr)		/* output: 1/2 block of new time values		*/
{
    Float *transBuff, *transBuffPtr;
    int				i, j;
    Float			*timeBuffPtr, *destPtr;
    Float *timeOutPtr;

	transBuff = AllocMemory(2*BLOCK_LEN_LONG*sizeof(Float));
	timeOutPtr = AllocMemory(BLOCK_LEN_LONG*sizeof(Float));

    if (blockType == NORM_TYPE)  {
		unfold (freqInPtr, transBuff, BLOCK_LEN_LONG);
		/* Do 1 LONG transform */
		ITransformBlock (hDecoder, transBuff, LONG_BLOCK, wnd_shape, timeBuff);	/* ch ); */

		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;		/*	 [ch];	*/
		destPtr = timeOutPtr;

    /* idimkovic: reduce loop overhead by unrolling */
    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i)
    {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;

		}

    /* Save second half as old data */
		timeBuffPtr = timeBuff;		/*		 [ch];		*/
    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i)  {
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
		}
    }

    else if (blockType == SHORT_TYPE)
    {
		/* Do 8 SHORT transforms */

		if (hDecoder->dolbyShortOffset_f2t)
			destPtr = timeBuff + 4 * BLOCK_LEN_SHORT;		/* DBS */
		else
			destPtr = timeBuff + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2;	/*	448	*/

		for (i = 0; i < 8; i++) {
			unfold (freqInPtr, transBuff, BLOCK_LEN_SHORT );
			/*was freqinPtr++, 8 .. mfd */
			freqInPtr += BLOCK_LEN_SHORT;   /*  added mfd   */
			ITransformBlock (hDecoder, transBuff, SHORT_BLOCK, wnd_shape, timeBuff);

			/* Add first half of short window and old data */
			transBuffPtr = transBuff;

      for (j = BLOCK_LEN_SHORT/16-1; j >= 0; --j)  {
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
        *destPtr++ += *transBuffPtr++; *destPtr++ += *transBuffPtr++;
			}

			/* Save second half of short window */
      for (j = BLOCK_LEN_SHORT/16-1; j >= 0; --j)  {
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
        *destPtr++ = *transBuffPtr++;  *destPtr++ = *transBuffPtr++;
			}
			destPtr -= BLOCK_LEN_SHORT;
		}
		/* Copy data to output buffer */
		destPtr = timeOutPtr;
		timeBuffPtr = timeBuff;		/*		 [ch];		*/

    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i)  {
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
		}
		/* Update timeBuff fifo */
		destPtr = timeBuff;		/*		 [ch];		*/
    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i)  {
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
      *destPtr++ = *timeBuffPtr++; *destPtr++ = *timeBuffPtr++;
     }
    }

    else if (blockType == START_TYPE)  {
		unfold(freqInPtr, transBuff, BLOCK_LEN_LONG);
		ITransformBlock (hDecoder, transBuff, START_FLAT_BLOCK, wnd_shape, timeBuff);
		/* Add first half and old data */
		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;
		destPtr = timeOutPtr;
    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i)  {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
		}
		/* Save second half as old data */
		timeBuffPtr = timeBuff;
    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i)  {
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++;  *timeBuffPtr++ = *transBuffPtr++;
		}
		hDecoder->dolbyShortOffset_f2t = 0;
    }

    else if (blockType == STOP_TYPE)  {
		unfold (freqInPtr, transBuff, BLOCK_LEN_LONG);
		/* Do 1 LONG transforms */
		ITransformBlock (hDecoder, transBuff, STOP_FLAT_BLOCK, wnd_shape, timeBuff);
		/* Add first half and old data */
		transBuffPtr = transBuff;
		timeBuffPtr = timeBuff;
		destPtr = timeOutPtr;
    for (i = (BLOCK_LEN_LONG - NFLAT)/16 - 1; i>=0;--i)
    {
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
			*destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
      *destPtr++ = *transBuffPtr++ + *timeBuffPtr++;
    }
    for ( i = NFLAT/16-1; i>=0;--i)  {
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
      *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
		}

		/* Save second half as old data */
		timeBuffPtr = timeBuff;

    for (i = BLOCK_LEN_LONG/16 - 1; i >= 0; --i )  {
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
      *timeBuffPtr++ = *transBuffPtr++; *timeBuffPtr++ = *transBuffPtr++;
		}
    }

    i=0;    
    do
    {
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
      *(ftimeOutPtr++) = (timeOutPtr[i]); i++;
    } while(i<BLOCK_LEN_LONG);

	FreeMemory(timeOutPtr);
	FreeMemory(transBuff);
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
void time2freq_adapt(faacDecHandle hDecoder,
	WINDOW_TYPE blockType,			/* input: blockType 0-3						*/
	Wnd_Shape *wnd_shape, 	/* input/output								*/
	Float *timeInPtr, 		/* input: time domain data				*/
	Float *ffreqOutPtr)		/* output: 1/2 block of new freq values		*/ 
{
    Float *transBuff, *transBuffPtr;
    int i, j;
    Float *srcPtr;
    Float *destPtr;
    Float *freqOutPtr;

	transBuff = AllocMemory(2*BLOCK_LEN_LONG*sizeof(Float));
	freqOutPtr = AllocMemory(BLOCK_LEN_LONG*sizeof(Float));

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
      break;
    }
#endif





    if (blockType == ONLY_LONG)
    {
		srcPtr = timeInPtr;
		destPtr = transBuff;
    for (i = 2 * BLOCK_LEN_LONG / 16 - 1; i >= 0; --i)
    {
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
		}
		/* Do 1 LONG transform */
		TransformBlock (hDecoder, transBuff, LONG_BLOCK, wnd_shape);

		srcPtr = transBuff;
		destPtr = freqOutPtr;
    for (i = BLOCK_LEN_LONG/16-1; i>=0; --i)
    {
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
      *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
		}
    }

    else if (blockType == EIGHT_SHORT)  {
	/* Do 8 SHORT transforms */

        srcPtr = timeInPtr + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2;
	destPtr = freqOutPtr;

	for (i = 0; i < 8; i++) {
	    transBuffPtr = transBuff;
      for (i = 2 * BLOCK_LEN_SHORT/16-1; i>=0; --i)  {
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
        *transBuffPtr++ = *srcPtr++; *transBuffPtr++ = *srcPtr++;
	    }
	    srcPtr -= BLOCK_LEN_SHORT;
	    TransformBlock (hDecoder, transBuff, SHORT_BLOCK, wnd_shape);

	    /* Copy data to output buffer */
	    transBuffPtr = transBuff;
      for (j = BLOCK_LEN_SHORT/16-1; j>=0;--j)  {
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;
    *destPtr++ = *transBuffPtr++; *destPtr++ = *transBuffPtr++;

	    }
	}
    }

    else if (blockType == OLD_START)  {
        srcPtr = timeInPtr;
	destPtr = transBuff;
  for (i = 2 * BLOCK_LEN_LONG/16-1; i>=0;--i)  {
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;

	}
	TransformBlock (hDecoder, transBuff, START_FLAT_BLOCK, wnd_shape);

	srcPtr = transBuff;
	destPtr = freqOutPtr;
  for (i = BLOCK_LEN_LONG/16-1; i>=0;--i)  {
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
	}
	hDecoder->dolbyShortOffset_t2f = 0;
    }

    else if (blockType == OLD_STOP)  {
        srcPtr = timeInPtr;
	destPtr = transBuff;
  for (i = 2 * BLOCK_LEN_LONG/16-1; i>=0;--i)  {
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++; *destPtr++ = *srcPtr++;
	}
	TransformBlock (hDecoder, transBuff, STOP_FLAT_BLOCK, wnd_shape);

	srcPtr = transBuff;
	destPtr = freqOutPtr;
  for (i = BLOCK_LEN_LONG/16-1; i>=0;--i)  {
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;
    *destPtr++ = *srcPtr++;     *destPtr++ = *srcPtr++;

	}
    }

	FreeMemory(freqOutPtr);
	FreeMemory(transBuff);
}
