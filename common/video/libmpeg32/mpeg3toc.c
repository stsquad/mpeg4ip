#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>




#define INIT_VECTORS(data, size, allocation, tracks) \
{ \
	int k; \
	data = calloc(1, sizeof(int64_t*) * tracks); \
	size = calloc(1, sizeof(int*) * tracks); \
	allocation = calloc(1, sizeof(int*) * tracks); \
 \
	for(k = 0; k < tracks; k++) \
	{ \
		allocation[k] = 0x100; \
		data[k] = calloc(1, sizeof(int64_t) * allocation[k]); \
	} \
}

#define APPEND_VECTOR(data, size, allocation, track, value) \
{ \
	if(!data[track] || allocation[track] <= size[track]) \
	{ \
		int64_t *new_data = calloc(1, sizeof(int64_t) * allocation[track] * 2); \
 \
		if(data[track]) \
		{ \
			memcpy(new_data, data[track], sizeof(int64_t) * allocation[track]); \
			free(data[track]); \
		} \
		allocation[track] *= 2; \
		data[track] = new_data; \
	} \
 \
	data[track][size[track]++] = value; \
}

#define DELETE_VECTORS(data, size, allocation, tracks) \
{ \
	int k; \
	for(k = 0; k < tracks; k++) if(data[k]) free(data[k]); \
	free(data); \
}






#define PUT_INT32(x) \
{ \
	if(MPEG3_LITTLE_ENDIAN) \
	{ \
		fputc(((unsigned char*)&x)[3], output); \
		fputc(((unsigned char*)&x)[2], output); \
		fputc(((unsigned char*)&x)[1], output); \
		fputc(((unsigned char*)&x)[0], output); \
	} \
	else \
	{ \
		fputc(((unsigned char*)&x)[0], output); \
		fputc(((unsigned char*)&x)[1], output); \
		fputc(((unsigned char*)&x)[2], output); \
		fputc(((unsigned char*)&x)[3], output); \
	} \
}




#define PUT_INT64(x) \
{ \
	if(MPEG3_LITTLE_ENDIAN) \
	{ \
		fputc(((unsigned char*)&x)[7], output); \
		fputc(((unsigned char*)&x)[6], output); \
		fputc(((unsigned char*)&x)[5], output); \
		fputc(((unsigned char*)&x)[4], output); \
		fputc(((unsigned char*)&x)[3], output); \
		fputc(((unsigned char*)&x)[2], output); \
		fputc(((unsigned char*)&x)[1], output); \
		fputc(((unsigned char*)&x)[0], output); \
	} \
	else \
	{ \
		fwrite(&x, 1, 8, output); \
	} \
}





