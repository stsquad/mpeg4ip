/* 
 *  dovlc.c
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
#include "bitstream.h"

int main(int argc, char **argv)
{
  int c, bit, bits_left, count, coeff_count;
  dv_vlc_t vlc;
  unsigned char *p;
  static unsigned char buffer[256];
  bitstream_t *bs;
  

  p = buffer;
  bits_left = 8;
  dv_construct_vlc_table();
  bs = bitstream_init();
  coeff_count=count=0;
  while((c=fgetc(stdin)) != EOF) {
    if(c=='0') bit = 0;
    else if(c=='1') bit = 1;
    else continue;
    count++;
    *p |= (bit << (bits_left -1));
    bits_left--;
    if(!bits_left) { p++; bits_left=8; }
  }
  bitstream_new_buffer(bs,buffer,256);
  while(count >0) {
    dv_peek_vlc(bs,count,&vlc);
    if(vlc.len > 0) {
      printf("(%d,%d,%d)",vlc.run,vlc.amp,vlc.len);
      bitstream_flush(bs,vlc.len);
      count-=vlc.len;
      coeff_count+=(vlc.run+1);
    } else if(vlc.len == VLC_ERROR) {
      printf("X (%d tossed)\n", count);
      exit(-1);
    } else if(vlc.len == VLC_NOBITS) {
      printf("* (%d tossed)\n", count);
      printf("*coeff_count=%d \n",coeff_count);
      exit(0);
    } else if(vlc.len == 0) {
      printf("#");
      bitstream_flush(bs,4);
      count-=4;
      coeff_count=0;
    } else {
      printf("danger! unknown return from vlc!\n");
      exit(-2);
    }
  }
  exit(0);
}
