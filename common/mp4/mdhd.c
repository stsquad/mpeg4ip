#include "quicktime.h"

int quicktime_mdhd_init(quicktime_mdhd_t *mdhd)
{
	mdhd->version = 0;
	mdhd->flags = 0;
	mdhd->creation_time = quicktime_current_time();
	mdhd->modification_time = quicktime_current_time();
	mdhd->time_scale = 0;
	mdhd->duration = 0;
	mdhd->language = 0;
	mdhd->quality = 0;
}

int quicktime_mdhd_init_video(quicktime_t *file, 
							quicktime_mdhd_t *mdhd,
							int time_scale)
{
	mdhd->time_scale = time_scale;
	mdhd->duration = 0;      /* set this when closing */
}

int quicktime_mdhd_init_audio(quicktime_t *file, 
							quicktime_mdhd_t *mdhd, 
							int time_scale)
{
	mdhd->time_scale = time_scale;
	mdhd->duration = 0;      /* set this when closing */
}

int quicktime_mdhd_init_hint(quicktime_t *file, 
							quicktime_mdhd_t *mdhd, 
							quicktime_trak_t *refTrak,
							int time_scale)
{
	mdhd->duration = refTrak->mdia.mdhd.duration;
	mdhd->time_scale = time_scale;
	mdhd->quality = refTrak->mdia.mdhd.quality;
	mdhd->language = refTrak->mdia.mdhd.language;
}

quicktime_mdhd_delete(quicktime_mdhd_t *mdhd)
{
}

int quicktime_read_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd)
{
	mdhd->version = quicktime_read_char(file);
	mdhd->flags = quicktime_read_int24(file);
	mdhd->creation_time = quicktime_read_int32(file);
	mdhd->modification_time = quicktime_read_int32(file);
	mdhd->time_scale = quicktime_read_int32(file);
	mdhd->duration = quicktime_read_int32(file);
	mdhd->language = quicktime_read_int16(file);
	mdhd->quality = quicktime_read_int16(file);
}

int quicktime_mdhd_dump(quicktime_mdhd_t *mdhd)
{
	printf("   media header\n");
	printf("    version %d\n", mdhd->version);
	printf("    flags %d\n", mdhd->flags);
	printf("    creation_time %u\n", mdhd->creation_time);
	printf("    modification_time %u\n", mdhd->modification_time);
	printf("    time_scale %d\n", mdhd->time_scale);
	printf("    duration %d\n", mdhd->duration);
	printf("    language %d\n", mdhd->language);
	printf("    quality %d\n", mdhd->quality);
}

int quicktime_write_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "mdhd");

	quicktime_write_char(file, mdhd->version);
	quicktime_write_int24(file, mdhd->flags);
	quicktime_write_int32(file, mdhd->creation_time);
	quicktime_write_int32(file, mdhd->modification_time);
	quicktime_write_int32(file, mdhd->time_scale);
	quicktime_write_int32(file, mdhd->duration);
	quicktime_write_int16(file, mdhd->language);
	if (file->use_mp4) {
		quicktime_write_int16(file, 0x0000);	
	} else {
		quicktime_write_int16(file, mdhd->quality);	
	}

	quicktime_atom_write_footer(file, &atom);
}

