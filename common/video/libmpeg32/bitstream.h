#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <systems.h>

//                                    next bit in forward direction
//                                  next bit in reverse direction |
//                                                              v v
// | | | | | | | | | | | | | | | | | | | | | | | | | | |1|1|1|1|1|1| */
//                                                     ^         ^
//                                                     |         bit_number = 1
//                                                     bfr_size = 6

typedef struct
{
	uint32_t bfr;  /* bfr = buffer for bits */
	int bit_number;   /* position of pointer in bfr */
	int bfr_size;    /* number of bits in bfr.  Should always be a multiple of 8 */
/* If the input ptr is true, data is read from it instead of the demuxer. */
	unsigned char *input_ptr;
  unsigned char *orig_ptr;
  long buflen;
} mpeg3_bits_t;

/* ======================================================================== */
/*                                 Entry Points */
/* ======================================================================== */

mpeg3_bits_t *mpeg3bits_new_stream(void);

static __inline int mpeg3bits_error (mpeg3_bits_t *stream)
{
  return 0;
}

static __inline int mpeg3bits_eof (mpeg3_bits_t *stream)
{
  long diff = stream->input_ptr - stream->orig_ptr;
  return (diff >= stream->buflen);
}

/* Read bytes backward from the file until the reverse_bits is full. */
static __inline void mpeg3bits_fill_reverse_bits(mpeg3_bits_t* stream, int bits)
{
// Right justify
	while(stream->bit_number > 7)
	{
		stream->bfr >>= 8;
		stream->bfr_size -= 8;
		stream->bit_number -= 8;
	}

// Insert bytes before bfr_size
	while(stream->bfr_size - stream->bit_number < bits)
	{
	    stream->bfr |= (unsigned int)(*--stream->input_ptr) << stream->bfr_size;
	    if (stream->input_ptr < stream->orig_ptr) {
	      printf("fill reverse past begin\n");
	    }
		stream->bfr_size += 8;
	}
}

/* Read bytes forward from the file until the forward_bits is full. */
static __inline void mpeg3bits_fill_bits(mpeg3_bits_t* stream, int bits)
{
	while(stream->bit_number < bits)
	{
		stream->bfr <<= 8;
		stream->bfr |= *stream->input_ptr++;
		stream->bit_number += 8;
		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;
	}
}

/* Return 8 bits, advancing the file position. */
static __inline unsigned int mpeg3bits_getbyte_noptr(mpeg3_bits_t* stream)
{
	if(stream->bit_number < 8)
	{
		stream->bfr <<= 8;
		stream->bfr |= *stream->input_ptr++;

		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;

		return (stream->bfr >> stream->bit_number) & 0xff;
	}
	return (stream->bfr >> (stream->bit_number -= 8)) & 0xff;
}

static __inline unsigned int mpeg3bits_getbit_noptr(mpeg3_bits_t* stream)
{
	if(!stream->bit_number)
	{
		stream->bfr <<= 8;
		stream->bfr |= *stream->input_ptr++;

		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;

		stream->bit_number = 7;

		return (stream->bfr >> 7) & 0x1;
	}
	return (stream->bfr >> (--stream->bit_number)) & (0x1);
}

/* Return n number of bits, advancing the file position. */
/* Use in place of flushbits */
static __inline unsigned int mpeg3bits_getbits(mpeg3_bits_t* stream, int bits)
{
	if(bits <= 0) return 0;
	mpeg3bits_fill_bits(stream, bits);
	return (stream->bfr >> (stream->bit_number -= bits)) & (0xffffffff >> (32 - bits));
}

static __inline unsigned int mpeg3bits_showbits24_noptr(mpeg3_bits_t* stream)
{
	while(stream->bit_number < 24)
	{
		stream->bfr <<= 8;
		stream->bfr |= *stream->input_ptr++;
		stream->bit_number += 8;
		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;
	}
	return (stream->bfr >> (stream->bit_number - 24)) & 0xffffff;
}

static __inline unsigned int mpeg3bits_showbits32_noptr(mpeg3_bits_t* stream)
{
	while(stream->bit_number < 32)
	{
		stream->bfr <<= 8;
		stream->bfr |= *stream->input_ptr++;
		stream->bit_number += 8;
		stream->bfr_size += 8;
		if(stream->bfr_size > 32) stream->bfr_size = 32;
	}
	return stream->bfr;
}

static __inline unsigned int mpeg3bits_showbits(mpeg3_bits_t* stream, int bits)
{
	mpeg3bits_fill_bits(stream, bits);
	return (stream->bfr >> (stream->bit_number - bits)) & (0xffffffff >> (32 - bits));
}

#if 0
static __inline unsigned int mpeg3bits_getbits_reverse(mpeg3_bits_t* stream, int bits)
{
	unsigned int result;
	mpeg3bits_fill_reverse_bits(stream, bits);
	result = (stream->bfr >> stream->bit_number) & (0xffffffff >> (32 - bits));
	stream->bit_number += bits;
	return result;
}

static __inline unsigned int mpeg3bits_showbits_reverse(mpeg3_bits_t* stream, int bits)
{
	unsigned int result;
	mpeg3bits_fill_reverse_bits(stream, bits);
	result = (stream->bfr >> stream->bit_number) & (0xffffffff >> (32 - bits));
	return result;
}

#endif
#ifdef __cplusplus
extern "C" {
#endif
int mpeg3bits_use_ptr_len(mpeg3_bits_t *stream, unsigned char *buffer,
			  long buflen);
#ifdef __cplusplus
}
#endif

#endif
