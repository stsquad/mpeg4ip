#ifndef __FILESTREAM_H__
#define __FILESTREAM_H__ 1

  
struct FILE_STREAM;

typedef int (*read_byte_t)(struct FILE_STREAM *fs, int *err);
typedef unsigned long (*filelength_t)(struct FILE_STREAM *fs);
typedef void (*seek_t)(struct FILE_STREAM *fs, unsigned long offset, int mode);
typedef void (*close_t)(struct FILE_STREAM *fs);
typedef int (*peek_t)(struct FILE_STREAM *fs, void *data, int length);

typedef struct FILE_STREAM {
	void *stream;
	int http; /* 0 if local file, 1 if an HTTP stream */
	unsigned char *data;
	int buffer_offset;
	int buffer_length;
	int file_offset;
  read_byte_t read_byte;
  filelength_t filelength;
  seek_t seek;
  close_t close;
  close_t reset;
  peek_t peek;
  void *userdata;
} FILE_STREAM;

#define LOCAL_BUFFER 1024 /* Buffer length for local files */
#define REMOTE_BUFFER 1024 * 64 /* Buffer length for http streams */
FILE_STREAM *open_filestream_yours (read_byte_t read_byte,
				    filelength_t filelength,
				    seek_t seek,
				    close_t close,
				    close_t reset,
				    peek_t peek,
				    void *userdata);

FILE_STREAM *open_filestream(const char *filename);
unsigned char read_byte_filestream(FILE_STREAM *fs);
int read_buffer_filestream(FILE_STREAM *fs, void *data, int length);
unsigned long filelength_filestream(FILE_STREAM *fs);
void close_filestream(FILE_STREAM *fs);
void seek_filestream(FILE_STREAM *fs, unsigned long offset, int mode);
unsigned long tell_filestream(FILE_STREAM *fs);
int peek_buffer_filestream(FILE_STREAM *fs, void *data, int length);
#endif

