#include "../mpeg3private.h"
#include "../mpeg3protos.h"
#include "mpeg3video.h"
#include <stdlib.h>
#include <string.h>

unsigned int mpeg3bits_next_startcode(mpeg3_bits_t* stream)
{
/* Perform forwards search */
	mpeg3bits_byte_align(stream);


//printf("mpeg3bits_next_startcode 1 %lld %lld\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);


//mpeg3_read_next_packet(stream->demuxer);
//printf("mpeg3bits_next_startcode 2 %d %d\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);

/* Perform search */
	while(1)
	{
		unsigned int code = mpeg3bits_showbits32_noptr(stream);

//printf("mpeg3bits_next_startcode 2 %lx\n", code);
		if((code >> 8) == MPEG3_PACKET_START_CODE_PREFIX) break;
		if(mpeg3bits_eof(stream)) break;


		mpeg3bits_getbyte_noptr(stream);

/*
 * printf("mpeg3bits_next_startcode 3 %08x %d %d\n", 
 * 	mpeg3bits_showbits32_noptr(stream), 
 * 	stream->demuxer->titles[0]->fs->current_byte, 
 * 	stream->demuxer->titles[0]->fs->total_bytes);
 */
	}
//printf("mpeg3bits_next_startcode 4 %d %d\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);
	return mpeg3bits_showbits32_noptr(stream);
}

/* Line up on the beginning of the next code. */
int mpeg3video_next_code(mpeg3_bits_t* stream, unsigned int code)
{
	while(!mpeg3bits_eof(stream) && 
		mpeg3bits_showbits32_noptr(stream) != code)
	{
		mpeg3bits_getbyte_noptr(stream);
	}
	return mpeg3bits_eof(stream);
}

/* Line up on the beginning of the previous code. */
int mpeg3video_prev_code(mpeg3_bits_t* stream, unsigned int code)
{
	while(!mpeg3bits_bof(stream) && 
		mpeg3bits_showbits_reverse(stream, 32) != code)
	{
//printf("mpeg3video_prev_code %08x %08x\n", 
//	mpeg3bits_showbits_reverse(stream, 32), mpeg3demux_tell(stream->demuxer));
		mpeg3bits_getbits_reverse(stream, 8);
	}
	return mpeg3bits_bof(stream);
}

long mpeg3video_goptimecode_to_frame(mpeg3video_t *video)
{
/*  printf("mpeg3video_goptimecode_to_frame %d %d %d %d %f\n",  */
/*  	video->gop_timecode.hour, video->gop_timecode.minute, video->gop_timecode.second, video->gop_timecode.frame, video->frame_rate); */
	return (long)(video->gop_timecode.hour * 3600 * video->frame_rate + 
		video->gop_timecode.minute * 60 * video->frame_rate +
		video->gop_timecode.second * video->frame_rate +
		video->gop_timecode.frame) - 1 - video->first_frame;
}

int mpeg3video_match_refframes(mpeg3video_t *video)
{
	unsigned char *dst, *src;
	int i, j, size;

	for(i = 0; i < 3; i++)
	{
		if(video->newframe[i])
		{
			if(video->newframe[i] == video->refframe[i])
			{
				src = video->refframe[i];
				dst = video->oldrefframe[i];
			}
			else
			{
				src = video->oldrefframe[i];
				dst = video->refframe[i];
			}

    		if(i == 0)
				size = video->coded_picture_width * video->coded_picture_height + 32 * video->coded_picture_width;
    		else 
				size = video->chrom_width * video->chrom_height + 32 * video->chrom_width;

			memcpy(dst, src, size);
		}
	}
	return 0;
}

// Must perform seeking now to get timecode for audio
int mpeg3video_seek_percentage(mpeg3video_t *video, double percentage)
{
	video->percentage_seek = percentage;
	return 0;
}

int mpeg3video_seek_frame(mpeg3video_t *video, long frame)
{
	video->frame_seek = frame;
	return 0;
}

