/* Concatenate elementary streams */

#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define BUFFER_SIZE            1000000

int main(int argc, char *argv[])
{
	char inpath[1024], outpath[1024], newpath[1024];
	mpeg3_t *in;
	FILE *out;
	int out_counter = 0;
	int current_file, current_output_file = 0, i;
	unsigned int bits;
	unsigned char *buffer;
	long output_size;
	int result = 0;
	long total_frames = 0;
	int do_audio = 0, do_video = 0;
	int stream = 0;
	int64_t total_written = 0;

	if(argc < 2)
	{
		fprintf(stderr, "Concatenate elementary streams or demultiplex a program stream.\n"
			"Usage: mpeg3cat -[av0123456789] <infile> [infile...] > <outfile>\n\n"
			"Example: Concatenate 2 video files: mpeg3cat xena1.m2v xena2.m2v > xena.m2v\n"
			"         Extract audio stream 0: mpeg3cat -a0 xena.vob > war_cry.ac3\n");
		exit(1);
	}

	outpath[0] = 0;
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] != 'a' && argv[i][1] != 'v' && argv[i][1] != 'o')
			{
				fprintf(stderr, "invalid option %s\n", argv[i]);
				exit(1);
			}
			else
			if(argv[i][1] == 'o')
			{
// Check for filename
				if(i < argc - 1)
				{
					strcpy(outpath, argv[++i]);
				}
				else
				{
					fprintf(stderr, "-o requires an output file\n");
					exit(1);
				}

// Check if file exists
				if(out = fopen(outpath, "r"))
				{
					fprintf(stderr, "%s exists.\n", outpath);
					exit(1);
				}

			}
			else
			{
				if(argv[i][1] == 'a') do_audio = 1;
				else
				if(argv[i][1] == 'v') do_video = 1;
				
				if(argv[i][2] != 0)
				{
					stream = argv[i][2] - 48;
				}
			}
		}
	}

	buffer = malloc(BUFFER_SIZE);
	if(outpath[0])
	{
		if(!(out = fopen(outpath, "wb")))
		{
			fprintf(stderr, "Failed to open %s for writing\n", outpath);
			exit(1);
		}
	}
	else
		out = stdout;

	for(current_file = 1; current_file < argc; current_file++)
	{
		if(argv[current_file][0] == '-') continue;

		strcpy(inpath, argv[current_file]);
		if(!(in = mpeg3_open(inpath)))
		{
			fprintf(stderr, "Skipping %s\n", inpath);
			continue;
		}





//fprintf(stderr, "%d %d %d %d\n", in->is_transport_stream, in->is_program_stream, in->is_audio_stream, in->is_video_stream);
		if((mpeg3_has_audio(in) && in->is_audio_stream) || 
			(do_audio && !in->is_audio_stream && !in->is_video_stream))
		{
			do_audio = 1;
/* Add audio stream to end */
			mpeg3demux_seek_byte(in->atrack[stream]->demuxer, 0);
			while(!mpeg3_read_audio_chunk(in, buffer, 
				&output_size, 
				BUFFER_SIZE,
				stream))
			{
				result = !fwrite(buffer, output_size, 1, out);
				if(result)
				{
					perror("fwrite audio chunk");
					break;
				}
			}
		}
		else
		if((mpeg3_has_video(in) && in->is_video_stream) ||
			(do_video && !in->is_video_stream && !in->is_audio_stream))
		{
/* Add video stream to end */
			int hour, minute, second, frame;
			long gop_frame;
			unsigned long code;
			float carry;
			int i, offset;
			
			mpeg3demux_seek_byte(in->vtrack[stream]->demuxer, 0);
			do_video = 1;
			while(!mpeg3_read_video_chunk(in, 
				buffer, 
				&output_size, 
				BUFFER_SIZE,
				stream) &&
				output_size >= 4)
			{
				code = (unsigned long)buffer[output_size - 4] << 24; 
				code |= (unsigned long)buffer[output_size - 3] << 16; 
				code |= (unsigned long)buffer[output_size - 2] << 8; 
				code |= (unsigned long)buffer[output_size - 1]; 

/* Got a frame at the end of this buffer. */
				if(code == MPEG3_PICTURE_START_CODE)
				{
					total_frames++;
				}
				else
				if(code == MPEG3_SEQUENCE_END_CODE)
				{
/* Got a sequence end code at the end of this buffer. */
					output_size -= 4;
				}

				code = (unsigned long)buffer[0] << 24;
				code |= (unsigned long)buffer[1] << 16;
				code |= (unsigned long)buffer[2] << 8;
				code |= buffer[3];

				i = 0;
				offset = 0;
				if(code == MPEG3_SEQUENCE_START_CODE && current_output_file > 0)
				{
/* Skip the sequence start code */
					i += 4;
					while(i < output_size && 
						code != MPEG3_GOP_START_CODE)
					{
						code <<= 8;
						code |= buffer[i++];
					}
					i -= 4;
					offset = i;
				}

/* Search for GOP header to fix */
				code = (unsigned long)buffer[i++] << 24;
				code |= (unsigned long)buffer[i++] << 16;
				code |= (unsigned long)buffer[i++] << 8;
				code |= buffer[i++];
				while(i < output_size &&
					code != MPEG3_GOP_START_CODE)
				{
					code <<= 8;
					code |= buffer[i++];
				}

				if(code == MPEG3_GOP_START_CODE)
				{
/* Get the time code */
					code = (unsigned long)buffer[i] << 24;
					code |= (unsigned long)buffer[i + 1] << 16;
					code |= (unsigned long)buffer[i + 2] << 8;
					code |= (unsigned long)buffer[i + 3];

					hour = code >> 26 & 0x1f;
					minute = code >> 20 & 0x3f;
					second = code >> 13 & 0x3f;
					frame = code >> 7 & 0x3f;

					gop_frame = (long)(hour * 3600 * mpeg3_frame_rate(in, stream) +
							minute * 60 * mpeg3_frame_rate(in, stream) +
							second * mpeg3_frame_rate(in, stream) + 
							frame);
/* fprintf(stderr, "old: %02d:%02d:%02d:%02d ", hour, minute, second, frame); */
/* Write a new time code */
					hour = (long)((float)(total_frames - 1) / mpeg3_frame_rate(in, stream) / 3600);
					carry = hour * 3600 * mpeg3_frame_rate(in, stream);
					minute = (long)((float)(total_frames - 1 - carry) / mpeg3_frame_rate(in, stream) / 60);
					carry += minute * 60 * mpeg3_frame_rate(in, stream);
					second = (long)((float)(total_frames - 1 - carry) / mpeg3_frame_rate(in, stream));
					carry += second * mpeg3_frame_rate(in, stream);
					frame = (total_frames - 1 - carry);

					buffer[i] = ((code >> 24) & 0x80) | (hour << 2) | (minute >> 4);
					buffer[i + 1] = ((code >> 16) & 0x08) | ((minute & 0xf) << 4) | (second >> 3);
					buffer[i + 2] = ((second & 0x7) << 5) | (frame >> 1);
					buffer[i + 3] = (code & 0x7f) | ((frame & 0x1) << 7);
/* fprintf(stderr, "new: %02d:%02d:%02d:%02d\n", hour, minute, second, frame); */
				}


/* Test 32 bit overflow */
				if(outpath[0])
				{
					if(ftell(out) > 0x7f000000)
					{
						fclose(out);
						out_counter++;
						sprintf(newpath, "%s%03d", outpath, out_counter);
						if(!(out = fopen(newpath, "wb")))
						{
							fprintf(stderr, "Couldn't open %s for writing.\n", newpath);
							exit(1);
						}
					}
				}

/* Write the frame */
				result = !fwrite(buffer + offset, output_size - offset, 1, out);
				if(result)
				{
					perror("fwrite video chunk");
					break;
				}
			}
		}
		else
		if(in->is_program_stream)
		{
			mpeg3_demuxer_t *demuxer = in->vtrack[0]->demuxer;
			result = 0;
//fprintf(stderr, "mpeg3cat 1\n");

/* Append program stream with no changes */
			demuxer->read_all = 1;
			mpeg3demux_seek_byte(demuxer, 0);

			while(!result)
			{
				result = mpeg3_advance_timecode(demuxer, 0);


//fprintf(stderr, "mpeg3cat 1 %d\n", result);
				if(!result) 
				{
					result = mpeg3demux_read_program(demuxer);
					if(result)
						fprintf(stderr, "Hit end of data in %s\n", inpath);
				}


// Decrypt it
				if(!result)
				{
					long decryption_offset = demuxer->last_packet_decryption - demuxer->last_packet_start;
					mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
					mpeg3io_seek(title->fs, demuxer->last_packet_start);
					demuxer->raw_size = demuxer->last_packet_end - demuxer->last_packet_start;
//fprintf(stderr, "mpeg3cat 2 %d %x\n", decryption_offset, demuxer->raw_data[decryption_offset]);
					mpeg3io_read_data(demuxer->raw_data, demuxer->raw_size, title->fs);
					if(decryption_offset > 0 && 
						demuxer->raw_data[decryption_offset] & 0x30)
					{
//fprintf(stderr, "mpeg3cat 3\n");
						if(mpeg3_decrypt_packet(title->fs->css, 
							demuxer->raw_data,
							0))
						{
							fprintf(stderr, "get_ps_pes_packet: Decryption not available\n");
							return 1;
						}
//fprintf(stderr, "mpeg3cat 4\n");
						demuxer->raw_data[decryption_offset] &= 0xcf;
					}
				}

//printf("mpeg3cat 3 %d\n", result);
// Write it
//fprintf(stderr, "mpeg3cat 5\n");
				if(!result) 
				{
					result = !fwrite(demuxer->raw_data, 
							demuxer->raw_size, 
							1, 
							out);
					total_written += demuxer->raw_size;
					if(result) fprintf(stderr, "%s\n", strerror(errno));
				}

//fprintf(stderr, X64 " "X64"\n", mpeg3demux_tell(demuxer), mpeg3demux_tell(demuxer) - total_written);
			}
		}
		else
		{
			fprintf(stderr, "Unsupported stream type.\n");
			mpeg3_close(in);
			in = 0;
			continue;
		}

		mpeg3_close(in);
		in = 0;
		current_output_file++;
	}

/* Terminate output */
	if(current_output_file > 0 && do_video)
	{
/*fprintf(stderr, "\n"); */
/* Write new end of sequence */
		buffer[0] = MPEG3_SEQUENCE_END_CODE >> 24;
		buffer[1] = (MPEG3_SEQUENCE_END_CODE >> 16) & 0xff;
		buffer[2] = (MPEG3_SEQUENCE_END_CODE >> 8) & 0xff;
		buffer[3] = MPEG3_SEQUENCE_END_CODE & 0xff;
		result = !fwrite(buffer, 4, 1, out);
	}
	if(outpath[0]) fclose(out);

	exit(0);
}
