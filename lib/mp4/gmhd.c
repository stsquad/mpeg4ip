#include "quicktime.h"




int quicktime_gmhd_init(quicktime_gmhd_t *gmhd)
{
	quicktime_gmin_init(&(gmhd->gmin));
}

int quicktime_gmhd_delete(quicktime_gmhd_t *gmhd)
{
	quicktime_gmin_delete(&(gmhd->gmin));
}

int quicktime_gmhd_dump(quicktime_gmhd_t *gmhd)
{
	printf("    generic media header\n");
	quicktime_gmin_dump(&gmhd->gmin);
}

int quicktime_read_gmhd(quicktime_t *file, quicktime_gmhd_t *gmhd, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	do {
		quicktime_atom_read_header(file, &leaf_atom);

		if (quicktime_atom_is(&leaf_atom, "gmin")) {
			quicktime_read_gmin(file, &(gmhd->gmin));
		} else {
			quicktime_atom_skip(file, &leaf_atom);
		}
	} while (quicktime_position(file) < parent_atom->end);
}

int quicktime_write_gmhd(quicktime_t *file, quicktime_gmhd_t *gmhd)
{
	
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "gmhd");

	quicktime_write_gmin(file, &(gmhd->gmin));

	quicktime_atom_write_footer(file, &atom);
}

