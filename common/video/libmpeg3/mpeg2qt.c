#include <pthread.h>
#include <signal.h>

#include "colormodels.h"
#include "libmpeg3.h"
#include "quicktime.h"

#define OUTPUT_PATH "movie.mov"
#define VIDEO_CODEC QUICKTIME_DIVX
//#define VIDEO_CODEC QUICKTIME_JPEG
//#define VIDEO_CODEC QUICKTIME_YUV420
//#define AUDIO_CODEC QUICKTIME_TWOS
#define AUDIO_CODEC QUICKTIME_VORBIS


// Hack for libdv to remove glib dependancy

void
g_log (const char    *log_domain,
       int  log_level,
       const char    *format,
       ...)
{
}

void
g_logv (const char    *log_domain,
       int  log_level,
       const char    *format,
       ...)
{
}



// Hack for XFree86 4.1.0

int atexit(void (*function)(void))
{
	return 0;
}


// Dump mpeg video to a quicktime movie

quicktime_t *output;
mpeg3_t *input;
pthread_mutex_t mutex;
int predicate = 0;

void* trap_interrupt()
{
	pthread_mutex_lock(&mutex);
	if(!predicate)
	{
		predicate = 1;

		printf("interrupt trapped 1\n");
		quicktime_close(output);
		printf("interrupt trapped 2\n");
		exit(0);
// Can't join any threads in the mpeg library which may have been interrupted
// before unblocking.
//		mpeg3_close(input);
	}
}





int main(int argc, char *argv[])
{
	int frame_count = -1;
	char *row_pointers[3];
	int do_audio = 0;
	int do_video = 0;
	int channels = 0;
	long afragment = 65536;
	float **audio_output;
	int layer = 0;
	int astream = 0;
	char input_path[1024];
	char output_path[1024];
	int i;
	long current_frame = 0;
	long current_sample = 0;

	pthread_mutex_init(&mutex, NULL);
	signal(SIGINT, trap_interrupt);

	input_path[0] = 0;
	output_path[0] = 0;

	if(argc < 3)
	{
		printf("Usage: %s [-a] [-v] <mpeg file> <output movie> [frame count]\n", argv[0]);
		exit(1);
	}
	
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-a"))
		{
			do_audio = 1;
		}
		else
		if(!strcmp(argv[i], "-v"))
		{
			do_video = 1;
		}
		else
		if(!input_path[0])
		{
			strcpy(input_path, argv[i]);
		}
		else
		if(!output_path[0])
		{
			strcpy(output_path, argv[i]);
		}
		else
			frame_count = atol(argv[i]);
	}

	if(!(input = mpeg3_open(input_path)))
	{
		exit(1);
	}

	if(!(output = quicktime_open(output_path, 0, 1)))
	{
		exit(1);
	}

	if(do_video)
	{
		if(!mpeg3_total_vstreams(input))
		{
			do_video = 0;
		}
		else
		{
			quicktime_set_video(output, 
				1, 
				mpeg3_video_width(input, layer), 
				mpeg3_video_height(input, layer), 
				mpeg3_frame_rate(input, layer), 
				VIDEO_CODEC);
			quicktime_set_jpeg(output, 80, 0);
		}
	}

	if(do_audio)
	{
		if(!mpeg3_total_astreams(input))
		{
			do_audio = 0;
		}
		else
		{
			int i;
			channels = mpeg3_audio_channels(input, astream);

			quicktime_set_audio(output, 
				channels, 
				mpeg3_sample_rate(input, 0), 
				24, 
				AUDIO_CODEC);
			
			audio_output = malloc(sizeof(float*) * channels);
			for(i = 0; i < channels; i++)
				audio_output[i] = malloc(sizeof(float) * afragment);
		}
	}

//	quicktime_set_jpeg(output, 100, 0);
	mpeg3_set_mmx(input, 0);

	while((!(do_video && mpeg3_end_of_video(input, layer)) || 
			!(do_audio && mpeg3_end_of_audio(input, astream))) &&
		(current_frame < frame_count || frame_count < 0))
	{
//printf("%d %d\n", mpeg3_end_of_video(input, layer), mpeg3_end_of_audio(input, astream));
		if(do_audio)
		{
			if(!mpeg3_end_of_audio(input, astream))
			{
				int fragment = afragment;
				int i, j, k;

				i = astream;
				k = 0;
				for(j = 0; j < mpeg3_audio_channels(input, i); j++, k++)
				{
					if(j == 0)
						mpeg3_read_audio(input, 
							audio_output[k], 	 /* Pointer to pre-allocated buffer of floats */
							0,      /* Pointer to pre-allocated buffer of int16's */
							j,          /* Channel to decode */
							fragment,         /* Number of samples to decode */
							i);
					else
						mpeg3_reread_audio(input, 
							audio_output[k], 	 /* Pointer to pre-allocated buffer of floats */
							0,      /* Pointer to pre-allocated buffer of int16's */
							j,          /* Channel to decode */
							fragment,         /* Number of samples to decode */
							i);
				}



				quicktime_encode_audio(output, 
					0, 
					audio_output, 
					fragment);

				current_sample += fragment;
				if(!do_video)
				{
					printf(" %d samples written\r", current_sample);
					fflush(stdout);
				}
			}
			else
				current_sample += afragment;
		}

		if(do_video)
		{
			if(!mpeg3_end_of_video(input, layer))
			{
				int fragment;
				if(do_audio)
					fragment = (long)((double)current_sample / 
							mpeg3_sample_rate(input, 0) *
							mpeg3_frame_rate(input, layer) - 
						current_frame);
				else
					fragment = 1;

				for(i = 0; i < fragment && !mpeg3_end_of_video(input, layer); i++)
				{
					mpeg3_read_yuvframe_ptr(input,
						&row_pointers[0],
						&row_pointers[1],
						&row_pointers[2],
						layer);

					switch(mpeg3_colormodel(input, layer))
					{
						case MPEG3_YUV420P:
							quicktime_set_cmodel(output, BC_YUV420P);
							break;
						case MPEG3_YUV422P:
							quicktime_set_cmodel(output, BC_YUV422P);
							break;
					}
					quicktime_encode_video(output, 
						(unsigned char **)row_pointers, 
						0);

					current_frame++;
					printf(" %d frames written\r", current_frame);
					fflush(stdout);
				}
			}
		}
	}

	printf("\n");
	quicktime_close(output);
	mpeg3_close(input);
}








