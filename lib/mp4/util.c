#include <time.h>
#include "quicktime.h"

/* Disk I/O */


/* Read entire buffer from the preload buffer */
int quicktime_read_preload(quicktime_t *file, char *data, int size)
{
	long selection_start = file->file_position;
	long selection_end = file->file_position + size;
	long fragment_start, fragment_len, fragment_end;

	fragment_start = file->preload_ptr + (selection_start - file->preload_start);
	while(fragment_start < 0) fragment_start += file->preload_size;
	while(fragment_start >= file->preload_size) fragment_start -= file->preload_size;

	while(selection_start < selection_end)
	{
		fragment_len = selection_end - selection_start;
		if(fragment_start + fragment_len > file->preload_size)
			fragment_len = file->preload_size - fragment_start;
		fragment_end = fragment_start + fragment_len;

		while(fragment_start < fragment_end)
		{
			*data++ = file->preload_buffer[fragment_start++];
		}
		if(fragment_start >= file->preload_size) fragment_start = 0;
		selection_start += fragment_len;
	}
	return 0;
}


int quicktime_read_data(quicktime_t *file, char *data, int size)
{
	int result = 1;
	if(!file->preload_size)
	{
		if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
		result = fread(data, size, 1, file->stream);
	}
	else
	{
		long selection_start = file->file_position;
		long selection_end = file->file_position + size;
		long fragment_start, fragment_len, fragment_end;

		if(selection_end - selection_start > file->preload_size)
		{
/* Size is larger than preload size.  Should never happen. */
			if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
			result = fread(data, size, 1, file->stream);
		}
		else
		if(selection_start >= file->preload_start && 
			selection_start < file->preload_end &&
			selection_end <= file->preload_end &&
			selection_end > file->preload_start)
		{
/* Entire range is in buffer */
			quicktime_read_preload(file, data, size);
		}
		else
		if(selection_end > file->preload_end && selection_end - file->preload_size < file->preload_end)
		{
/* Range is after buffer */
/* Move the preload start to within one preload length of the selection_end */
			while(selection_end - file->preload_start > file->preload_size)
			{
				fragment_len = selection_end - file->preload_start - file->preload_size;
				if(file->preload_ptr + fragment_len > file->preload_size) fragment_len = file->preload_size - file->preload_ptr;
				file->preload_start += fragment_len;
				file->preload_ptr += fragment_len;
				if(file->preload_ptr >= file->preload_size) file->preload_ptr = 0;
			}

/* Append sequential data after the preload end to the new end */
			fragment_start = file->preload_ptr + file->preload_end - file->preload_start;
			while(fragment_start >= file->preload_size) fragment_start -= file->preload_size;

			while(file->preload_end < selection_end)
			{
				fragment_len = selection_end - file->preload_end;
				if(fragment_start + fragment_len > file->preload_size) fragment_len = file->preload_size - fragment_start;
				if(ftell(file->stream) != file->preload_end) fseek(file->stream, file->preload_end, SEEK_SET);
				result = fread(&(file->preload_buffer[fragment_start]), fragment_len, 1, file->stream);
				file->preload_end += fragment_len;
				fragment_start += fragment_len;
				if(fragment_start >= file->preload_size) fragment_start = 0;
			}

			quicktime_read_preload(file, data, size);
		}
		else
		{
/*printf("quicktime_read_data 4 selection_start %ld selection_end %ld preload_start %ld\n", selection_start, selection_end, file->preload_start); */
/* Range is before buffer or over a preload_size away from the end of the buffer. */
/* Replace entire preload buffer with range. */
			if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
			result = fread(file->preload_buffer, size, 1, file->stream);
			file->preload_start = file->file_position;
			file->preload_end = file->file_position + size;
			file->preload_ptr = 0;
			quicktime_read_preload(file, data, size);
		}
	}

	file->file_position += size;
	return result;
}

int quicktime_write_data(quicktime_t *file, char *data, int size)
{
	int result;
	if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
	result = fwrite(data, size, 1, file->stream);
	file->file_position += size;
	return result;
}

int quicktime_test_position(quicktime_t *file)
{
	if (quicktime_position(file) < 0)
	{
		printf("quicktime_test_position: 32 bit overflow\n");
		return 1;
	}
	else
	return 0;
}

int quicktime_read_pascal(quicktime_t *file, char *data)
{
	char len = quicktime_read_char(file);
	quicktime_read_data(file, data, len);
	data[len] = 0;
}

int quicktime_write_pascal(quicktime_t *file, char *data)
{
	char len = strlen(data);
	quicktime_write_data(file, &len, 1);
	quicktime_write_data(file, data, len);
}

float quicktime_read_fixed32(quicktime_t *file)
{
	unsigned long a, b, c, d;
	unsigned char data[4];

	quicktime_read_data(file, data, 4);
/*	fread(data, 4, 1, file->stream); */
	a = data[0];
	b = data[1];
	c = data[2];
	d = data[3];
	
	a = (a << 8) + b;
	b = (c << 8) + d;

	return (float)a + (float)b / 65536;
}

int quicktime_write_fixed32(quicktime_t *file, float number)
{
	unsigned char data[4];
	int a, b;

	a = number;
	b = (number - a) * 65536;
	data[0] = a >> 8;
	data[1] = a & 0xff;
	data[2] = b >> 8;
	data[3] = b & 0xff;

	return quicktime_write_data(file, data, 4);
}

int quicktime_write_int64(quicktime_t *file, u_int64_t value)
{
	unsigned char data[8];
	int i;

	for (i = 7; i >= 0; i--) {
		data[i] = value & 0xff;
		value >>= 8;
	}

	return quicktime_write_data(file, data, 8);
}

