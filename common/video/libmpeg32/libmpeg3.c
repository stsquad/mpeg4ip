#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

mpeg3_t* mpeg3_new(const char *path)
{
	int i;
	mpeg3_t *file = calloc(1, sizeof(mpeg3_t));
	file->cpus = 1;
	file->fs = mpeg3_new_fs(path);
	file->demuxer = mpeg3_new_demuxer(file, 0, 0, -1);
	file->seekable = 1;
	return file;
}

int mpeg3_delete(mpeg3_t *file)
{
	int i;

	for(i = 0; i < file->total_vstreams; i++)
		mpeg3_delete_vtrack(file, file->vtrack[i]);

	for(i = 0; i < file->total_astreams; i++)
		mpeg3_delete_atrack(file, file->atrack[i]);

	mpeg3_delete_fs(file->fs);
	mpeg3_delete_demuxer(file->demuxer);

	if(file->frame_offsets)
	{
		for(i = 0; i < file->total_vstreams; i++)
		{
			free(file->frame_offsets[i]);
			free(file->keyframe_numbers[i]);
		}

		free(file->frame_offsets);
		free(file->keyframe_numbers);
		free(file->total_frame_offsets);
		free(file->total_keyframe_numbers);
	}

	if(file->sample_offsets)
	{
		for(i = 0; i < file->total_astreams; i++)
			free(file->sample_offsets[i]);

		free(file->sample_offsets);
		free(file->total_sample_offsets);
	}

	free(file);
	return 0;
}

int mpeg3_check_sig(const char *path)
{
	mpeg3_fs_t *fs;
	uint32_t bits;
	char *ext;
	int result = 0;

	fs = mpeg3_new_fs(path);
	if(mpeg3io_open_file(fs))
	{
/* File not found */
		return 0;
	}

	bits = mpeg3io_read_int32(fs);
/* Test header */
	if(bits == MPEG3_TOC_PREFIX)
	{
		result = 1;
	}
	else
	if((((bits >> 24) & 0xff) == MPEG3_SYNC_BYTE) ||
		(bits == MPEG3_PACK_START_CODE) ||
		((bits & 0xfff00000) == 0xfff00000) ||
		((bits & 0xffff0000) == 0xffe30000) ||
		(bits == MPEG3_SEQUENCE_START_CODE) ||
		(bits == MPEG3_PICTURE_START_CODE) ||
		(((bits & 0xffff0000) >> 16) == MPEG3_AC3_START_CODE) ||
		((bits >> 8) == MPEG3_ID3_PREFIX) ||
		(bits == MPEG3_RIFF_CODE) ||
        (bits == MPEG3_IFO_PREFIX))
	{
		result = 1;

		ext = strrchr(path, '.');
		if(ext)
		{
/* Test file extension. */
			if(strncasecmp(ext, ".ifo", 4) && 
            	strncasecmp(ext, ".mp2", 4) && 
				strncasecmp(ext, ".mp3", 4) &&
				strncasecmp(ext, ".m1v", 4) &&
				strncasecmp(ext, ".m2v", 4) &&
				strncasecmp(ext, ".m2s", 4) &&
				strncasecmp(ext, ".mpg", 4) &&
				strncasecmp(ext, ".vob", 4) &&
				strncasecmp(ext, ".mpeg", 4) &&
				strncasecmp(ext, ".ac3", 4))
				result = 0;
		}
	}

	mpeg3io_close_file(fs);
	mpeg3_delete_fs(fs);
	return result;
}


static uint32_t read_int32(unsigned char *buffer, int *position)
{
	uint32_t temp;

	if(MPEG3_LITTLE_ENDIAN)
	{
		((unsigned char*)&temp)[3] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[0] = buffer[(*position)++];
	}
	else
	{
		((unsigned char*)&temp)[0] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[3] = buffer[(*position)++];
	}
	
	return temp;
}

static uint64_t read_int64(unsigned char *buffer, int *position)
{
	uint64_t temp;

	if(MPEG3_LITTLE_ENDIAN)
	{
		((unsigned char*)&temp)[7] = buffer[(*position)++];
		((unsigned char*)&temp)[6] = buffer[(*position)++];
		((unsigned char*)&temp)[5] = buffer[(*position)++];
		((unsigned char*)&temp)[4] = buffer[(*position)++];
		((unsigned char*)&temp)[3] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[0] = buffer[(*position)++];
	}
	else
	{
		((unsigned char*)&temp)[0] = buffer[(*position)++];
		((unsigned char*)&temp)[1] = buffer[(*position)++];
		((unsigned char*)&temp)[2] = buffer[(*position)++];
		((unsigned char*)&temp)[3] = buffer[(*position)++];
		((unsigned char*)&temp)[4] = buffer[(*position)++];
		((unsigned char*)&temp)[5] = buffer[(*position)++];
		((unsigned char*)&temp)[6] = buffer[(*position)++];
		((unsigned char*)&temp)[7] = buffer[(*position)++];
	}

	return temp;
}






