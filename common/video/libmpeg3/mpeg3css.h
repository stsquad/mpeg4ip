#ifndef MPEG3CSS_H
#define MPEG3CSS_H


#include "mpeg3private.inc"

struct mpeg3_block 
{
	unsigned char b[5];
};

struct mpeg3_playkey 
{
	int offset;
	unsigned char key[5];
};

typedef struct
{
	int encrypted;
	char device_path[MPEG3_STRLEN];    /* Device the file is located on */
	unsigned char disk_key[MPEG3_DVD_PACKET_SIZE];
	unsigned char title_key[5];
	char challenge[10];
	struct mpeg3_block key1;
	struct mpeg3_block key2;
	struct mpeg3_block keycheck;
	int varient;
	int fd;
	char path[MPEG3_STRLEN];
} mpeg3_css_t;

#endif
