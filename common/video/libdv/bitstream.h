/* 
 *  bitstream.h
 *
 *	Copyright (C) Aaron Holtzman - Dec 1999
 * 	Modified by Erik Walthinsen - Feb 2000
 *      Modified by Charles 'Buck' Krasic - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  This file was originally part of mpeg2dec, a free MPEG-2 video
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
 * */

#ifndef DV_BITSTREAM_H
#define DV_BITSTREAM_H

#include <dv_types.h>

#ifdef __cplusplus
extern "C" {
#endif

//My new and improved vego-matic endian swapping routine
//(stolen from the kernel)
#if defined(WORDS_BIGENDIAN)
#define swab32(x) (x)
#else // LITTLE_ENDIAN
#    define swab32(x)\
((((guint8*)&x)[0] << 24) | (((guint8*)&x)[1] << 16) |  \
 (((guint8*)&x)[2] << 8)  | (((guint8*)&x)[3]))
#endif // LITTLE_ENDIAN

bitstream_t *bitstream_init();
void bitstream_set_fill_func(bitstream_t *bs,guint32 (*next_function) (guint8 **,void *),void *priv);
void bitstream_next_buffer(bitstream_t * bs);
void bitstream_new_buffer(bitstream_t *bs,guint8 *buf,guint32 len);
void bitstream_byte_align(bitstream_t *bs);

static void bitstream_next_word(bitstream_t *bs) {
  guint32 diff = bs->buflen - bs->bufoffset;

  if (diff == 0)
    bitstream_next_buffer(bs);

  if ((bs->buflen - bs->bufoffset) >=4 ) {
    bs->next_word = *(gulong *)(bs->buf + bs->bufoffset);
    bs->next_word = swab32(bs->next_word);
    bs->next_bits = 32;
//    fprintf(stderr,"next_word is %08x at %d\n",bs->next_word,bs->bufoffset);
    bs->bufoffset += 4;
  } else {
    bs->next_word = *(gulong *)(bs->buf + bs->buflen - 4);
    bs->next_bits = (bs->buflen - bs->bufoffset) * 8;
//    fprintf(stdout,"end of buffer, have %d bits\n",bs->next_bits);
    bitstream_next_buffer(bs);
//    fprintf(stderr,"next_word is %08x at %d\n",bs->next_word,bs->bufoffset);
  }
}

//
// The fast paths for _show, _flush, and _get are in the
// bitstream.h header file so they can be inlined.
//
// The "bottom half" of these routines are suffixed _bh
//
// -ah
//

guint32 static inline bitstream_show_bh(bitstream_t *bs,guint32 num_bits) {
  guint32 result;

  result = (bs->current_word << (32 - bs->bits_left)) >> (32 - bs->bits_left);
  num_bits -= bs->bits_left;
  result = (result << num_bits) | (bs->next_word >> (32 - num_bits));

  return result;
}

guint32 static inline bitstream_get_bh(bitstream_t *bs,guint32 num_bits) {
  guint32 result;

  num_bits -= bs->bits_left;
  result = (bs->current_word << (32 - bs->bits_left)) >> (32 - bs->bits_left);

  if (num_bits != 0)
    result = (result << num_bits) | (bs->next_word >> (32 - num_bits));

  bs->current_word = bs->next_word;
  bs->bits_left = 32 - num_bits;
  bitstream_next_word(bs);

  return result;
}

void static inline bitstream_flush_bh(bitstream_t *bs,guint32 num_bits) {
  //fprintf(stderr,"(flush) current_word 0x%08x, next_word 0x%08x, bits_left %d, num_bits %d\n",current_word,next_word,bits_left,num_bits);

  bs->current_word = bs->next_word;
  bs->bits_left = (32 + bs->bits_left) - num_bits;
  bitstream_next_word(bs);
}

static inline guint32 bitstream_show(bitstream_t * bs, guint32 num_bits) {
  if (num_bits <= bs->bits_left)
    return (bs->current_word >> (bs->bits_left - num_bits));

  return bitstream_show_bh(bs,num_bits);
}

static inline guint32 bitstream_get(bitstream_t * bs, guint32 num_bits) {
  guint32 result;

  bs->bitsread += num_bits;

  if (num_bits < bs->bits_left) {
    result = (bs->current_word << (32 - bs->bits_left)) >> (32 - num_bits);
    bs->bits_left -= num_bits;
    return result;
  }

  return bitstream_get_bh(bs,num_bits);
}

// This will fail unpredictably if you try to put more than 32 bits back
static inline void bitstream_unget(bitstream_t *bs, guint32 data, guint8 num_bits)
{
  guint high_bits;
  guint32 mask = (1<<num_bits)-1;

  g_return_if_fail((num_bits <= 32) && (num_bits >0) && (!(data & (~mask))));

  bs->bitsread -= num_bits;
  if(num_bits <= (32 - bs->bits_left)) {
    bs->current_word &= (~(mask << bs->bits_left));
    bs->current_word |= (data << bs->bits_left);
    bs->bits_left += num_bits;
  } else if(bs->bits_left == 32) {
    bs->next_word = bs->current_word;
    bs->current_word = data;
    bs->bits_left = num_bits;
    bs->bufoffset -= 4;
  } else {
    high_bits = 32 - bs->bits_left;
    bs->next_word = 
      (data << bs->bits_left) | 
      ((bs->current_word << high_bits) >> high_bits);
    bs->current_word = (data >> high_bits);
    bs->bits_left = num_bits - high_bits;
    bs->bufoffset -= 4;
  }
}

static inline void bitstream_flush(bitstream_t * bs, guint32 num_bits) {
  if (num_bits < bs->bits_left)
    bs->bits_left -= num_bits;
  else
    bitstream_flush_bh(bs,num_bits);

  bs->bitsread += num_bits;
}

static inline void bitstream_flush_large(bitstream_t *bs,guint32 num_bits) {
  gint bits = num_bits;

  while (bits > 32) {
    bitstream_flush(bs,32);
    bits -= 32;
  }
  bitstream_flush(bs,bits);
}

static inline void bitstream_seek_set(bitstream_t *bs, guint32 offset) {
  bs->bufoffset = ((offset & (~0x1f)) >> 5) << 2;
  bs->current_word = *(gulong *)(bs->buf + bs->bufoffset);
  bs->current_word = swab32(bs->current_word);
  bs->bufoffset += 4;
  bs->next_word = *(gulong *)(bs->buf + bs->bufoffset);
  bs->next_word = swab32(bs->next_word);
  bs->bufoffset += 4;
  bs->bits_left = 32 - (offset & 0x1f);
  bs->next_bits = 32;
  bs->bitsread = offset;
} 


#ifdef __cplusplus
}
#endif

#endif /* DV_BITSTREAM_H */
