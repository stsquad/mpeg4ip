#include "quicktime.h"


int quicktime_tkhd_init(quicktime_tkhd_t *tkhd)
{
	int i;
	tkhd->version = 0;
	tkhd->flags = 15;
	tkhd->creation_time = quicktime_current_time();
	tkhd->modification_time = quicktime_current_time();
	tkhd->track_id;
	tkhd->reserved1 = 0;
	tkhd->duration = 0;      /* need to set this when closing */
	for(i = 0; i < 8; i++) tkhd->reserved2[i] = 0;
	tkhd->layer = 0;
	tkhd->alternate_group = 0;
	tkhd->volume = 0.996094;
	tkhd->reserved3 = 0;
	quicktime_matrix_init(&(tkhd->matrix));
	tkhd->track_width = 0;
	tkhd->track_height = 0;
	tkhd->is_audio = FALSE;
	tkhd->is_video = FALSE;
	tkhd->is_hint = FALSE;
	return 0;
}

int quicktime_tkhd_init_audio(quicktime_t *file, 
								quicktime_tkhd_t *tkhd)
{
	tkhd->is_audio = TRUE;
}

int quicktime_tkhd_init_video(quicktime_t *file, 
								quicktime_tkhd_t *tkhd, 
								int frame_w, 
								int frame_h)
{
	tkhd->is_video = TRUE;
	tkhd->track_width = frame_w;
	tkhd->track_height = frame_h;
	tkhd->volume = 0;
}

int quicktime_tkhd_init_hint(quicktime_t *file, 
								quicktime_tkhd_t *tkhd)
{
	tkhd->is_hint = TRUE;
	tkhd->flags = 0;
	tkhd->volume = 0;
	tkhd->alternate_group = 1;
}

int quicktime_tkhd_delete(quicktime_tkhd_t *tkhd)
{
	return 0;
}

int quicktime_tkhd_dump(quicktime_tkhd_t *tkhd)
{
	printf("  track header\n");
	printf("   version %d\n", tkhd->version);
	printf("   flags %ld\n", tkhd->flags);
	printf("   creation_time %u\n", tkhd->creation_time);
	printf("   modification_time %u\n", tkhd->modification_time);
	printf("   track_id %d\n", tkhd->track_id);
	printf("   reserved1 %ld\n", tkhd->reserved1);
	printf("   duration %ld\n", tkhd->duration);
	quicktime_print_chars("   reserved2 ", tkhd->reserved2, 8);
	printf("   layer %d\n", tkhd->layer);
	printf("   alternate_group %d\n", tkhd->alternate_group);
	printf("   volume %f\n", tkhd->volume);
	printf("   reserved3 %d\n", tkhd->reserved3);
	quicktime_matrix_dump(&(tkhd->matrix));
	printf("   track_width %f\n", tkhd->track_width);
	printf("   track_height %f\n", tkhd->track_height);
}

int quicktime_read_tkhd(quicktime_t *file, quicktime_tkhd_t *tkhd)
{
	tkhd->version = quicktime_read_char(file);
	tkhd->flags = quicktime_read_int24(file);
	tkhd->creation_time = quicktime_read_int32(file);
	tkhd->modification_time = quicktime_read_int32(file);
	tkhd->track_id = quicktime_read_int32(file);
	tkhd->reserved1 = quicktime_read_int32(file);
	tkhd->duration = quicktime_read_int32(file);
	quicktime_read_data(file, tkhd->reserved2, 8);
	tkhd->layer = quicktime_read_int16(file);
	tkhd->alternate_group = quicktime_read_int16(file);
	tkhd->volume = quicktime_read_fixed16(file);
	tkhd->reserved3 = quicktime_read_int16(file);
	quicktime_read_matrix(file, &(tkhd->matrix));
	tkhd->track_width = quicktime_read_fixed32(file);
	tkhd->track_height = quicktime_read_fixed32(file);
}

int quicktime_write_tkhd(quicktime_t *file, quicktime_tkhd_t *tkhd)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "tkhd");

	quicktime_write_char(file, tkhd->version);
	if (file->use_mp4 && tkhd->flags != 0) {
		quicktime_write_int24(file, 1);
	} else {
		quicktime_write_int24(file, tkhd->flags);
	}
	quicktime_write_int32(file, tkhd->creation_time);
	quicktime_write_int32(file, tkhd->modification_time);
	quicktime_write_int32(file, tkhd->track_id);
	quicktime_write_int32(file, tkhd->reserved1);
	quicktime_write_int32(file, tkhd->duration);

	if (file->use_mp4) {
		int i;
		for (i = 0; i < 3; i++) {
			quicktime_write_int32(file, 0x00000000);
		}
		if (tkhd->is_audio) {
			quicktime_write_int16(file, 0x0100);
		} else {
			quicktime_write_int16(file, 0x0000);
		}
		quicktime_write_int16(file, 0x0000);
		quicktime_write_int32(file, 0x00010000);
		for (i = 0; i < 3; i++) {
			quicktime_write_int32(file, 0x00000000);
		}
		quicktime_write_int32(file, 0x00010000);
		for (i = 0; i < 3; i++) {
			quicktime_write_int32(file, 0x00000000);
		}
		quicktime_write_int32(file, 0x40000000);
		if (tkhd->is_video) {
			quicktime_write_int32(file, 0x01400000);
			quicktime_write_int32(file, 0x00F00000);
		} else {
			quicktime_write_int32(file, 0x00000000);
			quicktime_write_int32(file, 0x00000000);
		}
	} else {
		quicktime_write_data(file, tkhd->reserved2, 8);
		quicktime_write_int16(file, tkhd->layer);
		quicktime_write_int16(file, tkhd->alternate_group);
		quicktime_write_fixed16(file, tkhd->volume);
		quicktime_write_int16(file, tkhd->reserved3);
		quicktime_write_matrix(file, &(tkhd->matrix));
		quicktime_write_fixed32(file, tkhd->track_width);
		quicktime_write_fixed32(file, tkhd->track_height);
	}

	quicktime_atom_write_footer(file, &atom);
}


