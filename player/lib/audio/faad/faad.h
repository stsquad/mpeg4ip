#ifndef _AACDEC_H
#define _AACDEC_H
#include "filestream.h"
typedef struct {
	int channels;
	int sampling_rate;
	int bit_rate;
	int length;
} faadAACInfo;

int aac_decode_init_your_filestream(FILE_STREAM *fs);

FILE_STREAM *open_filestream_yours (read_byte_t read_byte,
				    filelength_t filelength,
				    seek_t seek,
				    close_t close,
				    close_t reset,
				    peek_t peek,
				    void *userdata);
int aac_decode_init_filestream(const char *fn);
int aac_decode_init(faadAACInfo *fInfo);
void aac_decode_free(void);
int  aac_decode_frame(short *sample_buffer);
int aac_get_sample_rate(void);
int aac_decode_seek(int pos_ms);

void CommonExit(int errorcode, char *message);
void CommonWarning(char *message);

#endif
