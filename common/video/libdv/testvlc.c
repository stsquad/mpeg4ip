/* 
 *  testvlc.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
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

#include "vlc.h"

typedef struct dv_vlc_test_s {
  gint8 run;
  gint16 amp;
  guint16 val;
  guint8 len;
} dv_vlc_test_t;

static dv_vlc_test_t dv_vlc_test_table[89] = {
  { 0, 1, 0x0, 2 },
  { 0, 2, 0x2, 3 },
  {-1, 0, 0x6, 4 },
  { 1, 1, 0x7, 4 },
  { 0, 3, 0x8, 4 },
  { 0, 4, 0x9, 4 },
  { 2, 1, 0x14, 5 },
  { 1, 2, 0x15, 5 },
  { 0, 5, 0x16, 5 },
  { 0, 6, 0x17, 5 },
  { 3, 1, 0x30, 6 },
  { 4, 1, 0x31, 6 },
  { 0, 7, 0x32, 6 },
  { 0, 8, 0x33, 6 },
  { 5, 1,  0x68, 7 },
  { 6, 1,  0x69, 7 },
  { 2, 2,  0x6a, 7 },
  { 1, 3,  0x6b, 7 },
  { 1, 4,  0x6c, 7 },
  { 0, 9,  0x6d, 7 },
  { 0, 10, 0x6e, 7 },
  { 0, 11, 0x6f, 7 },
  { 7,  1,  0xe0, 8 },
  { 8,  1,  0xe1, 8 },
  { 9,  1,  0xe2, 8 },
  { 10, 1,  0xe3, 8 },
  { 3,  2,  0xe4, 8 },
  { 4,  2,  0xe5, 8 },
  { 2,  3,  0xe6, 8 },
  { 1,  5,  0xe7, 8 },
  { 1,  6,  0xe8, 8 },
  { 1,  7,  0xe9, 8 },
  { 0,  12, 0xea, 8 },
  { 0,  13, 0xeb, 8 },
  { 0,  14, 0xec, 8 },
  { 0,  15, 0xed, 8 },
  { 0,  16, 0xee, 8 },
  { 0,  17, 0xef, 8 },
  { 11, 1,  0x1e0, 9 },
  { 12, 1,  0x1e1, 9 },
  { 13, 1,  0x1e2, 9 },
  { 14, 1,  0x1e3, 9 },
  { 5,  2,  0x1e4, 9 },
  { 6,  2,  0x1e5, 9 },
  { 3,  3,  0x1e6, 9 },
  { 4,  3,  0x1e7, 9 },
  { 2,  4,  0x1e8, 9 },
  { 2,  5,  0x1e9, 9 },
  { 1,  8,  0x1ea, 9 },
  { 0,  18, 0x1eb, 9 },
  { 0,  19, 0x1ec, 9 },
  { 0,  20, 0x1ed, 9 },
  { 0,  21, 0x1ee, 9 },
  { 0,  22, 0x1ef, 9 },
  { 5, 3,  0x3e0, 10 },
  { 3, 4,  0x3e1, 10 },
  { 3, 5,  0x3e2, 10 },
  { 2, 6,  0x3e3, 10 },
  { 1, 9,  0x3e4, 10 },
  { 1, 10, 0x3e5, 10 },
  { 1, 11, 0x3e6, 10 },
  { 0, 0,  0x7ce, 11 },
  { 1, 0,  0x7cf, 11 },
  { 6, 3,  0x7d0, 11 },
  { 4, 4,  0x7d1, 11 },
  { 3, 6,  0x7d2, 11 },
  { 1, 12, 0x7d3, 11 },
  { 1, 13, 0x7d4, 11 },
  { 1, 14, 0x7d5, 11 },
  { 2, 0, 0xfac, 12 },
  { 3, 0, 0xfad, 12 },
  { 4, 0, 0xfae, 12 },
  { 5, 0, 0xfaf, 12 },
  { 7,  2,  0xfb0, 12 },
  { 8,  2,  0xfb1, 12 },
  { 9,  2,  0xfb2, 12 },
  { 10, 2,  0xfb3, 12 },
  { 7,  3,  0xfb4, 12 },
  { 8,  3,  0xfb5, 12 },
  { 4,  5,  0xfb6, 12 },
  { 3,  7,  0xfb7, 12 },
  { 2,  7,  0xfb8, 12 },
  { 2,  8,  0xfb9, 12 },
  { 2,  9,  0xfba, 12 },
  { 2,  10, 0xfbb, 12 },
  { 2,  11, 0xfbc, 12 },
  { 1,  15, 0xfbd, 12 },
  { 1,  16, 0xfbe, 12 },
  { 1,  17, 0xfbf, 12 },
}; // dv_vlc_test_table

int main(int argc, char **argv)
{
  gint i;
  dv_vlc_t vlc;
  gint run, amp, len, val;

  dv_construct_vlc_table();
  for(i=0;i<89;i++) {
    val = dv_vlc_test_table[i].val;
    amp = dv_vlc_test_table[i].amp;
    len = dv_vlc_test_table[i].len;
    run = dv_vlc_test_table[i].run;
    if(amp > 0) { len++; val <<= 1; }
    dv_decode_vlc(val << (16 - len), len, &vlc);
    if (!(run == vlc.run && amp == vlc.amp && len == vlc.len)) {
      fprintf(stderr, "Failed at %d; expected (%d,%d,%d), found (%d,%d,%d)\n",
	      i,
	      run, amp, len,
	      vlc.run, vlc.amp, vlc.len);
    }
  }
  for(run=6; run < 62; run++) {
    len = 13;
    val = (126 << 6) | run;
    amp = 0;
    dv_decode_vlc(val << (16 - len), len, &vlc);
    if (!(run == vlc.run && amp == vlc.amp && len == vlc.len)) {
      fprintf(stderr, "Failed; expected (%d,%d,%d), found (%d,%d,%d)\n",
	      run, amp, len,
	      vlc.run, vlc.amp, vlc.len);
    }
    g_assert(run == vlc.run && amp == vlc.amp && len == vlc.len);
  }
  for(amp=23; amp < 256; amp++) {
    val = (127 << 9) | (amp << 1);
    run = 0;
    len = 16;
    dv_decode_vlc(val << (16 - len), len, &vlc);
    if (!(run == vlc.run && amp == vlc.amp && len == vlc.len)) {
      fprintf(stderr, "Failed; expected (%d,%d,%d), found (%d,%d,%d)\n",
	      run, amp, len,
	      vlc.run, vlc.amp, vlc.len);
    }
    g_assert(run == vlc.run && amp == vlc.amp && len == vlc.len);
    __dv_decode_vlc(val, &vlc);
    if (!(run == vlc.run && amp == vlc.amp && len == vlc.len)) {
      fprintf(stderr, "Failed; expected (%d,%d,%d), found (%d,%d,%d)\n",
	      run, amp, len,
	      vlc.run, vlc.amp, vlc.len);
    }
    g_assert(run == vlc.run && amp == vlc.amp && len == vlc.len);
  }
  exit(0);
}


