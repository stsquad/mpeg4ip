#include "quicktime.h"

int quicktime_edts_init(quicktime_edts_t *edts)
{
	quicktime_elst_init(&(edts->elst));
}

int quicktime_edts_delete(quicktime_edts_t *edts)
{
	quicktime_elst_delete(&(edts->elst));
}

int quicktime_edts_init_table(quicktime_edts_t *edts)
{
	quicktime_elst_init_all(&(edts->elst));
}

int quicktime_read_edts(quicktime_t *file, quicktime_edts_t *edts, quicktime_atom_t *edts_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
		if(quicktime_atom_is(&leaf_atom, "elst"))
			{ quicktime_read_elst(file, &(edts->elst)); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < edts_atom->end);
}

int quicktime_edts_dump(quicktime_edts_t *edts)
{
	printf("  edit atom (edts)\n");
	quicktime_elst_dump(&(edts->elst));
}

int quicktime_write_edts(quicktime_t *file, quicktime_edts_t *edts, long duration)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "edts");
	quicktime_write_elst(file, &(edts->elst), duration);
	quicktime_atom_write_footer(file, &atom);
}
