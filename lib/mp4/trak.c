#include "quicktime.h"




int quicktime_trak_init(quicktime_trak_t *trak)
{
	quicktime_tkhd_init(&(trak->tkhd));
	quicktime_edts_init(&(trak->edts));
	quicktime_tref_init(&(trak->tref));
	quicktime_mdia_init(&(trak->mdia));
	quicktime_hint_udta_init(&(trak->hint_udta));
	return 0;
}

int quicktime_trak_init_video(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int frame_w, 
							int frame_h, 
							float frame_rate,
							int time_scale,
							char *compressor)
{
	quicktime_tkhd_init_video(file, &(trak->tkhd), frame_w, frame_h);
	quicktime_mdia_init_video(file, &(trak->mdia), frame_w, frame_h, 
		frame_rate, time_scale, compressor);
	quicktime_edts_init_table(&(trak->edts));

	return 0;
}

int quicktime_trak_init_audio(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int channels, 
							int sample_rate, 
							int bits, 
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	quicktime_mdia_init_audio(file, &(trak->mdia), channels, sample_rate, bits,
		sample_size, time_scale, sample_duration, compressor);
	quicktime_edts_init_table(&(trak->edts));

	return 0;
}

int quicktime_trak_init_hint(quicktime_t *file, 
							quicktime_trak_t *trak, 
							quicktime_trak_t *refTrak,
							int maxPktSize,
							int time_scale,
							int sampleDuration)
{
	quicktime_tkhd_init_hint(file, &(trak->tkhd));
	quicktime_edts_init_table(&(trak->edts));
	quicktime_tref_init_hint(&(trak->tref), refTrak);
	quicktime_mdia_init_hint(file, &(trak->mdia), 
		refTrak, maxPktSize, time_scale, sampleDuration);
	return 0;
}

int quicktime_trak_is_hint(quicktime_trak_t *trak)
{
	if (trak->tref.hint.numTracks > 0) {
		return 1;
	}
	return 0;
}

int quicktime_trak_delete(quicktime_trak_t *trak)
{
	quicktime_tkhd_delete(&(trak->tkhd));
	quicktime_hint_udta_delete(&(trak->hint_udta));
	return 0;
}


int quicktime_trak_dump(quicktime_trak_t *trak)
{
	printf(" track\n");
	quicktime_tkhd_dump(&(trak->tkhd));
	quicktime_edts_dump(&(trak->edts));
	quicktime_tref_dump(&(trak->tref));
	quicktime_mdia_dump(&(trak->mdia));
	quicktime_hint_udta_dump(&(trak->hint_udta));

	return 0;
}


quicktime_trak_t* quicktime_add_trak(quicktime_moov_t *moov)
{
	if(moov->total_tracks < MAXTRACKS)
	{
		moov->trak[moov->total_tracks] = malloc(sizeof(quicktime_trak_t));
		quicktime_trak_init(moov->trak[moov->total_tracks]);
		moov->total_tracks++;
	}
	return moov->trak[moov->total_tracks - 1];
}

quicktime_trak_t* quicktime_find_track_by_id(quicktime_moov_t *moov, int trackId)
{
	int i;

	for (i = 0; i < moov->total_tracks; i++) {
		if (moov->trak[i]->tkhd.track_id == trackId) {
			return moov->trak[i];
		}
	}
	
	return NULL;
}

int quicktime_delete_trak(quicktime_moov_t *moov, quicktime_trak_t *trak)
{
	int i, j;

	for (i = 0; i < moov->total_tracks; i++) {
		if (moov->trak[i] == trak) {
			quicktime_trak_delete(trak);
			free(trak);
			moov->trak[i] = NULL;
			for (j = i + 1; j < moov->total_tracks; j++, i++) {
				moov->trak[i] = moov->trak[j];
			}
			moov->trak[j] = NULL;
			moov->total_tracks--;
			return 0;
		}
	}
	return -1;
}


