#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "bitstream.h"
#include <stdlib.h>

mpeg3_bits_t *mpeg3bits_new_stream(void)
{
	mpeg3_bits_t *stream = malloc(sizeof(mpeg3_bits_t));
	stream->bfr = 0;
	stream->bfr_size = 0;
	stream->bit_number = 0;
	stream->input_ptr = NULL;
	return stream;
}

int mpeg3bits_delete_stream(mpeg3_bits_t* stream)
{
	free(stream);
	return 0;
}

/* For mp3 decompression use a pointer in a buffer for getbits. */
int mpeg3bits_use_ptr(mpeg3_bits_t* stream, unsigned char *buffer)
{
	stream->bfr_size = stream->bit_number = 0;
	stream->bfr = 0;
	stream->input_ptr = buffer;
	stream->orig_ptr = buffer;
	stream->buflen = 65535;
	return 0;
}

int mpeg3bits_use_ptr_len(mpeg3_bits_t *stream, unsigned char *buffer,
			  long buflen)
{
  stream->bfr_size = stream->bit_number = 0;
  stream->bfr = 0;
  stream->input_ptr = buffer;
  stream->orig_ptr = buffer;
  stream->buflen = buflen;
  return 0;
}

/* Erase the buffer with the next 4 bytes in the file. */
int mpeg3bits_refill(mpeg3_bits_t* stream)
{
	stream->bit_number = 32;
	stream->bfr_size = 32;

	stream->bfr = (unsigned int)(*stream->input_ptr++) << 24;
	stream->bfr |= (unsigned int)(*stream->input_ptr++) << 16;
	stream->bfr |= (unsigned int)(*stream->input_ptr++) << 8;
	stream->bfr |= *stream->input_ptr++;

	return mpeg3bits_eof(stream);
}


int mpeg3bits_byte_align(mpeg3_bits_t *stream)
{
	stream->bit_number = (stream->bit_number + 7) & 0xf8;
	return 0;
}

int mpeg3bits_getbitoffset(mpeg3_bits_t *stream)
{
	return stream->bit_number & 7;
}

