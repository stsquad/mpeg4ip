/*
 * The contents of most of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Robert Skegg
 */
#include "mp4live.h"

void video_filter_decimate (u_int8_t *pI, 
			    uint32_t final_width,
			    uint32_t final_height)
{
  uint32_t line, pair, sum;

  uint8_t *pO;

  /* We have a double-size image, field interlaced,
   * to convert to non-interlaced.
   * Drop alternate lines, average pairs of pixels. ####
   * The buffer is 4x size needed and raw data is read twice as
   * fast as converted data is output, so can use the same buffer.
   */
  pO = pI;
  // error_message("op-lines=%d ip-pix=%d pI=%d",final_height,final_width,(int)pI);
  /* step thru FINAL_HEIGHT output lines, averaging pix pairs */
  for(line = 0; line < final_height ; line++) {
    for(pair=0; pair < (final_width * 2); pair += 2) {   /* Step thru pairs input pixels */
      sum = *(pI + pair);
      sum += *(pI + pair + 1); /* sum */
      *pO++ = sum / 2; /* Avg */
    }
    pI += (final_width * 4);  /*jump that line and the next */
  }
  /* step thru output lines of V and U block */
  for(line = 0; line < final_height; line++) {
    for(pair=0;pair < final_width;pair += 2) {  /* Step thru pairs of bytes */
      sum = *(pI + pair);
      sum += *(pI + pair + 1); /* sum */
      *pO++ = sum / 2;  /* avg */
    }
    pI +=(final_width * 2);  /*jump that line and the next */
  }
}

/*
 * MPL license stops here.
 * Deinterlacing plugin - linear blend
 * Adapted by Robert Skegg from xawtv contributions, AUTHORS:
 * Conrad Kreyling <conrad@conrad.nerdland.org>
 * Patrick Barrett <yebyen@nerdland.org>
 *
 * This is licenced under the GNU GPL until someone tells me I'm stealing code
 * and can't do that ;) www.gnu.org for any version of the license.
 *
 * Based on xawtv-3.72/libng/plugins/flt-nop.c (also GPL)
 * Linear blend deinterlacing algorithm adapted from mplayer's libpostproc
 */

#ifdef USE_MMX
#define emms()                  __asm__ __volatile__ ("emms")
#else
#define emms()
#endif
/* This is the filter routine -----------------------------------*/
static inline void linearBlend(unsigned char *src, int stride)
{
#ifdef USE_MMX
#define PAVGB(a,b)  "pavgb " #a ", " #b " \n\t"
// error_message("mmx");
  asm volatile(
       "leal (%0, %1), %%eax                           \n\t"
       "leal (%%eax, %1, 4), %%ecx                     \n\t"

       "movq (%0), %%mm0                               \n\t" // L0
       "movq (%%eax, %1), %%mm1                        \n\t" // L2
       PAVGB(%%mm1, %%mm0)                                   // L0+L2
       "movq (%%eax), %%mm2                            \n\t" // L1
       PAVGB(%%mm2, %%mm0)
       "movq %%mm0, (%0)                               \n\t"
       "movq (%%eax, %1, 2), %%mm0                     \n\t" // L3
       PAVGB(%%mm0, %%mm2)                                   // L1+L3
       PAVGB(%%mm1, %%mm2)                                   // 2L2 + L1 + L3
       "movq %%mm2, (%%eax)                            \n\t"
       "movq (%0, %1, 4), %%mm2                        \n\t" // L4
       PAVGB(%%mm2, %%mm1)                                   // L2+L4
       PAVGB(%%mm0, %%mm1)                                   // 2L3 + L2 + L4
       "movq %%mm1, (%%eax, %1)                        \n\t"
       "movq (%%ecx), %%mm1                            \n\t" // L5
       PAVGB(%%mm1, %%mm0)                                   // L3+L5
       PAVGB(%%mm2, %%mm0)                                   // 2L4 + L3 + L5
       "movq %%mm0, (%%eax, %1, 2)                     \n\t"
       "movq (%%ecx, %1), %%mm0                        \n\t" // L6
       PAVGB(%%mm0, %%mm2)                                   // L4+L6
       PAVGB(%%mm1, %%mm2)                                   // 2L5 + L4 + L6
       "movq %%mm2, (%0, %1, 4)                        \n\t"
       "movq (%%ecx, %1, 2), %%mm2                     \n\t" // L7


      PAVGB(%%mm2, %%mm1)                                   // L5+L7
       PAVGB(%%mm0, %%mm1)                                   // 2L6 + L5 + L7
       "movq %%mm1, (%%ecx)                            \n\t"
       "movq (%0, %1, 8), %%mm1                        \n\t" // L8
       PAVGB(%%mm1, %%mm0)                                   // L6+L8
       PAVGB(%%mm2, %%mm0)                                   // 2L7 + L6 + L8
       "movq %%mm0, (%%ecx, %1)                        \n\t"
       "movq (%%ecx, %1, 4), %%mm0                     \n\t" // L9
       PAVGB(%%mm0, %%mm2)                                   // L7+L9
       PAVGB(%%mm1, %%mm2)                                   // 2L8 + L7 + L9
       "movq %%mm2, (%%ecx, %1, 2)                     \n\t"

       : : "r" (src), "r" (stride)
       : "%eax", "%ecx"
  );
  emms();
#else
  int x;
// error_message("c");
  for (x=0; x<8; x++)
  {
     src[0       ] = (src[0       ] + 2*src[stride  ] + src[stride*2])>>2;
     src[stride  ] = (src[stride  ] + 2*src[stride*2] + src[stride*3])>>2;
     src[stride*2] = (src[stride*2] + 2*src[stride*3] + src[stride*4])>>2;
     src[stride*3] = (src[stride*3] + 2*src[stride*4] + src[stride*5])>>2;
     src[stride*4] = (src[stride*4] + 2*src[stride*5] + src[stride*6])>>2;
     src[stride*5] = (src[stride*5] + 2*src[stride*6] + src[stride*7])>>2;
     src[stride*6] = (src[stride*6] + 2*src[stride*7] + src[stride*8])>>2;
     src[stride*7] = (src[stride*7] + 2*src[stride*8] + src[stride*9])>>2;

     src++;
  }
#endif
}

/* This is the frame processor for interlace filter -----------------*/

void
video_filter_interlace (u_int8_t* Start,  u_int8_t* End, u_int16_t Stride)
{
  unsigned int x, y, height;
  unsigned char *src;

  height = (End - Start) / Stride;
// error_message("A=%d X=%d Y=%d", (int)Start, (int)Stride, (int)height);
  for (y = 1; y < height - 8; y+=8)
  {
        for (x = 0; x < Stride; x+=8)
        {
            src = Start + x + (y * Stride);
            linearBlend(src, Stride);
       }
  }

  emms();
}

