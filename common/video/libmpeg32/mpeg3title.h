#ifndef MPEG3TITLE_H
#define MPEG3TITLE_H

#include "mpeg3io.h"

// May get rid of time values and rename to a cell offset table.
// May also get rid of end byte.
typedef struct
{
	int64_t start_byte;
	double start_time;
	double absolute_start_time;
	double absolute_end_time;
	int64_t end_byte;
	double end_time;
	int program;
} mpeg3demux_timecode_t;

typedef struct
{
	void *file;
	mpeg3_fs_t *fs;
	int64_t total_bytes;     /* Total bytes in file.  Critical for seeking and length. */
/* Timecode table */
	mpeg3demux_timecode_t *timecode_table;
	long timecode_table_size;    /* Number of entries */
	long timecode_table_allocation;    /* Number of available slots */
} mpeg3_title_t;

#endif
