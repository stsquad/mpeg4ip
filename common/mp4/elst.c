#include "quicktime.h"


int quicktime_elst_table_init(quicktime_elst_table_t *table)
{
	table->duration = 0;
	table->time = 0;
	table->rate = 1;
}

int quicktime_elst_table_delete(quicktime_elst_table_t *table)
{
}

int quicktime_read_elst_table(quicktime_t *file, quicktime_elst_table_t *table)
{
	table->duration = quicktime_read_int32(file);
	table->time = quicktime_read_int32(file);
	table->rate = quicktime_read_fixed32(file);
}

int quicktime_write_elst_table(quicktime_t *file, quicktime_elst_table_t *table, long duration)
{
	table->duration = duration;
	quicktime_write_int32(file, table->duration);
	quicktime_write_int32(file, table->time);
	quicktime_write_fixed32(file, table->rate);
}

int quicktime_elst_table_dump(quicktime_elst_table_t *table)
{
	printf("    edit list table\n");
	printf("     duration %ld\n", table->duration);
	printf("     time %ld\n", table->time);
	printf("     rate %f\n", table->rate);
}

int quicktime_elst_init(quicktime_elst_t *elst)
{
	elst->version = 0;
	elst->flags = 0;
	elst->total_entries = 0;
	elst->table = 0;
}

int quicktime_elst_init_all(quicktime_elst_t *elst)
{
	if(!elst->total_entries)
	{
		elst->total_entries = 1;
		elst->table = (quicktime_elst_table_t*)malloc(sizeof(quicktime_elst_table_t) * elst->total_entries);
		quicktime_elst_table_init(&(elst->table[0]));
	}
}

int quicktime_elst_delete(quicktime_elst_t *elst)
{
	int i;
	if(elst->total_entries)
	{
		for(i = 0; i < elst->total_entries; i++)
			quicktime_elst_table_delete(&(elst->table[i]));
		free(elst->table);
	}
	elst->total_entries = 0;
}

int quicktime_elst_dump(quicktime_elst_t *elst)
{
	int i;
	printf("   edit list (elst)\n");
	printf("    version %d\n", elst->version);
	printf("    flags %d\n", elst->flags);
	printf("    total_entries %d\n", elst->total_entries);

	for(i = 0; i < elst->total_entries; i++)
	{
		quicktime_elst_table_dump(&(elst->table[i]));
	}
}

int quicktime_read_elst(quicktime_t *file, quicktime_elst_t *elst)
{
	int i;
	quicktime_atom_t leaf_atom;

	elst->version = quicktime_read_char(file);
	elst->flags = quicktime_read_int24(file);
	elst->total_entries = quicktime_read_int32(file);
	elst->table = (quicktime_elst_table_t*)malloc(sizeof(quicktime_elst_table_t) * elst->total_entries);
	for(i = 0; i < elst->total_entries; i++)
	{
		quicktime_elst_table_init(&(elst->table[i]));
		quicktime_read_elst_table(file, &(elst->table[i]));
	}
}

int quicktime_write_elst(quicktime_t *file, quicktime_elst_t *elst, long duration)
{
	quicktime_atom_t atom;
	int i;
	quicktime_atom_write_header(file, &atom, "elst");

	quicktime_write_char(file, elst->version);
	quicktime_write_int24(file, elst->flags);
	quicktime_write_int32(file, elst->total_entries);
	for(i = 0; i < elst->total_entries; i++)
	{
		quicktime_write_elst_table(file, elst->table, duration);
	}

	quicktime_atom_write_footer(file, &atom);
}
