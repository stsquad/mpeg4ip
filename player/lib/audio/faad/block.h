/*
 * FAAD - Freeware Advanced Audio Decoder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: block.h,v 1.3 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifndef BLOCK_H
#define BLOCK_H 1

#define IN_DATATYPE  double
#define OUT_DATATYPE double

#ifndef BLOCK_LEN_LONG	 
#define BLOCK_LEN_LONG	 1024
#endif
#define BLOCK_LEN_SHORT  128


#define NWINLONG	(BLOCK_LEN_LONG)
#define ALFALONG	4.0
#define NWINSHORT	(BLOCK_LEN_SHORT)
#define ALFASHORT	7.0

#define	NWINFLAT	(NWINLONG)					/* flat params */
#define	NWINADV		(NWINLONG-NWINSHORT)		/* Advanced flat params */
#define NFLAT		((NWINFLAT-NWINSHORT)/2)
#define NADV0		((NWINADV-NWINSHORT)/2)

#define	START_OFFSET 0
#define	SHORT_IN_START_OFFSET 5
#define	STOP_OFFSET 3
#define SHORT_IN_STOP_OFFSET 0

#define WEAVE_START	0		/* type parameter for start blocks */
#define WEAVE_STOP	1		/* type paremeter for stop blocks */


typedef enum {
    WS_FHG, WS_DOLBY, N_WINDOW_SHAPES
} 
Window_shape;

typedef enum {
    WT_LONG, 
    WT_SHORT, 
    WT_FLAT, 
    WT_ADV,			/* Advanced flat window */
    N_WINDOW_TYPES
} 
WINDOW_TYPE;

typedef enum {                  /* ADVanced transform types */
    LONG_BLOCK,
    START_BLOCK,
    SHORT_BLOCK,
    STOP_BLOCK,
    START_ADV_BLOCK,
    STOP_ADV_BLOCK,
    START_FLAT_BLOCK,
    STOP_FLAT_BLOCK,
    N_BLOCK_TYPES
} 
BLOCK_TYPE;

typedef enum {  		/* Advanced window sequence (frame) types */
    ONLY_LONG,
    LONG_START, 
    LONG_STOP,
    SHORT_START, 
    SHORT_STOP,
    EIGHT_SHORT, 
    SHORT_EXT_STOP,
    NINE_SHORT,
    OLD_START,
    OLD_STOP,
    N_WINDOW_SEQUENCES
} 
WINDOW_SEQUENCE;

//wmay - made non-static
void unfold (Float *data_in, Float *data_out, int inLeng);
void InitBlock(void);
void EndBlock(void);
void ITransformBlock(faacDecHandle hDecoder,Float* dataPtr, BLOCK_TYPE wT, Wnd_Shape *ws, Float *state);
void TransformBlock(faacDecHandle hDecoder,Float* dataPtr, BLOCK_TYPE bT, Wnd_Shape *wnd_shape);

#endif	/*	BLOCK_H	*/