int quicktime_read_trak(quicktime_t *file, quicktime_trak_t *trak, quicktime_atom_t *trak_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "tkhd"))
			{ quicktime_read_tkhd(file, &(trak->tkhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "mdia"))
			{ quicktime_read_mdia(file, &(trak->mdia), &leaf_atom); }
		else
/* optional */
		if(quicktime_atom_is(&leaf_atom, "clip"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "matt"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "edts"))
			{ quicktime_read_edts(file, &(trak->edts), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "load"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "tref"))
			{ quicktime_read_tref(file, &(trak->tref), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "imap"))
			{ quicktime_atom_skip(file, &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "udta"))
			{ quicktime_read_hint_udta(file, &(trak->hint_udta), &leaf_atom); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < trak_atom->end);

	return 0;
}

int quicktime_write_trak(quicktime_t *file, quicktime_trak_t *trak, long moov_time_scale)
{
	long duration;
	long timescale;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "trak");
	quicktime_trak_duration(trak, &duration, &timescale);

	/* printf("quicktime_write_trak duration %d\n", duration); */

	/* get duration in movie's units */
	if (timescale) {
		trak->tkhd.duration = 
			(long)((float)duration / timescale * moov_time_scale);
	} else {
		trak->tkhd.duration = 0;
	}
	trak->mdia.mdhd.duration = duration;
	trak->mdia.mdhd.time_scale = timescale;

	if (trak->mdia.minf.is_hint) {
		if (duration && timescale) {
			trak->mdia.minf.hmhd.avgbitrate =
				(trak->hint_udta.hinf.trpy.numBytes * 8)
					/ (duration / timescale);
		} else {
			trak->mdia.minf.hmhd.avgbitrate = 0;
		}
	}

	quicktime_write_tkhd(file, &(trak->tkhd));
	quicktime_write_edts(file, &(trak->edts), trak->tkhd.duration);
	quicktime_write_tref(file, &(trak->tref));
	quicktime_write_mdia(file, &(trak->mdia));
	quicktime_write_hint_udta(file, &(trak->hint_udta));

	quicktime_atom_write_footer(file, &atom);

	return 0;
}

long quicktime_track_end(quicktime_trak_t *trak)
{
/* get the byte endpoint of the track in the file */
	long size = 0;
	long chunk, chunk_offset, chunk_samples, sample;
	quicktime_stsz_t *stsz = &(trak->mdia.minf.stbl.stsz);
	quicktime_stsz_table_t *table = stsz->table;
	quicktime_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	quicktime_stco_t *stco;

/* get the last chunk offset */
/* the chunk offsets contain the HEADER_LENGTH themselves */
	stco = &(trak->mdia.minf.stbl.stco);
	chunk = stco->total_entries;
	size = chunk_offset = stco->table[chunk - 1].offset;

/* get the number of samples in the last chunk */
	chunk_samples = stsc->table[stsc->total_entries - 1].samples;

/* get the size of last samples */
#ifdef NOTDEF
	if(stsz->sample_size)
	{
/* assume audio so calculate the sample size */
		size += chunk_samples * stsz->sample_size
			* trak->mdia.minf.stbl.stsd.table[0].channels 
			* trak->mdia.minf.stbl.stsd.table[0].sample_size / 8;
	}
	else
	{
/* assume video */
#endif
		for(sample = stsz->total_entries - chunk_samples; 
			sample >= 0 && sample < stsz->total_entries; sample++)
		{
			size += stsz->table[sample].size;
		}
#ifdef NOTDEF
	}
#endif

	/* for hint tracks */
	if (quicktime_trak_is_hint(trak)) {
		/* compute maximum packet duration */
		if (trak->hint_udta.hinf.maxr.maxBitRate != 0) {
			trak->hint_udta.hinf.dmax.milliSecs = 
				(trak->hint_udta.hinf.pmax.numBytes * 8) *
				((float)trak->hint_udta.hinf.maxr.granularity /
					(float)trak->hint_udta.hinf.maxr.maxBitRate);
		}
	}

	return size;
}

long quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak)
{
/*printf("file->rd %d file->wr %d\n", file->rd, file->wr); */
	if(file->wr)
	{
/* get the sample count when creating a new file */
 		quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
		long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
		long chunk = trak->mdia.minf.stbl.stco.total_entries;
		long sample;

		if(chunk)
		{
			sample = quicktime_sample_of_chunk(trak, chunk);
			sample += table[total_entries - 1].samples;
		}
		else 
			sample = 0;
		
		return sample;
	}
	else
	{
/* get the sample count when reading only */
		quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
		int i;
		long total = 0;

		for(i = 0; i < stts->total_entries; i++)
		{
			total += stts->table[i].sample_count;
		}
		return total;
	}
}

