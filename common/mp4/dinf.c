#include "quicktime.h"

int quicktime_dinf_init(quicktime_dinf_t *dinf)
{
	quicktime_dref_init(&(dinf->dref));
}

int quicktime_dinf_delete(quicktime_dinf_t *dinf)
{
	quicktime_dref_delete(&(dinf->dref));
}

int quicktime_dinf_init_all(quicktime_dinf_t *dinf)
{
	quicktime_dref_init_all(&(dinf->dref));
}

int quicktime_dinf_dump(quicktime_dinf_t *dinf)
{
	printf("    data information (dinf)\n");
	quicktime_dref_dump(&(dinf->dref));
}

int quicktime_read_dinf(quicktime_t *file, quicktime_dinf_t *dinf, quicktime_atom_t *dinf_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
		if(quicktime_atom_is(&leaf_atom, "dref"))
			{ quicktime_read_dref(file, &(dinf->dref)); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < dinf_atom->end);
}

int quicktime_write_dinf(quicktime_t *file, quicktime_dinf_t *dinf)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "dinf");
	quicktime_write_dref(file, &(dinf->dref));
	quicktime_atom_write_footer(file, &atom);
}