int main(int argc, char *argv[])
{
	struct stat st;

	if(argc < 3)
	{
		fprintf(stderr, "Create a table of contents for a DVD or mpeg stream.\n"
			"	Usage: mpeg3toc <path> <output>\n"
			"\n"
			"	The path should be absolute unless you plan\n"
			"	to always run your movie editor from the same directory\n"
			"	as the filename.  For renderfarms the filesystem prefix\n"
			"	should be / and the movie directory mounted under the same\n"
			"	directory on each node.\n"
			"Example: mpeg3toc /cd2/video_ts/vts_01_0.ifo titanic.toc\n");
		exit(1);
	}



	stat(argv[1], &st);

	if(!st.st_size)
	{
		fprintf(stderr, "%s is 0 length.  Skipping\n", argv[1]);
	}
	else
	{
		int i, j, l;
		int64_t size;
		int vtracks;
		int atracks;
		mpeg3_t *input;
		uint64_t **frame_offsets;
		uint64_t **keyframe_numbers;
		uint64_t **sample_offsets;
		int *total_frame_offsets;
		int *total_sample_offsets;
		int *total_keyframe_numbers;
		int *frame_offset_allocation;
		int *sample_offset_allocation;
		int *keyframe_numbers_allocation;
		int done = 0;
		double sample_rate;
		double frame_rate;
		FILE *output;
		int total_samples = 0;
		int total_frames = 0;
		int rewind = 1;

		input = mpeg3_open(argv[1]);
		output = fopen(argv[2], "w");

		vtracks = mpeg3_total_vstreams(input);
		atracks = mpeg3_total_astreams(input);
		if(atracks) sample_rate = mpeg3_sample_rate(input, 0);
		if(vtracks) frame_rate = mpeg3_frame_rate(input, 0);

// Handle titles
		INIT_VECTORS(frame_offsets, total_frame_offsets, frame_offset_allocation, vtracks);
		INIT_VECTORS(keyframe_numbers, total_keyframe_numbers, keyframe_numbers_allocation, vtracks);
		INIT_VECTORS(sample_offsets, total_sample_offsets, sample_offset_allocation, atracks);

		while(!done)
		{
			int sample_count = MPEG3_AUDIO_CHUNKSIZE;
			int frame_count;
			int have_audio = 0;
			int have_video = 0;

// Store current position and read sample_count from each atrack
			for(j = 0; j < atracks; j++)
			{
				if(rewind)
				{
					mpeg3_demuxer_t *demuxer = input->atrack[j]->demuxer;
					mpeg3demux_seek_byte(demuxer, 0);
				}

				if(!mpeg3_end_of_audio(input, j))
				{
// Don't want to maintain separate vectors for offset and title.
					int64_t title_number = mpeg3demux_tell_title(input->atrack[j]->demuxer);
					int64_t position = mpeg3demux_tell(input->atrack[j]->demuxer);
					int64_t result;
					if(position < MPEG3_IO_SIZE) position = MPEG3_IO_SIZE;
					result = (title_number << 56) | (position - MPEG3_IO_SIZE);

					have_audio = 1;
					APPEND_VECTOR(sample_offsets, 
						total_sample_offsets,
						sample_offset_allocation,
						j,
						result);


// Throw away samples
					mpeg3_read_audio(input, 
						0,
						0,
						0,
						sample_count,         /* Number of samples to decode */
						j);

					if(j == atracks - 1) 
					{
						total_samples += sample_count;
//						printf("Audio: title=%lld offset=%016llx ", title_number, position);
						printf("Audio: title=%lld total_samples=%d ", title_number, total_samples);
					}
				}
			}

			if(have_audio)
			{
				frame_count = 
					(int)((double)total_samples / sample_rate * frame_rate + 0.5) - 
					total_frames;
			}
			else
			{
				frame_count = 1;
			}

			for(j = 0; j < vtracks; j++)
			{
				if(rewind)
				{
					mpeg3_demuxer_t *demuxer = input->vtrack[j]->demuxer;
					mpeg3demux_seek_byte(demuxer, 0);
				}


				for(l = 0; l < frame_count; l++)
				{
					if(!mpeg3_end_of_video(input, j))
					{
						int64_t title_number = mpeg3demux_tell_title(input->vtrack[j]->demuxer);
						int64_t position = mpeg3demux_tell(input->vtrack[j]->demuxer);
						int64_t result;
						uint32_t code = 0;

						if(position < MPEG3_IO_SIZE) position = MPEG3_IO_SIZE;
						result = (title_number << 56) | (position - MPEG3_IO_SIZE);
						have_video = 1;

						APPEND_VECTOR(frame_offsets, 
							total_frame_offsets,
							frame_offset_allocation,
							j,
							result);



// Search for next frame start.
						if(total_frame_offsets[j] == 1)
						{
// Assume first frame is an I-frame
							APPEND_VECTOR(keyframe_numbers,
								total_keyframe_numbers,
								keyframe_numbers_allocation,
								j,
								0);



// Skip the first frame.
							mpeg3video_get_header(input->vtrack[j]->video, 0);
							input->vtrack[j]->video->current_repeat += 100;
						}





// Get next frame
						mpeg3video_get_header(input->vtrack[j]->video, 0);
						input->vtrack[j]->video->current_repeat += 100;

						if(input->vtrack[j]->video->pict_type == I_TYPE)
							APPEND_VECTOR(keyframe_numbers,
								total_keyframe_numbers,
								keyframe_numbers_allocation,
								j,
								total_frame_offsets[j] - 1);

						if(j == vtracks - 1 && l == frame_count - 1)
						{
							total_frames += frame_count;
							printf("Video: title=%lld total_frames=%d ", title_number, total_frames);
						}
					}
				}
			}

			if(!have_audio && !have_video) done = 1;

			printf("\r");
			fflush(stdout);
/*
 * if(total_frames > 10000) 
 * {
 * printf("\n");
 * return 0;
 * }
 */



			rewind = 0;
		}



// Write file type
		fputc('T', output);
		fputc('O', output);
		fputc('C', output);
		fputc(' ', output);


		if(input->is_program_stream)
		{
			fputc(FILE_TYPE_PROGRAM, output);
		}
		else
		if(input->is_transport_stream)
		{
			fputc(FILE_TYPE_TRANSPORT, output);
		}
		else
		if(input->is_audio_stream)
		{
			fputc(FILE_TYPE_AUDIO, output);
		}
		else
		if(input->is_video_stream)
		{
			fputc(FILE_TYPE_VIDEO, output);
		}

// Write stream ID's
// Only program and transport streams have these
		for(i = 0; i < MPEG3_MAX_STREAMS; i++)
		{
			if(input->demuxer->astream_table[i])
			{
				fputc(STREAM_AUDIO, output);
				PUT_INT32(i);
				PUT_INT32(input->demuxer->astream_table[i]);
			}

			if(input->demuxer->vstream_table[i])
			{
				fputc(STREAM_VIDEO, output);
				PUT_INT32(i);
				PUT_INT32(input->demuxer->vstream_table[i]);
			}
		}


// Write titles
		for(i = 0; i < input->demuxer->total_titles; i++)
		{
// Path
			fputc(TITLE_PATH, output);
			fprintf(output, input->demuxer->titles[i]->fs->path);
			fputc(0, output);
// Total bytes
			PUT_INT64(input->demuxer->titles[i]->total_bytes);
// Byte offsets of cells
			PUT_INT32(input->demuxer->titles[i]->timecode_table_size);
			for(j = 0; j < input->demuxer->titles[i]->timecode_table_size; j++)
			{
				PUT_INT64(input->demuxer->titles[i]->timecode_table[j].start_byte);
				PUT_INT64(input->demuxer->titles[i]->timecode_table[j].end_byte);
				PUT_INT32(input->demuxer->titles[i]->timecode_table[j].program);
			}
		}








		fputc(ATRACK_COUNT, output);
		PUT_INT32(atracks);

		fputc(VTRACK_COUNT, output);
		PUT_INT32(vtracks);

// Audio streams
		for(j = 0; j < atracks; j++)
		{
			PUT_INT32(total_sample_offsets[j]);
			for(i = 0; i < total_sample_offsets[j]; i++)
			{
				PUT_INT64(sample_offsets[j][i]);
//printf("Audio: offset=%016llx\n", sample_offsets[j][i]);
			}
		}

// Video streams
		for(j = 0; j < vtracks; j++)
		{
			PUT_INT32(total_frame_offsets[j]);
			for(i = 0; i < total_frame_offsets[j]; i++)
			{
				PUT_INT64(frame_offsets[j][i]);
//printf("Video: offset=%016llx\n", frame_offsets[j][i]);
			}

			PUT_INT32(total_keyframe_numbers[j]);
			for(i = 0; i < total_keyframe_numbers[j]; i++)
			{
				PUT_INT64(keyframe_numbers[j][i]);
//printf("Video: keyframe=%lld\n", keyframe_numbers[j][i]);
			}
		}




		DELETE_VECTORS(frame_offsets, total_frame_offsets, frame_offset_allocation, vtracks);
		DELETE_VECTORS(keyframe_numbers, total_keyframe_numbers, keyframe_numbers_allocation, vtracks);
		DELETE_VECTORS(sample_offsets, total_sample_offsets, sample_offset_allocation, atracks);


		mpeg3_close(input);
		fclose(output);
	}




	return 0;
}
