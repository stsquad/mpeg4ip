/* 
 *  gasmoff.c
 *
 *     Copyright (C) James Bowman - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */
#include <dv_types.h>

#include <stdio.h>

#define offsetof(S, M) \
    ((int)&(((S*)NULL)->M))

#define declare(S, M) \
    printf("#define %-40s %d\n", #S "_" #M, offsetof(S, M))
#define declaresize(S) \
    printf("#define %-40s %d\n", #S "_size", sizeof(S))
#define export(S) \
    printf("#define %-40s %d\n", #S, S)

int main(int argc, char *argv[])
{
  declare(dv_videosegment_t,	i);
  declare(dv_videosegment_t,	k);
  declare(dv_videosegment_t,	bs);
  declare(dv_videosegment_t,	mb);
  declare(dv_videosegment_t,	isPAL);

  declaresize(dv_macroblock_t);
  declare(dv_macroblock_t,	b);
  declare(dv_macroblock_t,	eob_count);
  declare(dv_macroblock_t,	vlc_error);
  declare(dv_macroblock_t,	qno);
  declare(dv_macroblock_t,	sta);
  declare(dv_macroblock_t,	i);
  declare(dv_macroblock_t,	j);
  declare(dv_macroblock_t,	k);

  declaresize(dv_block_t);
  declare(dv_block_t,		coeffs);
  declare(dv_block_t,		dct_mode);
  declare(dv_block_t,		class_no);
  declare(dv_block_t,		reorder);
  declare(dv_block_t,		reorder_sentinel);
  declare(dv_block_t,		offset);
  declare(dv_block_t,		end);
  declare(dv_block_t,		eob);
  declare(dv_block_t,		mark);

  declare(bitstream_t,	buf);

  export(DV_QUALITY_BEST);
  export(DV_QUALITY_FASTEST);
  export(DV_QUALITY_COLOR);
  export(DV_QUALITY_AC_MASK);
  export(DV_QUALITY_DC);
  export(DV_QUALITY_AC_1);
  export(DV_QUALITY_AC_2);
  export(DV_WEIGHT_BIAS);

  return 0;
}