static int read_toc(mpeg3_t *file)
{
	unsigned char *buffer;
	int file_type;
	int position = 4;
	int stream_type;
	int atracks;
	int vtracks;
	int i, j;

	buffer = malloc(mpeg3io_total_bytes(file->fs));
	mpeg3io_seek(file->fs, 0);
	mpeg3io_read_data(buffer, mpeg3io_total_bytes(file->fs), file->fs);


//printf("read_toc "D64"\n", mpeg3io_total_bytes(file->fs));

// File type
	file_type = buffer[position++];
	switch(file_type)
	{
		case FILE_TYPE_PROGRAM:
			file->is_program_stream = 1;
			break;
		case FILE_TYPE_TRANSPORT:
			file->is_transport_stream = 1;
			break;
		case FILE_TYPE_AUDIO:
			file->is_audio_stream = 1;
			break;
		case FILE_TYPE_VIDEO:
			file->is_video_stream = 1;
			break;
	}


// Stream ID's
	while((stream_type = buffer[position]) != TITLE_PATH)
	{
		int offset;
		int stream_id;

//printf("read_toc %d %x\n", position, buffer[position]);
		position++;
		offset = read_int32(buffer, &position);
		stream_id = read_int32(buffer, &position);

		if(stream_type == STREAM_AUDIO)
		{
			file->demuxer->astream_table[offset] = stream_id;
		}

		if(stream_type == STREAM_VIDEO)
		{
			file->demuxer->vstream_table[offset] = stream_id;
		}
	}





// Titles
	while(buffer[position] == TITLE_PATH)
	{
		char string[MPEG3_STRLEN];
		int string_len = 0;
		mpeg3_title_t *title;

		position++;
		while(buffer[position] != 0) string[string_len++] = buffer[position++];
		string[string_len++] = 0;
		position++;

		title = 
			file->demuxer->titles[file->demuxer->total_titles++] = 
			mpeg3_new_title(file, string);

		title->total_bytes = read_int64(buffer, &position);

// Cells
		title->timecode_table_size = 
			title->timecode_table_allocation = 
			read_int32(buffer, &position);
		title->timecode_table = calloc(title->timecode_table_size, sizeof(mpeg3demux_timecode_t));
		for(i = 0; i < title->timecode_table_size; i++)
		{
			title->timecode_table[i].start_byte = read_int64(buffer, &position);
			title->timecode_table[i].end_byte = read_int64(buffer, &position);
			title->timecode_table[i].program = read_int32(buffer, &position);
		}
	}



// Audio streams
// Skip ATRACK_COUNT
	position++;
	atracks = read_int32(buffer, &position);

// Skip VTRACK_COUNT
	position++;
	vtracks = read_int32(buffer, &position);


	if(atracks)
	{
		file->sample_offsets = malloc(sizeof(int64_t*) * atracks);
		file->total_sample_offsets = malloc(sizeof(int*) * atracks);

		for(i = 0; i < atracks; i++)
		{
			file->total_sample_offsets[i] = read_int32(buffer, &position);
			file->sample_offsets[i] = malloc(file->total_sample_offsets[i] * sizeof(int64_t));
			for(j = 0; j < file->total_sample_offsets[i]; j++)
			{
				file->sample_offsets[i][j] = read_int64(buffer, &position);
//printf("samples "X64"\n", file->sample_offsets[i][j]);
			}
		}
	}

	if(vtracks)
	{
		file->frame_offsets = malloc(sizeof(int64_t*) * vtracks);
		file->total_frame_offsets = malloc(sizeof(int*) * vtracks);
		file->keyframe_numbers = malloc(sizeof(int64_t*) * vtracks);
		file->total_keyframe_numbers = malloc(sizeof(int*) * vtracks);

		for(i = 0; i < vtracks; i++)
		{
			file->total_frame_offsets[i] = read_int32(buffer, &position);
			file->frame_offsets[i] = malloc(file->total_frame_offsets[i] * sizeof(int64_t));
			for(j = 0; j < file->total_frame_offsets[i]; j++)
			{
				file->frame_offsets[i][j] = read_int64(buffer, &position);
//printf("frame "X64"\n", file->frame_offsets[i][j]);
			}


			file->total_keyframe_numbers[i] = read_int32(buffer, &position);
			file->keyframe_numbers[i] = malloc(file->total_keyframe_numbers[i] * sizeof(int64_t));
			for(j = 0; j < file->total_keyframe_numbers[i]; j++)
			{
				file->keyframe_numbers[i][j] = read_int64(buffer, &position);
			}
		}
	}

	free(buffer);



//printf("read_toc 1\n");
	mpeg3demux_open_title(file->demuxer, 0);

//printf("read_toc 2 "X64"\n", mpeg3demux_tell(file->demuxer));
	return 0;
}




