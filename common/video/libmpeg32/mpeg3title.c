#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "mpeg3title.h"


#include <stdlib.h>


mpeg3_title_t* mpeg3_new_title(mpeg3_t *file, char *path)
{
	mpeg3_title_t *title = calloc(1, sizeof(mpeg3_title_t));
	title->fs = mpeg3_new_fs(path);
	title->file = file;
	return title;
}

int mpeg3_delete_title(mpeg3_title_t *title)
{
	mpeg3_delete_fs(title->fs);
	if(title->timecode_table_size)
	{
		free(title->timecode_table);
	}
	free(title);
	return 0;
}


int mpeg3_copy_title(mpeg3_title_t *dst, mpeg3_title_t *src)
{
	int i;

	mpeg3_copy_fs(dst->fs, src->fs);
	dst->total_bytes = src->total_bytes;

	if(src->timecode_table_size)
	{
		dst->timecode_table_allocation = src->timecode_table_allocation;
		dst->timecode_table_size = src->timecode_table_size;
		dst->timecode_table = calloc(1, sizeof(mpeg3demux_timecode_t) * dst->timecode_table_allocation);

		for(i = 0; i < dst->timecode_table_size; i++)
		{
			dst->timecode_table[i] = src->timecode_table[i];
		}
	}
	return 0;
}

int mpeg3_dump_title(mpeg3_title_t *title)
{
	int i;
	
	printf("mpeg3_dump_title path %s timecode_table_size %d\n", title->fs->path, title->timecode_table_size);
	for(i = 0; i < title->timecode_table_size; i++)
	{
		printf("%.02f: %x - %x %.02f %.02f %x\n", 
			title->timecode_table[i].absolute_start_time, 
			title->timecode_table[i].start_byte, 
			title->timecode_table[i].end_byte, 
			title->timecode_table[i].start_time, 
			title->timecode_table[i].end_time, 
			title->timecode_table[i].program);
	}
	return 0;
}


// Realloc doesn't work for some reason.
static void extend_timecode_table(mpeg3_title_t *title)
{
	if(!title->timecode_table || 
		title->timecode_table_allocation <= title->timecode_table_size)
	{
		long new_allocation;
		mpeg3demux_timecode_t *new_table;
		int i;

//printf("extend_timecode_table 1\n");
		new_allocation = title->timecode_table_allocation ? 
			title->timecode_table_size * 2 : 
			64;
		new_table = calloc(1, sizeof(mpeg3demux_timecode_t) * new_allocation);
//printf("extend_timecode_table 1\n");
		memcpy(new_table, 
			title->timecode_table, 
			sizeof(mpeg3demux_timecode_t) * title->timecode_table_allocation);
//printf("extend_timecode_table 1 %p %d %d\n", title->timecode_table, title->timecode_table_allocation,
//	(new_allocation - title->timecode_table_allocation));
		free(title->timecode_table);
		title->timecode_table = new_table;
//printf("extend_timecode_table 2\n");
		title->timecode_table_allocation = new_allocation;
//printf("extend_timecode_table 2\n");
	}
}

void mpeg3_new_timecode(mpeg3_title_t *title, 
		long start_byte, 
		double start_time,
		long end_byte,
		double end_time,
		int program)
{
	mpeg3demux_timecode_t *new_timecode;

	extend_timecode_table(title);
	new_timecode = &title->timecode_table[title->timecode_table_size];
	
	new_timecode->start_byte = start_byte;
	new_timecode->start_time = start_time;
	new_timecode->end_byte = end_byte;
	new_timecode->end_time = end_time;
	new_timecode->absolute_start_time = 0;
	new_timecode->program = program;
	title->timecode_table_size++;
}

mpeg3demux_timecode_t* mpeg3_append_timecode(mpeg3_demuxer_t *demuxer, 
		mpeg3_title_t *title, 
		long prev_byte, 
		double prev_time, 
		long start_byte, 
		double start_time,
		int dont_store,
		int program)
{
	mpeg3demux_timecode_t *new_timecode, *old_timecode;
	long i;

	extend_timecode_table(title);
/*
 * printf("mpeg3_append_timecode 1 %d %f %d %f %d %d\n", prev_byte, 
 * 		prev_time, 
 * 		start_byte, 
 * 		start_time,
 * 		dont_store,
 * 		program);
 */

	new_timecode = &title->timecode_table[title->timecode_table_size];
	if(!dont_store)
	{
		new_timecode->start_byte = start_byte;
		new_timecode->start_time = start_time;
		new_timecode->absolute_start_time = 0;

		if(title->timecode_table_size > 0)
		{
			old_timecode = &title->timecode_table[title->timecode_table_size - 1];
			old_timecode->end_byte = prev_byte;
			old_timecode->end_time = prev_time;
			new_timecode->absolute_start_time = 
				prev_time - 
				old_timecode->start_time + 
				old_timecode->absolute_start_time;
			new_timecode->absolute_end_time = start_time;
		}
	}

	title->timecode_table_size++;
	return new_timecode;
}

