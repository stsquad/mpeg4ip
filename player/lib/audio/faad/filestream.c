#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "faad_all.h"
#include "filestream.h"

#ifdef WIN32 /* Streaming will only work on Win32 platforms */
#define HTTP_STREAMING
#endif

/* HTTP streaming stuff */
#ifdef HTTP_STREAMING
#include "http.h"
#endif

FILE_STREAM *open_filestream_yours (read_byte_t read_byte,
				    filelength_t filelength,
				    seek_t seek,
				    close_t close,
				    close_t reset,
				    peek_t peek,
				    void *userdata)
{
  FILE_STREAM *fs;
  fs = (FILE_STREAM *)malloc(sizeof(FILE_STREAM));
  if (fs == NULL) return (NULL);
  memset(fs, 0, sizeof(FILE_STREAM));
  fs->read_byte = read_byte;
  fs->filelength = filelength;
  fs->seek = seek;
  fs->close = close;
  fs->userdata = userdata;
  fs->reset = reset;
  fs->peek = peek;
  return (fs);
}
FILE_STREAM *open_filestream(const char *filename)
{
	FILE_STREAM *fs;

#ifdef HTTP_STREAMING
	if(strncmp(filename,"http://", 7) == 0)
	{
		fs = (FILE_STREAM *) malloc(sizeof(FILE_STREAM) + REMOTE_BUFFER);
		memset(fs, 0, sizeof(FILE_STREAM) + REMOTE_BUFFER);

		if(fs == NULL)
			return NULL;

		fs->data = (unsigned char *)&fs[1];

		if((fs->stream = http_file_open(filename)) == NULL)
		{
			free(fs);
			return NULL;
		}

		fs->http = 1;
	}
	else
	{
#endif
		fs = (FILE_STREAM *) malloc(sizeof(FILE_STREAM) + LOCAL_BUFFER);
		memset(fs, 0, sizeof(FILE_STREAM) + LOCAL_BUFFER);

		if(fs == NULL)
			return NULL;

		fs->data = (unsigned char *)&fs[1];

		if((fs->stream = (void *)fopen(filename, "rb")) == NULL)
		{
			free(fs);
			return NULL;
		}
#ifdef HTTP_STREAMING
	}
#endif

	return fs;
}

unsigned char read_byte_filestream(FILE_STREAM *fs)
{
  if (fs->read_byte) {
    return ((fs->read_byte)(fs, &stop_now));
  }
  if (stop_now == 1)
    return 0;
	if(fs->buffer_offset == fs->buffer_length)
	{
		fs->buffer_offset = 0;

#ifdef HTTP_STREAMING
		if(fs->http)
			fs->buffer_length = http_file_read(fs->stream, fs->data, REMOTE_BUFFER);
		else
#endif
			fs->buffer_length = fread(fs->data, 1, LOCAL_BUFFER, (FILE *)fs->stream);

		if(fs->buffer_length <= 0)
		{
			fs->buffer_length = 0;
			stop_now = 1;
			return (unsigned char)-1;
		}
	}

	fs->file_offset++;

	return fs->data[fs->buffer_offset++];
}

int read_buffer_filestream(FILE_STREAM *fs, void *data, int length)
{
	int i;
	int read;
	unsigned char *data2 = (unsigned char *)data;

	for(i=0; i < length; i++)
	{
	  read = read_byte_filestream(fs);
	  data2[i] = read & 0xff;
	  if (read < 0) 
		{
			if(i)
			{
				break;
			}
			else
			{
				return -1;
			}
		}
	}

	return i;
}

unsigned long filelength_filestream(FILE_STREAM *fs)
{
	fpos_t curpos;
	unsigned long fsize;

	if (fs->filelength) {
	  return ((fs->filelength)(fs));
	}
#ifdef HTTP_STREAMING
	if(fs->http)
	{
		return http_file_length(fs->stream);
	}
#endif

	fgetpos((FILE *) fs->stream, &curpos);
	fseek((FILE *) fs->stream, 0L, SEEK_END);
	fsize = ftell((FILE *) fs->stream);
	fsetpos((FILE *) fs->stream, &curpos);
	
	return fsize;
}

void seek_filestream(FILE_STREAM *fs, unsigned long offset, int mode)
{
  if (fs->seek) {
    (fs->seek)(fs, offset, mode);
    return;
  }
#ifdef HTTP_STREAMING
	if(fs->http)
	{
		return;
	}
#endif
	fs->buffer_length = 0;
	fs->buffer_offset = 0;

	fseek((FILE *)fs->stream, offset, mode);
	fs->file_offset = ftell((FILE *)fs->stream);
}

unsigned long tell_filestream(FILE_STREAM *fs)
{
	return fs->file_offset;
}

void close_filestream(FILE_STREAM *fs)
{
	if(fs)
	{
	  if (fs->close) {
	    (fs->close)(fs);
	  } else {
		if(fs->stream)
		{
#ifdef HTTP_STREAMING
			if(fs->http)
				http_file_close(fs->stream);
			else
#endif
				fclose((FILE *) fs->stream);
		}
	  }
		free(fs);
		fs = NULL;
	}
}

int peek_buffer_filestream (FILE_STREAM *fs, void *data, int length)
{
  unsigned long current_pos;
  int ret;
  if (fs->peek) {
    return ((fs->peek)(fs, data, length));
  }
  current_pos = tell_filestream(fs);
  ret = read_buffer_filestream(fs, data, length);
  seek_filestream(fs, current_pos, SEEK_SET);
  return ret;
}
