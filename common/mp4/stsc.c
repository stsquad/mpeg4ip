#include "quicktime.h"



int quicktime_stsc_init(quicktime_stsc_t *stsc)
{
	stsc->version = 0;
	stsc->flags = 0;
	stsc->total_entries = 0;
	stsc->entries_allocated = 0;
}

int quicktime_stsc_init_table(quicktime_t *file, quicktime_stsc_t *stsc)
{
	if(!stsc->total_entries)
	{
		stsc->total_entries = 1;
		stsc->entries_allocated = 1;
		stsc->table = (quicktime_stsc_table_t*)malloc(sizeof(quicktime_stsc_table_t) * stsc->entries_allocated);
	}
}

int quicktime_stsc_init_video(quicktime_t *file, quicktime_stsc_t *stsc)
{
	quicktime_stsc_table_t *table;
	quicktime_stsc_init_table(file, stsc);
	table = &(stsc->table[0]);
	table->chunk = 1;
	table->samples = 1;
	table->id = 1;
}

int quicktime_stsc_init_audio(quicktime_t *file, quicktime_stsc_t *stsc)
{
	quicktime_stsc_table_t *table;
	quicktime_stsc_init_table(file, stsc);
	table = &(stsc->table[0]);
	table->chunk = 1;
	table->samples = 0;         /* set this after completion or after every audio chunk is written */
	table->id = 1;
}

int quicktime_stsc_delete(quicktime_stsc_t *stsc)
{
	if(stsc->total_entries) free(stsc->table);
	stsc->total_entries = 0;
}

int quicktime_stsc_dump(quicktime_stsc_t *stsc)
{
	int i;
	printf("     sample to chunk\n");
	printf("      version %d\n", stsc->version);
	printf("      flags %d\n", stsc->flags);
	printf("      total_entries %d\n", stsc->total_entries);
	for(i = 0; i < stsc->total_entries; i++)
	{
		printf("       chunk %d samples %d id %d\n", 
			stsc->table[i].chunk, stsc->table[i].samples, stsc->table[i].id);
	}
}

int quicktime_read_stsc(quicktime_t *file, quicktime_stsc_t *stsc)
{
	int i;
	stsc->version = quicktime_read_char(file);
	stsc->flags = quicktime_read_int24(file);
	stsc->total_entries = quicktime_read_int32(file);
	
	stsc->entries_allocated = stsc->total_entries;
	stsc->table = (quicktime_stsc_table_t*)malloc(sizeof(quicktime_stsc_table_t) * stsc->total_entries);
	for(i = 0; i < stsc->total_entries; i++)
	{
		stsc->table[i].chunk = quicktime_read_int32(file);
		stsc->table[i].samples = quicktime_read_int32(file);
		stsc->table[i].id = quicktime_read_int32(file);
	}
}


int quicktime_write_stsc(quicktime_t *file, quicktime_stsc_t *stsc)
{
	int i, last_same;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "stsc");

	for(i = 1, last_same = 0; i < stsc->total_entries; i++)
	{
		if(stsc->table[i].samples != stsc->table[last_same].samples)
		{
/* An entry has a different sample count. */
			last_same++;
			if(last_same < i)
			{
/* Move it up the list. */
				stsc->table[last_same] = stsc->table[i];
			}
		}
	}
	last_same++;
	stsc->total_entries = last_same;


	quicktime_write_char(file, stsc->version);
	quicktime_write_int24(file, stsc->flags);
	quicktime_write_int32(file, stsc->total_entries);
	for(i = 0; i < stsc->total_entries; i++)
	{
		quicktime_write_int32(file, stsc->table[i].chunk);
		quicktime_write_int32(file, stsc->table[i].samples);
		quicktime_write_int32(file, stsc->table[i].id);
	}

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_update_stsc(quicktime_stsc_t *stsc, long chunk, long samples)
{
/*	quicktime_stsc_table_t *table = stsc->table; */
	quicktime_stsc_table_t *new_table;
	long i;

	if(chunk > stsc->entries_allocated)
	{
		stsc->entries_allocated = chunk * 2;
		new_table = (quicktime_stsc_table_t*)malloc(sizeof(quicktime_stsc_table_t) * stsc->entries_allocated);
		for(i = 0; i < stsc->total_entries; i++) new_table[i] = stsc->table[i];
		free(stsc->table);
		stsc->table = new_table;
	}

	stsc->table[chunk - 1].samples = samples;
	stsc->table[chunk - 1].chunk = chunk;
	stsc->table[chunk - 1].id = 1;
	if(chunk > stsc->total_entries) stsc->total_entries = chunk;
	return 0;
}

/* Optimizing while writing doesn't allow seeks during recording so */
/* entries are created for every chunk and only optimized during */
/* writeout.  Unfortunately there's no way to keep audio synchronized */
/* after overwriting  a recording as the fractional audio chunk in the */
/* middle always overwrites the previous location of a larger chunk.  On */
/* writing, the table must be optimized.  RealProducer requires an  */
/* optimized table. */

