#include <stdio.h>
#include "wav.h"

/* wav file writing */

/* Make an unfinished RIFF structure, with null values in the lengths */
int CreateWavHeader(FILE *file, int channels, int samplerate, int resolution)
{
	int length;

	/* Use a zero length for the chunk sizes for now, then modify when finished */
	WAVEAUDIOFORMAT format;

	format.format_tag = 1;
	format.channels = channels;
	format.samplerate = samplerate;
	format.bits_per_sample = resolution;
	format.blockalign = channels * (resolution/8);
	format.bytes_per_second = format.samplerate * format.blockalign;

	fseek(file, 0, SEEK_SET);

	fwrite("RIFF\0\0\0\0WAVEfmt ", sizeof(char), 16, file); /* Write RIFF, WAVE, and fmt  headers */

	length = 16;
	fwrite(&length, 1, sizeof(long), file); /* Length of Format (always 16) */
	fwrite(&format, 1, sizeof(format), file);
	
	fwrite("data\0\0\0\0", sizeof(char), 8, file); /* Write data chunk */

	return 0;
}

/* Update the RIFF structure with proper values. CreateWavHeader must be called first */
int UpdateWavHeader(FILE *file)
{
	/*fpos_t*/int filelen, riff_length, data_length;

	/* Get the length of the file */

	if(!file)
		return -1;

    fseek( file, 0, SEEK_END );
    filelen = ftell( file );

	if(filelen <= 0)
		return -1;

	riff_length = filelen - 8;
	data_length = filelen - 44;

	fseek(file, 4, SEEK_SET);
	fwrite(&riff_length, 1, sizeof(long), file);

	fseek(file, 40, SEEK_SET);
	fwrite(&data_length, 1, sizeof(long), file);

	/* reset file position for appending data */
	fseek(file, 0, SEEK_END);

	return 0;
}
