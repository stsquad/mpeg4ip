#ifndef MPEG3DEMUX_H
#define MPEG3DEMUX_H

#include "mpeg3title.h"
#include <stdio.h>

typedef struct
{
/* mpeg3_t */
	void* file;
/* One packet. MPEG3_RAW_SIZE allocated since we don't know the packet size */
	unsigned char *raw_data;
	long raw_offset;
/* Amount loaded in last raw_data */
	int raw_size;
/* One packet payload */
	unsigned char *data_buffer;
	long data_size;
	long data_position;
/* Only one is on depending on which track owns the demultiplexer. */
	int do_audio;
	int do_video;
/* Direction of reads */
	int reverse;
/* Set to 1 when eof or attempt to read before beginning */
	int error_flag;
/* Temp variables for returning */
	unsigned char next_char;
/* Correction factor for time discontinuity */
	double time_offset;
	int read_all;
/* Info for mpeg3cat */
	long last_packet_start;
	long last_packet_end;
	long last_packet_decryption;

/* Titles */
	mpeg3_title_t *titles[MPEG3_MAX_STREAMS];
	int total_titles;
	int current_title;

/* Tables of every stream ID encountered */
	int astream_table[MPEG3_MAX_STREAMS];  /* macro of audio format if audio  */
	int vstream_table[MPEG3_MAX_STREAMS];  /* 1 if video */

/* Programs */
	int total_programs;
	int current_program;

/* Timecode in the current title */
	int current_timecode;

/* Byte position in the current title */
	long current_byte;

	int transport_error_indicator;
	int payload_unit_start_indicator;
	int pid;
	int transport_scrambling_control;
	int adaptation_field_control;
	int continuity_counter;
	int is_padding;
	int pid_table[MPEG3_PIDMAX];
	int continuity_counters[MPEG3_PIDMAX];
	int total_pids;
	int adaptation_fields;
	double time;           /* Time in seconds */
	int audio_pid;
	int video_pid;
	int astream;     /* Video stream ID being decoded.  -1 = select first ID in stream */
	int vstream;     /* Audio stream ID being decoded.  -1 = select first ID in stream */
	int aformat;      /* format of the audio derived from multiplexing codes */
	long program_association_tables;
	int table_id;
	int section_length;
	int transport_stream_id;
	long pes_packets;
	double pes_audio_time;  /* Presentation Time stamps */
	double pes_video_time;  /* Presentation Time stamps */
} mpeg3_demuxer_t;

/* ========================================================================= */
/*                             Entry points */
/* ========================================================================= */

unsigned char mpeg3demux_read_char_packet(mpeg3_demuxer_t *demuxer);
unsigned char mpeg3demux_read_prev_char_packet(mpeg3_demuxer_t *demuxer);

#define mpeg3demux_error(demuxer) (((mpeg3_demuxer_t *)(demuxer))->error_flag)

#define mpeg3demux_time_offset(demuxer) (((mpeg3_demuxer_t *)(demuxer))->time_offset)

#define mpeg3demux_current_time(demuxer) (((mpeg3_demuxer_t *)(demuxer))->time + ((mpeg3_demuxer_t *)(demuxer))->time_offset)

static unsigned char mpeg3demux_read_char(mpeg3_demuxer_t *demuxer)
{
//printf("mpeg3demux_read_char %lx %lx\n", demuxer->data_position, demuxer->data_size);
	if(demuxer->data_position < demuxer->data_size)
	{
		return demuxer->data_buffer[demuxer->data_position++];
	}
	else
	{
		return mpeg3demux_read_char_packet(demuxer);
	}
}



static unsigned char mpeg3demux_read_prev_char(mpeg3_demuxer_t *demuxer)
{
	if(demuxer->data_position != 0)
	{
		return demuxer->data_buffer[demuxer->data_position--];
	}
	else
	{
		return mpeg3demux_read_prev_char_packet(demuxer);
	}
}


int mpeg3demux_eof(mpeg3_demuxer_t *demuxer);

#endif
