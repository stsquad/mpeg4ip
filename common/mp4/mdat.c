#include "quicktime.h"

int quicktime_mdat_init(quicktime_mdat_t *mdat)
{
	mdat->size = 8;
	mdat->start = 0;
}

int quicktime_mdat_delete(quicktime_mdat_t *mdat)
{
}

int quicktime_read_mdat(quicktime_t *file, quicktime_mdat_t *mdat, quicktime_atom_t *parent_atom)
{
	mdat->size = parent_atom->size;
	mdat->start = parent_atom->start;
	quicktime_atom_skip(file, parent_atom);
}

int quicktime_write_mdat(quicktime_t *file, quicktime_mdat_t *mdat)
{
	long position, size = 0, new_size = 0;
	int i, j;
	
	for(i = 0; i < file->total_atracks; i++)
	{
		new_size = quicktime_track_end(file->atracks[i].track);
		if(new_size > size) 
			size = new_size;

		for(j = 0; j < file->atracks[i].totalHintTracks; j++) {
			new_size = quicktime_track_end(file->atracks[i].hintTracks[j]);
			if(new_size > size) 
				size = new_size;
		}
	}

	for(i = 0; i < file->total_vtracks; i++)
	{
		new_size = quicktime_track_end(file->vtracks[i].track);
		if(new_size > size) 
			size = new_size;

		for(j = 0; j < file->vtracks[i].totalHintTracks; j++) {
			new_size = quicktime_track_end(file->vtracks[i].hintTracks[j]);
			if(new_size > size) 
				size = new_size;
		}
	}
	
	mdat->size = size;
	quicktime_set_position(file, mdat->start);
	quicktime_write_int32(file, mdat->size);
	quicktime_set_position(file, mdat->start + mdat->size);
}
