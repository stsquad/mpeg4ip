#include "quicktime.h"



int quicktime_stsz_init(quicktime_stsz_t *stsz)
{
	stsz->version = 0;
	stsz->flags = 0;
	stsz->sample_size = 0;
	stsz->total_entries = 0;
	stsz->entries_allocated = 0;
}

int quicktime_stsz_init_video(quicktime_t *file, quicktime_stsz_t *stsz)
{
	stsz->sample_size = 0;
	if(!stsz->entries_allocated)
	{
		stsz->entries_allocated = 2000;
		stsz->total_entries = 0;
		stsz->table = (quicktime_stsz_table_t*)malloc(sizeof(quicktime_stsz_table_t) * stsz->entries_allocated);
	}
}

int quicktime_stsz_init_audio(quicktime_t *file, quicktime_stsz_t *stsz, int sample_size)
{
	stsz->sample_size = sample_size;	/* if == 0, then use table */
	stsz->total_entries = 0;   /* set this when closing */
	stsz->entries_allocated = 0;
}

int quicktime_stsz_delete(quicktime_stsz_t *stsz)
{
	if(!stsz->sample_size && stsz->total_entries) 
		free(stsz->table);
	stsz->total_entries = 0;
	stsz->entries_allocated = 0;
}

int quicktime_stsz_dump(quicktime_stsz_t *stsz)
{
	int i;
	printf("     sample size\n");
	printf("      version %d\n", stsz->version);
	printf("      flags %d\n", stsz->flags);
	printf("      sample_size %d\n", stsz->sample_size);
	printf("      total_entries %d\n", stsz->total_entries);
	
	if(!stsz->sample_size)
	{
		for(i = 0; i < stsz->total_entries; i++)
		{
			printf("       sample_size %d\n", stsz->table[i].size);
		}
	}
}

int quicktime_read_stsz(quicktime_t *file, quicktime_stsz_t *stsz)
{
	int i;
	stsz->version = quicktime_read_char(file);
	stsz->flags = quicktime_read_int24(file);
	stsz->sample_size = quicktime_read_int32(file);
	stsz->total_entries = quicktime_read_int32(file);
	stsz->entries_allocated = stsz->total_entries;
	if(!stsz->sample_size)
	{
		stsz->table = (quicktime_stsz_table_t*)malloc(sizeof(quicktime_stsz_table_t) * stsz->entries_allocated);
		for(i = 0; i < stsz->total_entries; i++)
		{
			stsz->table[i].size = quicktime_read_int32(file);
		}
	}
}

int quicktime_write_stsz(quicktime_t *file, quicktime_stsz_t *stsz)
{
	int i, result;
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "stsz");

/* optimize if possible */
/* Xanim requires an unoptimized table for video. */
/* 	if(!stsz->sample_size) */
/* 	{ */
/* 		for(i = 0, result = 0; i < stsz->total_entries && !result; i++) */
/* 		{ */
/* 			if(stsz->table[i].size != stsz->table[0].size) result = 1; */
/* 		} */
/* 		 */
/* 		if(!result) */
/* 		{ */
/* 			stsz->sample_size = stsz->table[0].size; */
/* 			stsz->total_entries = 0; */
/* 			free(stsz->table); */
/* 		} */
/* 	} */

	quicktime_write_char(file, stsz->version);
	quicktime_write_int24(file, stsz->flags);
	quicktime_write_int32(file, stsz->sample_size);
	if(stsz->sample_size)
	{
		quicktime_write_int32(file, stsz->total_entries);
	}
	else
	{
		quicktime_write_int32(file, stsz->total_entries);
		for(i = 0; i < stsz->total_entries; i++)
		{
			quicktime_write_int32(file, stsz->table[i].size);
		}
	}

	quicktime_atom_write_footer(file, &atom);
}

int quicktime_update_stsz(quicktime_stsz_t *stsz, long sample, long sample_size)
{
	quicktime_stsz_table_t *new_table;
	int i;

	if(!stsz->sample_size)
	{
		if(sample >= stsz->entries_allocated)
		{
			stsz->entries_allocated = sample * 2;
			stsz->table = (quicktime_stsz_table_t *)realloc(stsz->table,
				sizeof(quicktime_stsz_table_t) * stsz->entries_allocated);
		}

		stsz->table[sample].size = sample_size;
		if(sample >= stsz->total_entries) 
			stsz->total_entries = sample + 1;
	}
}
