#include "quicktime.h"



int quicktime_moov_init(quicktime_moov_t *moov)
{
	int i;

	moov->total_tracks = 0;
	for(i = 0 ; i < MAXTRACKS; i++) moov->trak[i] = 0;
	quicktime_mvhd_init(&(moov->mvhd));
	quicktime_iods_init(&(moov->iods));
	quicktime_udta_init(&(moov->udta));
	quicktime_ctab_init(&(moov->ctab));
	return 0;
}

int quicktime_moov_delete(quicktime_moov_t *moov)
{
	int i;
	while(moov->total_tracks) {
		quicktime_delete_trak(moov, moov->trak[moov->total_tracks - 1]);
	}
	quicktime_mvhd_delete(&(moov->mvhd));
	quicktime_iods_delete(&(moov->iods));
	quicktime_udta_delete(&(moov->udta));
	quicktime_ctab_delete(&(moov->ctab));
	return 0;
}

int quicktime_moov_dump(quicktime_moov_t *moov)
{
	int i;
	printf("movie\n");
	quicktime_mvhd_dump(&(moov->mvhd));
	quicktime_iods_dump(&(moov->iods));
	quicktime_udta_dump(&(moov->udta));
	for(i = 0; i < moov->total_tracks; i++)
		quicktime_trak_dump(moov->trak[i]);
	quicktime_ctab_dump(&(moov->ctab));
}


int quicktime_read_moov(quicktime_t *file, quicktime_moov_t *moov, quicktime_atom_t *parent_atom)
{
/* mandatory mvhd */
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
		
		if(quicktime_atom_is(&leaf_atom, "mvhd"))
		{
			quicktime_read_mvhd(file, &(moov->mvhd));
		}
		else
		if(quicktime_atom_is(&leaf_atom, "iods"))
		{
			quicktime_read_iods(file, &(moov->iods));
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "clip"))
		{
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "trak"))
		{
			quicktime_trak_t *trak = quicktime_add_trak(moov);
			quicktime_read_trak(file, trak, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "udta"))
		{
			quicktime_read_udta(file, &(moov->udta), &leaf_atom);
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "ctab"))
		{
			quicktime_read_ctab(file, &(moov->ctab));
		}
		else
		{
			quicktime_atom_skip(file, &leaf_atom);
		}
	}while(quicktime_position(file) < parent_atom->end);
	
	return 0;
}

int quicktime_write_moov(quicktime_t *file, quicktime_moov_t *moov)
{
	quicktime_atom_t atom;
	int i;
	long longest_duration = 0;
	long duration, timescale;
	quicktime_atom_write_header(file, &atom, "moov");

/* get the duration from the longest track in the mvhd's timescale */
	for(i = 0; i < moov->total_tracks; i++)
	{
		quicktime_trak_fix_counts(file, moov->trak[i]);
		quicktime_trak_duration(moov->trak[i], &duration, &timescale);

		duration = (long)((float)duration / timescale * moov->mvhd.time_scale);

		if(duration > longest_duration)
		{
			longest_duration = duration;
		}
	}
	moov->mvhd.duration = longest_duration;
	moov->mvhd.selection_duration = longest_duration;

	quicktime_write_mvhd(file, &(moov->mvhd));
	quicktime_write_iods(file, &(moov->iods));
	quicktime_write_udta(file, &(moov->udta));

	for(i = 0; i < moov->total_tracks; i++)
	{
		quicktime_write_trak(file, moov->trak[i], moov->mvhd.time_scale);
	}
	/*quicktime_write_ctab(file, &(moov->ctab)); */

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_update_durations(quicktime_moov_t *moov)
{
	
}

int quicktime_shift_offsets(quicktime_moov_t *moov, long offset)
{
	int i;
	for(i = 0; i < moov->total_tracks; i++)
	{
		quicktime_trak_shift_offsets(moov->trak[i], offset);
	}
	return 0;
}
