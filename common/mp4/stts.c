#include "quicktime.h"



int quicktime_stts_init(quicktime_stts_t *stts)
{
	stts->version = 0;
	stts->flags = 0;
	stts->total_entries = 0;
	stts->entries_allocated = 0;
}

int quicktime_stts_init_table(quicktime_stts_t *stts)
{
	if(!stts->entries_allocated)
	{
		stts->entries_allocated = 1;
		stts->table = (quicktime_stts_table_t*)
			malloc(sizeof(quicktime_stts_table_t) * stts->entries_allocated);
		stts->total_entries = 1;
	}
}

int quicktime_stts_init_video(quicktime_t *file, quicktime_stts_t *stts, int time_scale, float frame_rate)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;      /* need to set this when closing */
	table->sample_duration = time_scale / frame_rate;
}

int quicktime_stts_init_audio(quicktime_t *file, quicktime_stts_t *stts, int time_scale, int sample_duration)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;     /* need to set this when closing or via update function */
	table->sample_duration = sample_duration;
}

int quicktime_stts_init_hint(quicktime_t *file, quicktime_stts_t *stts, int sample_duration)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;
	table->sample_duration = sample_duration;
}

int quicktime_stts_delete(quicktime_stts_t *stts)
{
	if(stts->total_entries) free(stts->table);
	stts->total_entries = 0;
}

int quicktime_stts_dump(quicktime_stts_t *stts)
{
	int i;
	printf("     time to sample\n");
	printf("      version %d\n", stts->version);
	printf("      flags %d\n", stts->flags);
	printf("      total_entries %d\n", stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		printf("       count %ld duration %ld\n", stts->table[i].sample_count, stts->table[i].sample_duration);
	}
}

int quicktime_read_stts(quicktime_t *file, quicktime_stts_t *stts)
{
	int i;
	stts->version = quicktime_read_char(file);
	stts->flags = quicktime_read_int24(file);
	stts->total_entries = quicktime_read_int32(file);

	stts->table = (quicktime_stts_table_t*)malloc(sizeof(quicktime_stts_table_t) * stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		stts->table[i].sample_count = quicktime_read_int32(file);
		stts->table[i].sample_duration = quicktime_read_int32(file);
	}
}

int quicktime_write_stts(quicktime_t *file, quicktime_stts_t *stts)
{
	int i;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "stts");

	quicktime_write_char(file, stts->version);
	quicktime_write_int24(file, stts->flags);
	quicktime_write_int32(file, stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		quicktime_write_int32(file, stts->table[i].sample_count);
		quicktime_write_int32(file, stts->table[i].sample_duration);
	}
	quicktime_atom_write_footer(file, &atom);
}

int quicktime_update_stts(quicktime_stts_t *stts, long sample_duration)
{
	if (sample_duration == stts->table[stts->total_entries-1].sample_duration) {
		stts->table[stts->total_entries-1].sample_count++;
	} else {
		/* need a new entry in the table */

		/* allocate more entries if necessary */
		if (stts->total_entries >= stts->entries_allocated) {
			stts->entries_allocated *= 2;
			stts->table = (quicktime_stts_table_t*)realloc(stts->table,
				sizeof(quicktime_stts_table_t) * stts->entries_allocated);
		}
	
		stts->table[stts->total_entries].sample_count = 1;
		stts->table[stts->total_entries].sample_duration = sample_duration;
		stts->total_entries++;
	}
}
