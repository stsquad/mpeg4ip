#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <stdlib.h>

mpeg3_vtrack_t* mpeg3_new_vtrack(mpeg3_t *file, 
	int stream_id, 
	mpeg3_demuxer_t *demuxer,
	int number)
{
	int result = 0;
	mpeg3_vtrack_t *new_vtrack;
	new_vtrack = calloc(1, sizeof(mpeg3_vtrack_t));
	new_vtrack->demuxer = mpeg3_new_demuxer(file, 0, 1, stream_id);
	if(new_vtrack->demuxer) mpeg3demux_copy_titles(new_vtrack->demuxer, demuxer);
	new_vtrack->current_position = 0;

//printf("mpeg3_new_vtrack 1\n");
// Copy pointers
	if(file->frame_offsets)
	{
		new_vtrack->frame_offsets = file->frame_offsets[number];
		new_vtrack->total_frame_offsets = file->total_frame_offsets[number];
		new_vtrack->keyframe_numbers = file->keyframe_numbers[number];
		new_vtrack->total_keyframe_numbers = file->total_keyframe_numbers[number];
	}

//printf("mpeg3_new_vtrack 1\n");
//printf("mpeg3_new_vtrack %llx\n", mpeg3demux_tell(new_vtrack->demuxer));
/* Get information about the track here. */
	new_vtrack->video = mpeg3video_new(file, new_vtrack);
	if(!new_vtrack->video)
	{
/* Failed */
		mpeg3_delete_vtrack(file, new_vtrack);
		new_vtrack = 0;
	}

//printf("mpeg3_new_vtrack 2\n");

	return new_vtrack;
}

int mpeg3_delete_vtrack(mpeg3_t *file, mpeg3_vtrack_t *vtrack)
{
	if(vtrack->video) mpeg3video_delete(vtrack->video);
	if(vtrack->demuxer) mpeg3_delete_demuxer(vtrack->demuxer);
	free(vtrack);
	return 0;
}
