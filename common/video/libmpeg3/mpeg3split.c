#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpeg3private.inc"

void copy_data(FILE *out, FILE *in, long bytes)
{
	long fragment_size = 0x100000;
	char *buffer = malloc(fragment_size);
	long i;
	
	for(i = 0; i < bytes; i += fragment_size)
	{
		if(i + fragment_size > bytes) fragment_size = bytes - i;
		if(!fread(buffer, 1, fragment_size, in))
		{
			perror("copy_data");
		}
		if(!fwrite(buffer, 1, fragment_size, out))
		{
			perror("copy_data");
		}
	}
	
	free(buffer);
}

void split_video(char *path, long default_fragment_size)
{
	long current_byte = 0;
	int result = 0;
	int i = 0;
	long total_bytes;
	FILE *in = fopen(path, "r");
	char *sequence_hdr;
	long sequence_hdr_size;
	unsigned long header = 0;
	long header_start;
	long header_end;

	if(!in)
	{
		perror("split_file");
		return;
	}
	fseek(in, 0, SEEK_END);
	total_bytes = ftell(in);
	fseek(in, 0, SEEK_SET);

// Copy sequence header
	do{
		header <<= 8;
		header = (header & 0xffffffff) | getc(in);
	}while(header != MPEG3_SEQUENCE_START_CODE && !feof(in));

	header_start = ftell(in) - 4;
	do{
		header <<= 8;
		header = (header & 0xffffffff) | getc(in);
	}while(header != MPEG3_GOP_START_CODE && !feof(in));
	header_end = ftell(in) - 4;

	sequence_hdr_size = header_end - header_start;
	sequence_hdr = malloc(sequence_hdr_size);
	fseek(in, header_start, SEEK_SET);
	fread(sequence_hdr, 1, sequence_hdr_size, in);
	fseek(in, 0, SEEK_SET);

	while(current_byte < total_bytes && !result)
	{
		FILE *out;
		char outpath[1024];
		long fragment_size;

		sprintf(outpath, "%s%02d", path, i);
		out = fopen(outpath, "w");
		if(!out)
		{
			perror("split_file");
			break;
		}

// Default fragment size
		fragment_size = default_fragment_size;
// Corrected fragment size
		if(current_byte + fragment_size >= total_bytes)
		{
			fragment_size = total_bytes - current_byte;
		}
		else
		{
			header = 0;

// Get GOP header
			fseek(in, current_byte + fragment_size, SEEK_SET);
			do
			{
				fseek(in, -1, SEEK_CUR);
				header >>= 8;
				header |= ((unsigned long)fgetc(in)) << 24;
				fseek(in, -1, SEEK_CUR);
			}while(ftell(in) > 0 && header != MPEG3_GOP_START_CODE);
			fragment_size = ftell(in) - current_byte;
			fseek(in, current_byte, SEEK_SET);
		}

// Put sequence header
		if(current_byte > 0)
		{
			fwrite(sequence_hdr, 1, sequence_hdr_size, out);
		}

// Put data
		copy_data(out, in, fragment_size);

		fclose(out);
		i++;
		current_byte += fragment_size; 
	}
	free(sequence_hdr);
}

int main(int argc, char *argv[])
{
	long bytes = 0;
	int i;
	
	if(argc < 2)
	{
		fprintf(stderr, "Split elementary streams into chunks of bytes.\n"
			"Usage: mpeg3split -b bytes <infile>\n");
		exit(1);
	}
	
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-b"))
		{
			if(i < argc - 1)
			{
				i++;
				bytes = atol(argv[i]);
			}
			else
			{
				fprintf(stderr, "-b must be paired with a value\n");
				exit(1);
			}
		}
		else
		if(bytes > 0)
		{
// Split a file
			split_video(argv[i], bytes);
		}
		else
		{
			fprintf(stderr, "No value for bytes specified,\n");
			exit(1);
		}
	}
}