mpeg3_t* mpeg3_open_copy(const char *path, mpeg3_t *old_file)
{
	mpeg3_t *file = 0;
	unsigned int bits;
	int i, done;

/* Initialize the file structure */
	file = mpeg3_new(path);







//printf("mpeg3_open_copy %s\n", path);

/* Need to perform authentication before reading a single byte. */
	if(mpeg3io_open_file(file->fs))
	{
		mpeg3_delete(file);
		return 0;
	}













/* =============================== Create the title objects ========================= */
	bits = mpeg3io_read_int32(file->fs);
//printf("mpeg3_open 1 %p %d %d %d %d\n", old_file, file->is_transport_stream, file->is_program_stream, file->is_video_stream, file->is_audio_stream);

	if(bits == MPEG3_TOC_PREFIX)   /* TOC  */
	{
/* Table of contents for another title set */
		if(!old_file)
		{
//printf("libmpeg3 1\n");
			if(read_toc(file))
			{
//printf("libmpeg3 1\n");
				mpeg3io_close_file(file->fs);
				mpeg3_delete(file);
				return 0;
			}
//printf("libmpeg3 1\n");
		}
		mpeg3io_close_file(file->fs);
	}
	else
// IFO file
#ifndef _WIN32
    if(bits == MPEG3_IFO_PREFIX)
    {
//printf("libmpeg3 1\n");
    	if(!old_file)
		{
//printf("libmpeg3 2\n");
			if(mpeg3_read_ifo(file, 0))
        	{
				mpeg3_delete(file);
				mpeg3io_close_file(file->fs);
				return 0;
        	}
//printf("libmpeg3 3\n");
		}
		file->is_ifo_file = 1;
		mpeg3io_close_file(file->fs);
    }
    else
#endif
	if(((bits >> 24) & 0xff) == MPEG3_SYNC_BYTE)
	{
/* Transport stream */
//printf("libmpeg3 2\n");
		file->is_transport_stream = 1;
	}
	else
	if(bits == MPEG3_PACK_START_CODE)
	{
/* Program stream */
/* Determine packet size empirically */
//printf("libmpeg3 3\n");
		file->is_program_stream = 1;
	}
	else
	if((bits & 0xfff00000) == 0xfff00000 ||
		(bits & 0xffff0000) == 0xffe30000 ||
		((bits >> 8) == MPEG3_ID3_PREFIX) ||
		(bits == MPEG3_RIFF_CODE))
	{
/* MPEG Audio only */
//printf("libmpeg3 4\n");
		file->is_audio_stream = 1;
	}
	else
	if(bits == MPEG3_SEQUENCE_START_CODE ||
		bits == MPEG3_PICTURE_START_CODE)
	{
/* Video only */
//printf("libmpeg3 5\n");
		file->is_video_stream = 1;
	}
	else
	if(((bits & 0xffff0000) >> 16) == MPEG3_AC3_START_CODE)
	{
/* AC3 Audio only */
//printf("libmpeg3 6\n");
		file->is_audio_stream = 1;
	}
	else
	{
//printf("libmpeg3 7\n");
		mpeg3_delete(file);
		fprintf(stderr, "mpeg3_open: not an MPEG 2 stream\n");
		return 0;
	}

//printf("mpeg3_open 2 %p %d %d %d %d\n", 
//	old_file, file->is_transport_stream, file->is_program_stream, file->is_video_stream, file->is_audio_stream);













// Configure packet size
	if(file->is_transport_stream)
		file->packet_size = MPEG3_TS_PACKET_SIZE;
	else
	if(file->is_program_stream)
		file->packet_size = 0;
	else
	if(file->is_audio_stream)
		file->packet_size = MPEG3_DVD_PACKET_SIZE;
	else
	if(file->is_video_stream)
		file->packet_size = MPEG3_DVD_PACKET_SIZE;













//printf("mpeg3_open 1\n");

/* Create titles */
/* Copy timecodes from an old demuxer */
	if(old_file && mpeg3_get_demuxer(old_file))
	{
		mpeg3demux_copy_titles(file->demuxer, mpeg3_get_demuxer(old_file));
		file->is_transport_stream = old_file->is_transport_stream;
		file->is_program_stream = old_file->is_program_stream;
	}
	else
/* Start from scratch */
	if(!file->demuxer->total_titles)
	{
		mpeg3demux_create_title(file->demuxer, 0, 0);
	}



//printf("mpeg3_open 2\n");











/* Generate tracks */
	if(file->is_transport_stream || file->is_program_stream)
	{
/* Create video tracks */
/* Video must be created before audio because audio uses the video timecode */
/* to get its length. */
		for(i = 0; i < MPEG3_MAX_STREAMS; i++)
		{
			if(file->demuxer->vstream_table[i])
			{
				file->vtrack[file->total_vstreams] = 
					mpeg3_new_vtrack(file, 
						i, 
						file->demuxer, 
						file->total_vstreams);
				if(file->vtrack[file->total_vstreams]) 
					file->total_vstreams++;

//printf("libmpeg3 %d %d %p %d\n", i, file->demuxer->vstream_table[i], file->vtrack[file->total_vstreams], file->total_vstreams);
			}
		}

/* Create audio tracks */
		for(i = 0; i < MPEG3_MAX_STREAMS; i++)
		{
			if(file->demuxer->astream_table[i])
			{
				file->atrack[file->total_astreams] = mpeg3_new_atrack(file, 
					i, 
					file->demuxer->astream_table[i], 
					file->demuxer,
					file->total_astreams);
				if(file->atrack[file->total_astreams]) file->total_astreams++;
			}
		}
	}
	else
	if(file->is_video_stream)
	{
/* Create video tracks */
//printf("mpeg3_open 3\n");
		file->vtrack[0] = mpeg3_new_vtrack(file, 
			-1, 
			file->demuxer, 
			0);
//printf("mpeg3_open 4\n");
		if(file->vtrack[0]) file->total_vstreams++;
	}
	else
	if(file->is_audio_stream)
	{
/* Create audio tracks */

//printf("mpeg3_open 3\n");
		file->atrack[0] = mpeg3_new_atrack(file, 
			-1, 
			AUDIO_UNKNOWN, 
			file->demuxer,
			0);
//printf("mpeg3_open 3\n");
		if(file->atrack[0]) file->total_astreams++;
	}

//printf("mpeg3_open 5\n");



	mpeg3io_close_file(file->fs);
	return file;
}