/* Create a title. */
/* Build a table of timecodes contained in the program stream. */
/* If toc is 0 just read the first and last timecode. */
int mpeg3demux_create_title(mpeg3_demuxer_t *demuxer, 
		int timecode_search, 
		FILE *toc)
{
	int result = 0, done = 0, counter_start, counter;
	mpeg3_t *file = demuxer->file;
	long next_byte, prev_byte;
	double next_time, prev_time, absolute_time;
	long i;
	mpeg3_title_t *title;
	u_int32_t test_header = 0;
	mpeg3demux_timecode_t *timecode = 0;

	demuxer->error_flag = 0;
	demuxer->read_all = 1;

/* Create a single title */
	if(!demuxer->total_titles)
	{
		demuxer->titles[0] = mpeg3_new_title(file, file->fs->path);
		demuxer->total_titles = 1;
		mpeg3demux_open_title(demuxer, 0);
	}

	title = demuxer->titles[0];
	title->total_bytes = mpeg3io_total_bytes(title->fs);








/* Get timecodes for the title */
	if(file->is_transport_stream || file->is_program_stream)
	{
		mpeg3io_seek(title->fs, 0);
//fprintf(stderr, "mpeg3demux_create_title: 0 %f %d %d\n", demuxer->time, result, mpeg3io_eof(title->fs));
		while(!done && !result && !mpeg3io_eof(title->fs))
		{
			next_byte = mpeg3io_tell(title->fs);
			result = mpeg3_read_next_packet(demuxer);
//printf("mpeg3demux_create_title: 1 %f %d\n", demuxer->time, result);

			if(!result)
			{
				next_time = demuxer->time;
//printf("timecode: %f %f %f\n", (double)next_time, (double)prev_time, (double)demuxer->time);
				if(next_time < prev_time || 
					next_time - prev_time > MPEG3_CONTIGUOUS_THRESHOLD ||
					!title->timecode_table_size)
				{
/* Discontinuous */
					timecode = mpeg3_append_timecode(demuxer, 
						title, 
						prev_byte, 
						prev_time, 
						next_byte, 
						next_time,
						0,
						0);
/*
 * printf("timecode: %ld %ld %f %f\n",
 * 				timecode->start_byte,
 * 				timecode->end_byte,
 * 				timecode->start_time,
 * 				timecode->end_time);
 */

					counter_start = next_time;
				}
				


// Breaks transport stream decoding	
// Kai Strieder
//				if (prev_time == next_time)
//				{
//					done = 1;
//				}


				prev_time = next_time;
				prev_byte = next_byte;
				counter = next_time;
			}

/* Just get the first bytes if not building a toc to get the stream ID's. */
			if(next_byte > 0x100000 && 
				(!timecode_search || !toc)) done = 1;
//printf("mpeg3demux_create_title 2 next_byte=%d next_time=%f done=%d result=%d %d\n", 
//	next_byte, next_time, done, result, mpeg3io_eof(title->fs));
		}

/* Get the last timecode */
		if(!toc || !timecode_search)
		{
			demuxer->read_all = 0;
			result = mpeg3io_seek(title->fs, title->total_bytes);
//			result = mpeg3io_seek(title->fs, title->total_bytes - 
//				(title->total_bytes % demuxer->packet_size));
//printf("mpeg3demux_create_title 3 %d\n", result);
			if(!result) result = mpeg3_read_prev_packet(demuxer);
		}

//fprintf(stderr, "mpeg3demux_create_title 4 %d %f\n", result, demuxer->time);
		if(title->timecode_table && timecode)
		{
			timecode->end_byte = title->total_bytes;
//			timecode->end_byte = mpeg3io_tell(title->fs)/*  + demuxer->packet_size */;
			timecode->end_time = demuxer->time;
			timecode->absolute_end_time = timecode->end_time - timecode->start_time;
		}
	}
//fprintf(stderr, "mpeg3demux_create_title 5 %d\n", result);

	mpeg3io_seek(title->fs, 0);
	demuxer->read_all = 0;
	return 0;
}

int mpeg3demux_print_timecodes(mpeg3_title_t *title, FILE *output)
{
	mpeg3demux_timecode_t *timecode;
	mpeg3_t *file = title->file;
	int i;

	if(title->timecode_table)
	{
		for(i = 0; i < title->timecode_table_size; i++)
		{
			timecode = &title->timecode_table[i];

			fprintf(output, "REGION: %ld %ld %f %f %d\n",
				timecode->start_byte,
				timecode->end_byte,
				timecode->start_time,
				timecode->end_time,
				timecode->program);
		}
	}
	return 0;
}

int mpeg3demux_print_streams(mpeg3_demuxer_t *demuxer, FILE *toc)
{
	int i;
/* Print the stream information */
	for(i = 0; i < MPEG3_MAX_STREAMS; i++)
	{
		if(demuxer->astream_table[i])
			fprintf(toc, "ASTREAM: %d %d\n", i, demuxer->astream_table[i]);

		if(demuxer->vstream_table[i])
			fprintf(toc, "VSTREAM: %d %d\n", i, demuxer->vstream_table[i]);
	}
	return 0;
}