int mpeg3video_seek(mpeg3video_t *video)
{
	long this_gop_start;
	int result = 0;
	int back_step;
	int attempts;
	mpeg3_t *file = video->file;
	mpeg3_bits_t *vstream = video->vstream;
	mpeg3_vtrack_t *track = video->track;
	double percentage;
	long frame_number;
	int match_refframes = 1;


//printf("mpeg3video_seek 1 %d\n", video->frame_seek);
// Must do seeking here so files which don't use video don't seek.
/* Seek to percentage */
	if(video->percentage_seek >= 0)
	{








//printf("mpeg3video_seek 1 %f\n", video->percentage_seek);
		percentage = video->percentage_seek;
		video->percentage_seek = -1;
		mpeg3bits_seek_percentage(vstream, percentage);


// Go to previous I-frame #1

		mpeg3bits_start_reverse(vstream);
//printf("mpeg3video_seek 2\n");

		if(video->has_gops)
			result = mpeg3video_prev_code(vstream, MPEG3_GOP_START_CODE);
		else
			result = mpeg3video_prev_code(vstream, MPEG3_SEQUENCE_START_CODE);
//printf("mpeg3video_seek 3\n");

		if(!result) mpeg3bits_getbits_reverse(vstream, 32);



// Go to previous I-frame #2
		if(video->has_gops)
			result = mpeg3video_prev_code(vstream, MPEG3_GOP_START_CODE);
		else
			result = mpeg3video_prev_code(vstream, MPEG3_SEQUENCE_START_CODE);

		if(!result) mpeg3bits_getbits_reverse(vstream, 32);




		mpeg3bits_start_forward(vstream);

// Reread first two I frames
		if(mpeg3bits_tell_percentage(vstream) <= 0) 
		{
			mpeg3bits_seek_percentage(vstream, 0);
			mpeg3video_get_firstframe(video);
			mpeg3video_read_frame_backend(video, 0);
		}
//printf("mpeg3video_seek 4\n");



// Read up to the correct percentage
		result = 0;
		while(!result && mpeg3bits_tell_percentage(vstream) < percentage)
		{
			result = mpeg3video_read_frame_backend(video, 0);
		}
//printf("mpeg3video_seek 5\n");








	}
	else
/* Seek to a frame */
	if(video->frame_seek >= 0)
	{
		frame_number = video->frame_seek;
		video->frame_seek = -1;
		if(frame_number < 0) frame_number = 0;
		if(frame_number > video->maxframe) frame_number = video->maxframe;

//printf("mpeg3video_seek 1 %ld %ld\n", frame_number, video->framenum);

/* Seek to I frame in table of contents */
		if(track->frame_offsets)
		{
			int i;
			for(i = track->total_keyframe_numbers - 1; i >= 0; i--)
			{
				if(track->keyframe_numbers[i] <= frame_number)
				{
					int frame;
					int title_number;
					int64_t byte;

// Go 2 I-frames before current position
					if(i > 0) i--;

					frame = track->keyframe_numbers[i];
					title_number = (track->frame_offsets[frame] & 
						0xff00000000000000) >> 56;
					byte = track->frame_offsets[frame] & 
						  0xffffffffffffff;

					video->framenum = track->keyframe_numbers[i];

					mpeg3bits_open_title(vstream, title_number);
					mpeg3bits_seek_byte(vstream, byte);

//printf("mpeg3video_seek 2 title_number=%d byte=%llx\n", title_number, byte);

// Get first 2 I-frames
					if(byte == 0)
					{
						mpeg3video_get_firstframe(video);
						mpeg3video_read_frame_backend(video, 0);
					}
					
					
//printf("mpeg3video_seek 2 %ld %ld\n", frame_number, video->framenum);

					mpeg3video_drop_frames(video, frame_number - video->framenum);
					break;
				}
			}
		}
		else
/* Seek to start of file */
		if(frame_number < 16)
		{
			video->repeat_count = video->current_repeat = 0;
			mpeg3bits_seek_start(vstream);
			video->framenum = 0;
			result = mpeg3video_drop_frames(video, 
				frame_number - video->framenum);
		}
		else
		{
/* Seek to an I frame. */
			if((frame_number < video->framenum || 
				frame_number - video->framenum > MPEG3_SEEK_THRESHOLD))
			{



/* Elementary stream */
				if(file->is_video_stream)
				{
					mpeg3_t *file = video->file;
					mpeg3_vtrack_t *track = video->track;
					int64_t byte = (int64_t)((double)(mpeg3demuxer_total_bytes(vstream->demuxer) / 
						track->total_frames) * 
						frame_number);
					long minimum = 65535;
					int done = 0;

//printf("seek elementary %d\n", frame_number);
/* Get GOP just before frame */
					do
					{
						result = mpeg3bits_seek_byte(vstream, byte);
						mpeg3bits_start_reverse(vstream);

						if(!result) result = mpeg3video_prev_code(vstream, MPEG3_GOP_START_CODE);
						mpeg3bits_start_forward(vstream);
						mpeg3bits_getbits(vstream, 8);
						if(!result) result = mpeg3video_getgophdr(video);
						this_gop_start = mpeg3video_goptimecode_to_frame(video);

//printf("wanted %ld guessed %ld byte %ld result %d\n", frame_number, this_gop_start, byte, result);
						if(labs(this_gop_start - frame_number) >= labs(minimum)) 
							done = 1;
						else
						{
							minimum = this_gop_start - frame_number;
							byte += (long)((float)(frame_number - this_gop_start) * 
								(float)(mpeg3demuxer_total_bytes(vstream->demuxer) / 
								track->total_frames));
							if(byte < 0) byte = 0;
						}
					}while(!result && !done);

//printf("wanted %d guessed %d\n", frame_number, this_gop_start);
					if(!result)
					{
						video->framenum = this_gop_start;
						result = mpeg3video_drop_frames(video, frame_number - video->framenum);
					}
				}
				else




/* System stream */
				{
					mpeg3bits_seek_time(vstream, (double)frame_number / video->frame_rate);

					percentage = mpeg3bits_tell_percentage(vstream);
//printf("seek frame %ld percentage %f byte %ld\n", frame_number, percentage, mpeg3bits_tell(vstream));
					mpeg3bits_start_reverse(vstream);
					mpeg3video_prev_code(vstream, MPEG3_GOP_START_CODE);
					mpeg3bits_getbits_reverse(vstream, 32);
					mpeg3bits_start_forward(vstream);
//printf("seek system 1 %f\n", (double)frame_number / video->frame_rate);

					while(!result && mpeg3bits_tell_percentage(vstream) < percentage)
					{
						result = mpeg3video_read_frame_backend(video, 0);
						if(match_refframes)
							mpeg3video_match_refframes(video);

//printf("seek system 2 %f %f\n", mpeg3bits_tell_percentage(vstream) / percentage);
						match_refframes = 0;
					}
//printf("seek system 3 %f\n", (double)frame_number / video->frame_rate);
				}

				video->framenum = frame_number;
			}
			else
// Drop frames
			{
				mpeg3video_drop_frames(video, frame_number - video->framenum);
			}
		}
	}

	return result;
}

int mpeg3video_previous_frame(mpeg3video_t *video)
{
	if(mpeg3bits_tell_percentage(video->vstream) <= 0) return 1;

// Get one picture
	mpeg3bits_start_reverse(video->vstream);
	mpeg3video_prev_code(video->vstream, MPEG3_PICTURE_START_CODE);
	mpeg3bits_getbits_reverse(video->vstream, 32);

	if(mpeg3bits_bof(video->vstream)) 
		mpeg3bits_seek_percentage(video->vstream, 0);
	mpeg3bits_start_forward(video->vstream);
	video->repeat_count = 0;
	return 0;
}

int mpeg3video_drop_frames(mpeg3video_t *video, long frames)
{
	int result = 0;
	long frame_number = video->framenum + frames;

/* Read the selected number of frames and skip b-frames */
	while(!result && frame_number > video->framenum)
	{
		result = mpeg3video_read_frame_backend(video, frame_number - video->framenum);
	}
	return result;
}
