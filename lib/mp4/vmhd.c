#include "quicktime.h"


int quicktime_vmhd_init(quicktime_vmhd_t *vmhd)
{
	vmhd->version = 0;
	vmhd->flags = 1;
	vmhd->graphics_mode = 64;
	vmhd->opcolor[0] = 32768;
	vmhd->opcolor[1] = 32768;
	vmhd->opcolor[2] = 32768;
}

int quicktime_vmhd_init_video(quicktime_t *file, 
								quicktime_vmhd_t *vmhd, 
								int frame_w,
								int frame_h, 
								float frame_rate)
{
}

int quicktime_vmhd_delete(quicktime_vmhd_t *vmhd)
{
}

int quicktime_vmhd_dump(quicktime_vmhd_t *vmhd)
{
	printf("    video media header\n");
	printf("     version %d\n", vmhd->version);
	printf("     flags %d\n", vmhd->flags);
	printf("     graphics_mode %d\n", vmhd->graphics_mode);
	printf("     opcolor %d %d %d\n", vmhd->opcolor[0], vmhd->opcolor[1], vmhd->opcolor[2]);
}

int quicktime_read_vmhd(quicktime_t *file, quicktime_vmhd_t *vmhd)
{
	int i;
	vmhd->version = quicktime_read_char(file);
	vmhd->flags = quicktime_read_int24(file);
	vmhd->graphics_mode = quicktime_read_int16(file);
	for(i = 0; i < 3; i++)
		vmhd->opcolor[i] = quicktime_read_int16(file);
}

int quicktime_write_vmhd(quicktime_t *file, quicktime_vmhd_t *vmhd)
{
	quicktime_atom_t atom;
	int i;
	quicktime_atom_write_header(file, &atom, "vmhd");

	quicktime_write_char(file, vmhd->version);
	quicktime_write_int24(file, vmhd->flags);

	if (file->use_mp4) {
		quicktime_write_int64(file, (u_int64_t)0);
	} else {
		quicktime_write_int16(file, vmhd->graphics_mode);
		for(i = 0; i < 3; i++) {
			quicktime_write_int16(file, vmhd->opcolor[i]);
		}
	}

	quicktime_atom_write_footer(file, &atom);
}