long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk)
{
	quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
	long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
	long chunk1entry, chunk2entry;
	long chunk1, chunk2, chunks, total = 0;

	for(chunk1entry = total_entries - 1, chunk2entry = total_entries; 
		chunk1entry >= 0; 
		chunk1entry--, chunk2entry--)
	{
		chunk1 = table[chunk1entry].chunk;

		if(chunk > chunk1)
		{
			if(chunk2entry < total_entries)
			{
				chunk2 = table[chunk2entry].chunk;

				if(chunk < chunk2) chunk2 = chunk;
			}
			else
				chunk2 = chunk;

			chunks = chunk2 - chunk1;

			total += chunks * table[chunk1entry].samples;
		}
	}

	return total;
}

int quicktime_chunk_of_sample(long *chunk_sample, long *chunk, quicktime_trak_t *trak, long sample)
{
	quicktime_stsc_table_t *table = NULL;
	long total_entries = 0;
	long chunk2entry;
	long chunk1, chunk2, chunk1samples, range_samples, total = 0;

	if (trak == NULL) {
		return -1;
	}
 	table = trak->mdia.minf.stbl.stsc.table;
	total_entries = trak->mdia.minf.stbl.stsc.total_entries;

	chunk1 = 1;
	chunk1samples = 0;
	chunk2entry = 0;

	do
	{
		chunk2 = table[chunk2entry].chunk;
		*chunk = chunk2 - chunk1;
		range_samples = *chunk * chunk1samples;

		if(sample < total + range_samples) break;

		chunk1samples = table[chunk2entry].samples;
		chunk1 = chunk2;

		if(chunk2entry < total_entries)
		{
			chunk2entry++;
			total += range_samples;
		}
	}while(chunk2entry < total_entries);

	if(chunk1samples)
		*chunk = (sample - total) / chunk1samples + chunk1;
	else
		*chunk = 1;

	*chunk_sample = total + (*chunk - chunk1) * chunk1samples;
	return 0;
}

long quicktime_chunk_to_offset(quicktime_trak_t *trak, long chunk)
{
	quicktime_stco_table_t *table = NULL;

	if (trak == NULL) {
		return -1;
	}
	table = trak->mdia.minf.stbl.stco.table;

	if(trak->mdia.minf.stbl.stco.total_entries && chunk > trak->mdia.minf.stbl.stco.total_entries)
		return table[trak->mdia.minf.stbl.stco.total_entries - 1].offset;
	else
	if(trak->mdia.minf.stbl.stco.total_entries)
		return table[chunk - 1].offset;
	else
		return HEADER_LENGTH;
	
	return 0;
}

long quicktime_offset_to_chunk(long *chunk_offset, quicktime_trak_t *trak, long offset)
{
	quicktime_stco_table_t *table = trak->mdia.minf.stbl.stco.table;
	int i;

	for(i = trak->mdia.minf.stbl.stco.total_entries - 1; i >= 0; i--)
	{
		if(table[i].offset <= offset)
		{
			*chunk_offset = table[i].offset;
			return i + 1;
		}
	}
	*chunk_offset = HEADER_LENGTH;
	return 1;
}

long quicktime_sample_range_size(quicktime_trak_t *trak, long chunk_sample, long sample)
{
	quicktime_stsz_table_t *table = trak->mdia.minf.stbl.stsz.table;
	long i, total;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
/* assume audio */
		return quicktime_samples_to_bytes(trak, sample - chunk_sample);
	}
	else
	{
/* probably video */
		for(i = chunk_sample, total = 0; i < sample; i++)
		{
			total += trak->mdia.minf.stbl.stsz.table[i].size;
		}
	}
	return total;
}

long quicktime_sample_to_offset(quicktime_trak_t *trak, long sample)
{
	long chunk, chunk_sample, chunk_offset1, chunk_offset2;

	if (trak == NULL) {
		return -1;
	}

	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
	chunk_offset1 = quicktime_chunk_to_offset(trak, chunk);
	chunk_offset2 = chunk_offset1 + quicktime_sample_range_size(trak, chunk_sample, sample);
/*printf("quicktime_sample_to_offset chunk %d sample %d chunk_offset %d chunk_sample %d chunk_offset + samples %d\n", */
/*	 chunk, sample, chunk_offset1, chunk_sample, chunk_offset2); */
	return chunk_offset2;
}

