#ifndef MPEG3IO_H
#define MPEG3IO_H

#include <mpeg4ip.h>
#include "mpeg3css.h"
#include "mpeg3privateinc.h"

/* Filesystem structure */
/* We buffer in MPEG3_IO_SIZE buffers.  Stream IO would require back */
/* buffering a buffer since the stream must be rewound for packet headers, */
/* sequence start codes, format parsing, decryption, and mpeg3cat. */



typedef struct
{
	FILE *fd;
	mpeg3_css_t *css;          /* Encryption object */
	char path[MPEG3_STRLEN];
	unsigned char *buffer;   /* Readahead buffer */
	int64_t buffer_offset;      /* Current buffer position */
	int64_t buffer_size;        /* Bytes in buffer */
	int64_t buffer_position;    /* Byte in file of start of buffer */

/* Hypothetical position of file pointer */
	int64_t current_byte;
	int64_t total_bytes;
} mpeg3_fs_t;




#define mpeg3io_tell(fs) (((mpeg3_fs_t *)(fs))->current_byte)

// End of file
#define mpeg3io_eof(fs) (((mpeg3_fs_t *)(fs))->current_byte >= ((mpeg3_fs_t *)(fs))->total_bytes)

// Beginning of file
#define mpeg3io_bof(fs)	(((mpeg3_fs_t *)(fs))->current_byte < 0)

#define mpeg3io_get_fd(fs) (fileno(((mpeg3_fs_t *)(fs))->fd))

#define mpeg3io_total_bytes(fs) (((mpeg3_fs_t *)(fs))->total_bytes)

static __inline int mpeg3io_sync_buffer(mpeg3_fs_t *fs)
{
	if(fs->buffer_position + fs->buffer_offset != fs->current_byte)
// Reposition buffer offset
	{
		fs->buffer_offset = fs->current_byte - fs->buffer_position;
	}

// Load new buffer
	if(fs->current_byte < fs->buffer_position ||
		fs->current_byte >= fs->buffer_position + fs->buffer_size)
	{
// Sequential reverse buffer
		if(fs->current_byte == fs->buffer_position - 1)
		{
//printf("mpeg3io_sync_buffer 1 %x %x\n", fs->current_byte, fs->buffer_position);
			fs->buffer_position = fs->current_byte - MPEG3_IO_SIZE + 1;
			if(fs->buffer_position < 0) fs->buffer_position = 0;

			fs->buffer_size = fs->current_byte - fs->buffer_position + 1;
			fs->buffer_offset = fs->buffer_size - 1;

			fseek(fs->fd, fs->buffer_position, SEEK_SET);
			fs->buffer_size = fread(fs->buffer, 1, fs->buffer_size, fs->fd);
		}
		else
// Sequential forward buffer or random seek
		{
//printf("mpeg3io_sync_buffer 2 %x %x\n", fs->current_byte, fs->buffer_position);
			fs->buffer_position = fs->current_byte;
			fs->buffer_offset = 0;
			fseek(fs->fd, fs->buffer_position, SEEK_SET);
			fs->buffer_size = fread(fs->buffer, 1, MPEG3_IO_SIZE, fs->fd);
		}
	}
	
	return !fs->buffer_size;
}

static unsigned int mpeg3io_read_char(mpeg3_fs_t *fs)
{
	unsigned int result;
	mpeg3io_sync_buffer(fs);
	result = fs->buffer[fs->buffer_offset++];
	fs->current_byte++;
	return result;
}

static __inline unsigned char mpeg3io_next_char(mpeg3_fs_t *fs)
{
	unsigned char result;
	mpeg3io_sync_buffer(fs);
	result = fs->buffer[fs->buffer_offset];
	return result;
}

static __inline uint32_t mpeg3io_read_int32(mpeg3_fs_t *fs)
{
	int a, b, c, d;
	uint32_t result;
/* Do not fread.  This breaks byte ordering. */
	a = mpeg3io_read_char(fs);
	b = mpeg3io_read_char(fs);
	c = mpeg3io_read_char(fs);
	d = mpeg3io_read_char(fs);
	result = (a << 24) |
					(b << 16) |
					(c << 8) |
					(d);
	return result;
}

static __inline uint32_t mpeg3io_read_int24(mpeg3_fs_t *fs)
{
	int b, c, d;
	uint32_t result;
/* Do not fread.  This breaks byte ordering. */
	b = mpeg3io_read_char(fs);
	c = mpeg3io_read_char(fs);
	d = mpeg3io_read_char(fs);
	result = (b << 16) |
					(c << 8) |
					(d);
	return result;
}

static __inline uint16_t mpeg3io_read_int16(mpeg3_fs_t *fs)
{
	int c, d;
	uint16_t result;
/* Do not fread.  This breaks byte ordering. */
	c = mpeg3io_read_char(fs);
	d = mpeg3io_read_char(fs);
	result = (c << 8) |
					(d);
	return result;
}

#endif