mpeg3_t* mpeg3_open(const char *path)
{
	return mpeg3_open_copy(path, 0);
}

int mpeg3_close(mpeg3_t *file)
{
/* File is closed in the same procedure it is opened in. */
	mpeg3_delete(file);
	return 0;
}


int mpeg3_has_audio(mpeg3_t *file)
{
	return file->total_astreams > 0;
}

int mpeg3_total_astreams(mpeg3_t *file)
{
	return file->total_astreams;
}

int mpeg3_audio_channels(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->channels;
	return -1;
}

int mpeg3_sample_rate(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->sample_rate;
	return -1;
}

int mpeg3_audio_samples_per_frame(mpeg3_t *file,
		int stream)
{
	if(file->total_astreams)
		return file->atrack[stream]->samples_per_frame;
	return -1;
}

uint32_t mpeg3_audio_get_number_of_frames (mpeg3_t *file, 
					   int stream)
{
  if (file->total_astreams) 
    return file->atrack[stream]->total_frames;
  return -1;
}

char* mpeg3_audio_format(mpeg3_t *file, int stream)
{
	if(stream < file->total_astreams)
	{
		switch(file->atrack[stream]->format)
		{
			case AUDIO_UNKNOWN: return "Unknown"; break;
			case AUDIO_MPEG:    return "MPEG"; break;
			case AUDIO_AC3:     return "AC3"; break;
			case AUDIO_PCM:     return "PCM"; break;
			case AUDIO_AAC:     return "AAC"; break;
			case AUDIO_JESUS:   return "Vorbis"; break;
		}
	}
	return "";
}

int mpeg3_has_video(mpeg3_t *file)
{
	return file->total_vstreams > 0;
}

int mpeg3_total_vstreams(mpeg3_t *file)
{
	return file->total_vstreams;
}

int mpeg3_video_width(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->width;
	return -1;
}

int mpeg3_video_height(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->height;
	return -1;
}

