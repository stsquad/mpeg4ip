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
 * $Id: monopred.h,v 1.1 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifndef _monopred_h_
#define _monopred_h_

#define MAX_PGRAD   2
#define MINVAR	    1
#define	Q_ZERO	    0x0000
#define	Q_ONE	    0x3F80

/* Predictor status information for mono predictor */
typedef struct {
    short	r[MAX_PGRAD];		/* contents of delay elements */
    short	kor[MAX_PGRAD];     	/* estimates of correlations */
    short	var[MAX_PGRAD];     	/* estimates of variances */
} PRED_STATUS;

typedef struct {
    float	r[MAX_PGRAD];		/* contents of delay elements */
    float	kor[MAX_PGRAD];     	/* estimates of correlations */
    float	var[MAX_PGRAD];     	/* estimates of variances */
} TMP_PRED_STATUS;

#endif /* _monopred_h_ */