int quicktime_write_int32(quicktime_t *file, long value)
{
	unsigned char data[4];

	data[0] = (value & 0xff000000) >> 24;
	data[1] = (value & 0xff0000) >> 16;
	data[2] = (value & 0xff00) >> 8;
	data[3] = value & 0xff;

	return quicktime_write_data(file, data, 4);
}

int quicktime_write_char32(quicktime_t *file, char *string)
{
	return quicktime_write_data(file, string, 4);
}


float quicktime_read_fixed16(quicktime_t *file)
{
	unsigned char data[2];
	
	quicktime_read_data(file, data, 2);
	return (float)data[0] + (float)data[1] / 256;
}

int quicktime_write_fixed16(quicktime_t *file, float number)
{
	unsigned char data[2];
	int a, b;

	a = number;
	b = (number - a) * 256;
	data[0] = a;
	data[1] = b;
	
	return quicktime_write_data(file, data, 2);
}

u_int64_t quicktime_read_int64(quicktime_t *file)
{
	u_char data[8];
	u_int64_t result = 0;
	int i;

	quicktime_read_data(file, data, 8);

	for (i = 0; i < 8; i++) {
		result |= ((u_int64_t)data[i]) << ((7 - i) * 8);
	}

	return result;
}

long quicktime_read_int32(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b, c, d;
	char data[4];
	
	quicktime_read_data(file, data, 4);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (a<<24) | (b<<16) | (c<<8) | d;
	return (long)result;
}


long quicktime_read_int24(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b, c;
	char data[4];
	
	quicktime_read_data(file, data, 3);
/*	fread(data, 3, 1, file->stream); */
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];

	result = (a<<16) | (b<<8) | c;
	return (long)result;
}

int quicktime_write_int24(quicktime_t *file, long number)
{
	unsigned char data[3];
	data[0] = (number & 0xff0000) >> 16;
	data[1] = (number & 0xff00) >> 8;
	data[2] = (number & 0xff);
	
	return quicktime_write_data(file, data, 3);
/*	return fwrite(data, 3, 1, file->stream); */
}

int quicktime_read_int16(quicktime_t *file)
{
	unsigned long result;
	unsigned long a, b;
	char data[2];
	
	quicktime_read_data(file, data, 2);
/*	fread(data, 2, 1, file->stream); */
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];

	result = (a<<8) | b;
	return (int)result;
}

int quicktime_write_int16(quicktime_t *file, int number)
{
	unsigned char data[2];
	data[0] = (number & 0xff00) >> 8;
	data[1] = (number & 0xff);
	
	return quicktime_write_data(file, data, 2);
/*	return fwrite(data, 2, 1, file->stream); */
}

int quicktime_read_char(quicktime_t *file)
{
	char output;
	quicktime_read_data(file, &output, 1);
	return output;
}

int quicktime_write_char(quicktime_t *file, char x)
{
	return quicktime_write_data(file, &x, 1);
}

int quicktime_read_char32(quicktime_t *file, char *string)
{
	quicktime_read_data(file, string, 4);
/*	fread(string, 4, 1, file->stream); */
}

long quicktime_position(quicktime_t *file) 
{ 
	return file->file_position; 
}

int quicktime_set_position(quicktime_t *file, long position) 
{
	file->file_position = position;
	return 0;
/*	fseek(file->stream, position, SEEK_SET);  */
}

int quicktime_copy_char32(char *output, char *input)
{
	*output++ = *input++;
	*output++ = *input++;
	*output++ = *input++;
	*output = *input;
}


int quicktime_print_chars(char *desc, char *input, int len)
{
	int i;
	printf("%s", desc);
	for(i = 0; i < len; i++) printf("%c", input[i]);
	printf("\n");
}

unsigned long quicktime_current_time()
{
	time_t t;
	time (&t);
	return (t+(66*31536000)+1468800);
}

int quicktime_match_32(char *input, char *output)
{
	if(input[0] == output[0] &&
		input[1] == output[1] &&
		input[2] == output[2] &&
		input[3] == output[3])
		return 1;
	else 
		return 0;
}

int quicktime_read_mp4_descr_length(quicktime_t *file)
{
	u_int8_t b;
	u_int8_t numBytes = 0;
	u_int32_t length = 0;

	do {
		b = quicktime_read_char(file);
		numBytes++;
		length = (length << 7) | (b & 0x7F);
	} while ((b & 0x80) && numBytes < 4);

	return length;
}

int quicktime_write_mp4_descr_length(quicktime_t *file, int length, bool compact)
{
	u_int8_t b;
	int8_t i;
	int8_t numBytes;

	if (compact) {
		if (length <= 0x7F) {
			numBytes = 1;
		} else if (length <= 0x3FFF) {
			numBytes = 2;
		} else if (length <= 0x1FFFFF) {
			numBytes = 3;
		} else {
			numBytes = 4;
		}
	} else {
		numBytes = 4;
	}

	for (i = numBytes-1; i >= 0; i--) {
		b = (length >> (i * 7)) & 0x7F;
		if (i != 0) {
			b |= 0x80;
		}
		quicktime_write_char(file, b);
	}

	return numBytes; 
}

void quicktime_atom_hexdump(quicktime_t* file, quicktime_atom_t* atom)
{
	int i;
	int oldPos;

	oldPos = quicktime_position(file);
	quicktime_set_position(file, atom->start);
	printf("atom hex dump:\n");
	for (i = 0; i < atom->size; i++) {
		printf("%02x ", (u_int8_t)quicktime_read_char(file));
		if ((i % 16) == 0 && i > 0) {
			printf("\n");
		}
	}
	printf("\n");
	quicktime_set_position(file, oldPos);
}
