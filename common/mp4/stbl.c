#include "quicktime.h"

int quicktime_stbl_init(quicktime_stbl_t *stbl)
{
	stbl->version = 0;
	stbl->flags = 0;
	quicktime_stsd_init(&(stbl->stsd));
	quicktime_stts_init(&(stbl->stts));
	quicktime_stss_init(&(stbl->stss));
	quicktime_stsc_init(&(stbl->stsc));
	quicktime_stsz_init(&(stbl->stsz));
	quicktime_stco_init(&(stbl->stco));
	quicktime_ctts_init(&(stbl->ctts));
}

int quicktime_stbl_init_video(quicktime_t *file, 
							quicktime_stbl_t *stbl, 
							int frame_w,
							int frame_h, 
							int time_scale, 
							float frame_rate,
							char *compressor)
{
	quicktime_stsd_init_video(file, &(stbl->stsd), frame_w, frame_h, frame_rate, compressor);
	quicktime_stts_init_video(file, &(stbl->stts), time_scale, frame_rate);
	quicktime_stss_init_common(file, &(stbl->stss));
	quicktime_stsc_init_video(file, &(stbl->stsc));
	quicktime_stsz_init_video(file, &(stbl->stsz));
	quicktime_stco_init_common(file, &(stbl->stco));
	quicktime_ctts_init_common(file, &(stbl->ctts));
}


int quicktime_stbl_init_audio(quicktime_t *file, 
							quicktime_stbl_t *stbl, 
							int channels, 
							int sample_rate, 
							int bits, 
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	quicktime_stsd_init_audio(file, &(stbl->stsd), channels, sample_rate, bits, compressor);
	quicktime_stts_init_audio(file, &(stbl->stts), time_scale, sample_duration);
	quicktime_stss_init_common(file, &(stbl->stss));
	quicktime_stsc_init_audio(file, &(stbl->stsc));
	quicktime_stsz_init_audio(file, &(stbl->stsz), sample_size);
	quicktime_stco_init_common(file, &(stbl->stco));
	quicktime_ctts_init_common(file, &(stbl->ctts));
}

int quicktime_stbl_init_hint(quicktime_t *file, 
							quicktime_stbl_t *stbl, 
							quicktime_trak_t *refTrak,
							int maxPktSize,
							int timeScale, 
							int sample_duration)
{
	quicktime_stsd_init_hint(file, &(stbl->stsd), maxPktSize, timeScale);
	quicktime_stts_init_hint(file, &(stbl->stts), sample_duration);
	quicktime_stss_init_common(file, &(stbl->stss));
	/* video's versions seems to be what we want, so we reuse them here */
	quicktime_stsc_init_video(file, &(stbl->stsc));
	quicktime_stsz_init_video(file, &(stbl->stsz));
	quicktime_stco_init_common(file, &(stbl->stco));
	quicktime_ctts_init_common(file, &(stbl->ctts));
}

int quicktime_stbl_delete(quicktime_stbl_t *stbl)
{
	quicktime_stsd_delete(&(stbl->stsd));
	quicktime_stts_delete(&(stbl->stts));
	quicktime_stss_delete(&(stbl->stss));
	quicktime_stsc_delete(&(stbl->stsc));
	quicktime_stsz_delete(&(stbl->stsz));
	quicktime_stco_delete(&(stbl->stco));
	quicktime_ctts_delete(&(stbl->ctts));
}

int quicktime_stbl_dump(void *minf_ptr, quicktime_stbl_t *stbl)
{
	printf("    sample table\n");
	quicktime_stsd_dump(minf_ptr, &(stbl->stsd));
	quicktime_stts_dump(&(stbl->stts));
	quicktime_stss_dump(&(stbl->stss));
	quicktime_stsc_dump(&(stbl->stsc));
	quicktime_stsz_dump(&(stbl->stsz));
	quicktime_stco_dump(&(stbl->stco));
	quicktime_ctts_dump(&(stbl->ctts));
}

int quicktime_read_stbl(quicktime_t *file, quicktime_minf_t *minf, quicktime_stbl_t *stbl, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "stsd"))
			{ 
				quicktime_read_stsd(file, minf, &(stbl->stsd)); 
/* Some codecs store extra information at the end of this */
				quicktime_atom_skip(file, &leaf_atom);
			}
		else
		if(quicktime_atom_is(&leaf_atom, "stts"))
			{ quicktime_read_stts(file, &(stbl->stts)); }
		else
		if(quicktime_atom_is(&leaf_atom, "stss"))
			{ quicktime_read_stss(file, &(stbl->stss)); }
		else
		if(quicktime_atom_is(&leaf_atom, "stsc"))
			{ quicktime_read_stsc(file, &(stbl->stsc)); }
		else
		if(quicktime_atom_is(&leaf_atom, "stsz"))
			{ quicktime_read_stsz(file, &(stbl->stsz)); }
		else
		if(quicktime_atom_is(&leaf_atom, "stco"))
			{ quicktime_read_stco(file, &(stbl->stco)); }
		else
		if(quicktime_atom_is(&leaf_atom, "ctts"))
			{ quicktime_read_ctts(file, &(stbl->ctts)); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);

	return 0;
}

int quicktime_write_stbl(quicktime_t *file, quicktime_minf_t *minf, quicktime_stbl_t *stbl)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "stbl");

	quicktime_write_stsd(file, minf, &(stbl->stsd));
	quicktime_write_stts(file, &(stbl->stts));
	quicktime_write_stss(file, &(stbl->stss));
	quicktime_write_stsc(file, &(stbl->stsc));
	quicktime_write_stsz(file, &(stbl->stsz));
	quicktime_write_stco(file, &(stbl->stco));
	quicktime_write_ctts(file, &(stbl->ctts));

	quicktime_atom_write_footer(file, &atom);
}


