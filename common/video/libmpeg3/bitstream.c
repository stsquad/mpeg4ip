#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <stdlib.h>

mpeg3_bits_t* mpeg3bits_new_stream(mpeg3_t *file, mpeg3_demuxer_t *demuxer)
{
	mpeg3_bits_t *stream = malloc(sizeof(mpeg3_bits_t));
	stream->bfr = 0;
	stream->bfr_size = 0;
	stream->bit_number = 0;
	stream->file = file;
	stream->demuxer = demuxer;
	stream->input_ptr = 0;
	return stream;
}

int mpeg3bits_delete_stream(mpeg3_bits_t* stream)
{
	free(stream);
	return 0;
}


/* Fill a buffer.  Only works if bit_number is on an 8 bit boundary */
int mpeg3bits_read_buffer(mpeg3_bits_t* stream, unsigned char *buffer, int bytes)
{
	int result, i = 0;
	while(stream->bit_number > 0)
	{
		stream->bit_number -= 8;
		mpeg3demux_read_prev_char(stream->demuxer);
	}

	stream->bit_number = 0;
	stream->bfr_size = 0;
	stream->bfr = 0;
	result = mpeg3demux_read_data(stream->demuxer, buffer, bytes);
	return result;
}

/* For mp3 decompression use a pointer in a buffer for getbits. */
int mpeg3bits_use_ptr(mpeg3_bits_t* stream, unsigned char *buffer)
{
	stream->bfr_size = stream->bit_number = 0;
	stream->bfr = 0;
	stream->input_ptr = buffer;
	return 0;
}

/* Go back to using the demuxer for getbits in mp3. */
int mpeg3bits_use_demuxer(mpeg3_bits_t* stream)
{
	if(stream->input_ptr)
	{
		stream->bfr_size = stream->bit_number = 0;
		stream->input_ptr = 0;
		stream->bfr = 0;
	}

	return 0;
}

/* Reconfigure for reverse operation */
/* Default is forward operation */
void mpeg3bits_start_reverse(mpeg3_bits_t* stream)
{
	int i;
	for(i = 0; i < stream->bfr_size; i += 8)
		if(stream->input_ptr)
			stream->input_ptr--;
		else
			mpeg3demux_read_prev_char(stream->demuxer);
}

/* Reconfigure for forward operation */
void mpeg3bits_start_forward(mpeg3_bits_t* stream)
{
	int i;
	for(i = 0; i < stream->bfr_size; i += 8)
		if(stream->input_ptr)
			stream->input_ptr++;
		else
			mpeg3demux_read_char(stream->demuxer);
}

/* Erase the buffer with the next 4 bytes in the file. */
int mpeg3bits_refill(mpeg3_bits_t* stream)
{
	stream->bit_number = 32;
	stream->bfr_size = 32;

	if(stream->input_ptr)
	{
		stream->bfr = (unsigned int)(*stream->input_ptr++) << 24;
		stream->bfr |= (unsigned int)(*stream->input_ptr++) << 16;
		stream->bfr |= (unsigned int)(*stream->input_ptr++) << 8;
		stream->bfr |= *stream->input_ptr++;
	}
	else
	{
		stream->bfr = mpeg3demux_read_char(stream->demuxer) << 24;
		stream->bfr |= mpeg3demux_read_char(stream->demuxer) << 16;
		stream->bfr |= mpeg3demux_read_char(stream->demuxer) << 8;
		stream->bfr |= mpeg3demux_read_char(stream->demuxer);
	}
	return mpeg3demux_eof(stream->demuxer);
}

/* Erase the buffer with the previous 4 bytes in the file. */
int mpeg3bits_refill_backwards(mpeg3_bits_t* stream)
{
	stream->bit_number = 0;
	stream->bfr_size = 32;
	stream->bfr = mpeg3demux_read_prev_char(stream->demuxer);
	stream->bfr |= (unsigned int)mpeg3demux_read_prev_char(stream->demuxer) << 8;
	stream->bfr |= (unsigned int)mpeg3demux_read_prev_char(stream->demuxer) << 16;
	stream->bfr |= (unsigned int)mpeg3demux_read_prev_char(stream->demuxer) << 24;
	return mpeg3demux_eof(stream->demuxer);
}

int mpeg3bits_byte_align(mpeg3_bits_t *stream)
{
	stream->bit_number = (stream->bit_number + 7) & 0xf8;
	return 0;
}

int mpeg3bits_seek_end(mpeg3_bits_t* stream)
{
	stream->bfr_size = stream->bit_number = 0;
	return mpeg3demux_seek_end(stream->demuxer);
}

int mpeg3bits_open_title(mpeg3_bits_t* stream, int title)
{
	stream->bfr_size = stream->bit_number = 0;
	return mpeg3demux_open_title(stream->demuxer, title);
}

int mpeg3bits_seek_start(mpeg3_bits_t* stream)
{
	stream->bfr_size = stream->bit_number = 0;
	return mpeg3demux_seek_start(stream->demuxer);
}

int mpeg3bits_seek_time(mpeg3_bits_t* stream, double time_position)
{
	stream->bfr_size = stream->bit_number = 0;
	return mpeg3demux_seek_time(stream->demuxer, time_position);
}

int mpeg3bits_seek_byte(mpeg3_bits_t* stream, int64_t position)
{
	stream->bfr_size = stream->bit_number = 0;
	return mpeg3demux_seek_byte(stream->demuxer, position);
}

int mpeg3bits_seek_percentage(mpeg3_bits_t* stream, double percentage)
{
	stream->bfr_size = stream->bit_number = 0;
	return mpeg3demux_seek_percentage(stream->demuxer, percentage);
}

int64_t mpeg3bits_tell(mpeg3_bits_t* stream)
{
	return mpeg3demux_tell(stream->demuxer);
}

int mpeg3bits_getbitoffset(mpeg3_bits_t *stream)
{
	return stream->bit_number & 7;
}