long quicktime_offset_to_sample(quicktime_trak_t *trak, long offset)
{
	long chunk_offset;
	long chunk = quicktime_offset_to_chunk(&chunk_offset, trak, offset);
	long chunk_sample = quicktime_sample_of_chunk(trak, chunk);
	long sample, sample_offset;
	quicktime_stsz_table_t *table = trak->mdia.minf.stbl.stsz.table;
	long total_samples = trak->mdia.minf.stbl.stsz.total_entries;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		sample = chunk_sample + (offset - chunk_offset) / 
			trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	for(sample = chunk_sample, sample_offset = chunk_offset; 
		sample_offset < offset && sample < total_samples; )
	{
		sample_offset += table[sample].size;
		if(sample_offset < offset) sample++;
	}
	
	return sample;
}

int quicktime_update_tables(quicktime_t *file, 
							quicktime_trak_t *trak, 
							long offset, 
							long chunk, 
							long sample, 
							long samples, 
							long sample_size,
							long sample_duration,
							u_char isSyncSample,
							long renderingOffset)
{
	if (offset + sample_size > file->mdat.size) {
		file->mdat.size = offset + sample_size;
	}
	quicktime_update_stco(&(trak->mdia.minf.stbl.stco), chunk, offset);
	if (sample_size) {
		quicktime_update_stsz(&(trak->mdia.minf.stbl.stsz), sample, sample_size);
	}
	quicktime_update_stsc(&(trak->mdia.minf.stbl.stsc), chunk, samples);
	if (sample_duration) {
		quicktime_update_stts(&(trak->mdia.minf.stbl.stts), sample_duration);
	}
	if (isSyncSample) {
		quicktime_update_stss(&(trak->mdia.minf.stbl.stss), sample);
	}
	quicktime_update_ctts(&(trak->mdia.minf.stbl.ctts), renderingOffset);
	return 0;
}

int quicktime_trak_duration(quicktime_trak_t *trak, long *duration, long *timescale)
{
	quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
	int i;
	*duration = 0;

	/* hint track duration is that of reference track */
	if (quicktime_trak_is_hint(trak)) {
#ifdef DEBUG
		if ((quicktime_trak_t*)trak->tref.hint.traks[0] == trak) {
			printf("quicktime_trak_duration: BUG, self reference loop!!!\n");
			*duration = 0;
			*timescale = 1;
			return 0;
		}
		if ((quicktime_trak_t*)trak->tref.hint.traks[0] == NULL) {
			printf("quicktime_trak_duration: BUG, NULL reference!!!\n");
			*duration = 0;
			*timescale = 1;
			return 0;
		}
#endif /* DEBUG */

		return quicktime_trak_duration(
			(quicktime_trak_t*)trak->tref.hint.traks[0], duration, timescale);
	}

	for(i = 0; i < stts->total_entries; i++) {
		*duration += stts->table[i].sample_duration * stts->table[i].sample_count;
	}

	*timescale = trak->mdia.mdhd.time_scale;
	return 0;
}

int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak)
{
	long samples = quicktime_track_samples(file, trak);

	if (trak->mdia.minf.stbl.stts.total_entries == 1) {
		trak->mdia.minf.stbl.stts.table[0].sample_count = samples;
	}

	if(trak->mdia.minf.stbl.stsz.sample_size)
		trak->mdia.minf.stbl.stsz.total_entries = samples;

	return 0;
}

long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk)
{
	long result, current_chunk;
	quicktime_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	long i = stsc->total_entries - 1;

	do
	{
		current_chunk = stsc->table[i].chunk;
		result = stsc->table[i].samples;
		i--;
	}while(i >= 0 && current_chunk > chunk);

	return result;
}

int quicktime_trak_shift_offsets(quicktime_trak_t *trak, long offset)
{
	quicktime_stco_t *stco = &(trak->mdia.minf.stbl.stco);
	int i;

	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset += offset;
	}
	return 0;
}
