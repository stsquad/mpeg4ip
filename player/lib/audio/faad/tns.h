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
 * $Id: tns.h,v 1.3 2001/06/28 23:54:22 wmaycisco Exp $
 */

#ifndef	_tns_h_
#define	_tns_h_

#define TNS_MAX_BANDS	49
#define TNS_MAX_ORDER	31
#define	TNS_MAX_WIN	8
#define TNS_MAX_FILT	3

typedef struct
{
    int start_band;
    int stop_band;
    int order;
    int direction;
    int coef_compress;
    int coef[TNS_MAX_ORDER];
} TNSfilt;

typedef struct 
{
    int n_filt;
    int coef_res;
    TNSfilt filt[TNS_MAX_FILT];
} TNSinfo;

typedef struct 
{
    int n_subblocks;
    TNSinfo info[TNS_MAX_WIN];
} TNS_frame_info;

#endif	/* _tns_h_ */