float mpeg3_aspect_ratio(mpeg3_t *file, int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->aspect_ratio;
	return 0;
}

double mpeg3_frame_rate(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->frame_rate;
	return -1;
}

double mpeg3_video_bitrate(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->bitrate;
	return -1;
}

int mpeg3_video_layer (mpeg3_t *file, int stream)
{
  if (file->total_vstreams) 
    return file->vtrack[stream]->mpeg_layer;
  return -1;
}

long mpeg3_video_frames(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->total_frames;
	return -1;
}

long mpeg3_get_frame(mpeg3_t *file,
		int stream)
{
	if(file->total_vstreams)
		return file->vtrack[stream]->current_position;
	return -1;
}

int mpeg3_set_frame(mpeg3_t *file, 
		long frame,
		int stream)
{
	if(file->total_vstreams)
	{
	  file->vtrack[stream]->current_position = frame;
		mpeg3vtrack_seek_frame(file->vtrack[stream], frame);
		return 0;
	}
	return -1;
}

int mpeg3_seek_percentage(mpeg3_t *file, double percentage)
{
	int i;
	for(i = 0; i < file->total_vstreams; i++)
	{
		mpeg3vtrack_seek_percentage(file->vtrack[i], percentage);
	}

	for(i = 0; i < file->total_astreams; i++)
	{
	  mpeg3atrack_seek_percentage(file->atrack[i], percentage);
	}

	return 0;
}

int mpeg3_seek_audio_percentage(mpeg3_t *file, int stream, double percentage)
{
  mpeg3atrack_seek_percentage(file->atrack[stream], percentage);
  return 0;
}

int mpeg3_seek_video_percentage(mpeg3_t *file, int stream, double percentage)
{
  mpeg3vtrack_seek_percentage(file->vtrack[stream], percentage);
  return 0;
}

#if 0
int mpeg3_previous_frame(mpeg3_t *file, int stream)
{
	file->last_type_read = 2;
	file->last_stream_read = stream;

	if(file->total_vstreams)
		return mpeg3video_previous_frame(file->vtrack[stream]->video);

	return 0;
}
#endif

double mpeg3_tell_percentage(mpeg3_t *file)
{
	double percent = 0;
	if(file->last_type_read == 1)
	{
		percent = mpeg3demux_tell_percentage(file->atrack[file->last_stream_read]->demuxer);
	}

	if(file->last_type_read == 2)
	{
		percent = mpeg3demux_tell_percentage(file->vtrack[file->last_stream_read]->demuxer);
	}
	return percent;
}


int mpeg3_end_of_audio(mpeg3_t *file, int stream)
{
	int result = 0;
	result = mpeg3demux_eof(file->atrack[stream]->demuxer);
	return result;
}

int mpeg3_end_of_video(mpeg3_t *file, int stream)
{
	int result = 0;
	result = mpeg3demux_eof(file->vtrack[stream]->demuxer);
	return result;
}



int mpeg3_read_audio_frame (mpeg3_t *file,
			    unsigned char **output,
			    uint32_t *size,
			    uint32_t *max_size,
			    int stream)
{
  int result = -1;
  if (file->total_astreams > stream) {
    result = mpeg3_atrack_read_frame(file->atrack[stream],
				     output, 
				     size,
				     max_size);
    file->last_type_read = 1;
    file->last_stream_read = stream;

  }
  return result;
}
				 
int mpeg3_get_audio_format (mpeg3_t *file, int stream)
{
  if (file->total_astreams > stream) {
    return (file->atrack[stream]->format);
  }
  return AUDIO_UNKNOWN;
}
int mpeg3_read_video_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream)
{
	int result = 0;
	if(file->total_vstreams)
	{
		result = mpeg3vtrack_read_raw(file->vtrack[stream], output, size, max_size);
		file->last_type_read = 2;
		file->last_stream_read = stream;
	}
	return result;
}

int mpeg3_read_video_chunk_resize (mpeg3_t *file,
				   unsigned char **output,
				   long *size,
				   int stream)
{
	int result = 0;
	if(file->total_vstreams)
	{
		result = mpeg3vtrack_read_raw_resize(file->vtrack[stream], 
						     output, 
						     size);
		file->last_type_read = 2;
		file->last_stream_read = stream;
	}
	return result;
}
void mpeg3_read_video_chunk_cleanup (mpeg3_t *file,
				     int stream)
{
	int result = 0;
	if(file->total_vstreams)
	{
	  mpeg3vtrack_cleanup_frame(file->vtrack[stream]);
	}
}
