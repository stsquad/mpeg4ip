#include "quicktime.h"



int quicktime_minf_init(quicktime_minf_t *minf)
{
	minf->is_video = minf->is_audio = minf->is_hint = 0;
	quicktime_vmhd_init(&(minf->vmhd));
	quicktime_smhd_init(&(minf->smhd));
	quicktime_hdlr_init(&(minf->hdlr));
	quicktime_dinf_init(&(minf->dinf));
	quicktime_stbl_init(&(minf->stbl));
}

int quicktime_minf_init_video(quicktime_t *file, 
								quicktime_minf_t *minf, 
								int frame_w,
								int frame_h, 
								int time_scale, 
								float frame_rate,
								char *compressor)
{
	minf->is_video = 1;
	quicktime_vmhd_init_video(file, &(minf->vmhd), frame_w, frame_h, frame_rate);
	quicktime_stbl_init_video(file, &(minf->stbl), frame_w, frame_h, time_scale, frame_rate, compressor);
	quicktime_hdlr_init_data(&(minf->hdlr));
	quicktime_dinf_init_all(&(minf->dinf));
}

int quicktime_minf_init_audio(quicktime_t *file, 
							quicktime_minf_t *minf, 
							int channels, 
							int sample_rate, 
							int bits,
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	minf->is_audio = 1;
/* smhd doesn't store anything worth initializing */
	quicktime_stbl_init_audio(file, &(minf->stbl), channels, sample_rate, bits, sample_size, time_scale, sample_duration, compressor);
	quicktime_hdlr_init_data(&(minf->hdlr));
	quicktime_dinf_init_all(&(minf->dinf));
}

int quicktime_minf_init_hint(quicktime_t *file, 
								quicktime_minf_t *minf, 
								quicktime_trak_t *refTrak,
								int maxPktSize,
								int timeScale, int sampleDuration)
{
	minf->is_hint = 1;
	quicktime_gmhd_init(&(minf->gmhd));
	quicktime_hmhd_init(&(minf->hmhd));
	quicktime_stbl_init_hint(file, &(minf->stbl), refTrak, 
		maxPktSize, timeScale, sampleDuration);
	quicktime_hdlr_init_data(&(minf->hdlr));
	quicktime_dinf_init_all(&(minf->dinf));
}

int quicktime_minf_delete(quicktime_minf_t *minf)
{
	quicktime_vmhd_delete(&(minf->vmhd));
	quicktime_smhd_delete(&(minf->smhd));
	quicktime_gmhd_delete(&(minf->gmhd));
	quicktime_hmhd_delete(&(minf->hmhd));
	quicktime_dinf_delete(&(minf->dinf));
	quicktime_stbl_delete(&(minf->stbl));
	quicktime_hdlr_delete(&(minf->hdlr));
}

int quicktime_minf_dump(quicktime_minf_t *minf)
{
	printf("   media info\n");
	printf("    is_audio %d\n", minf->is_audio);
	printf("    is_video %d\n", minf->is_video);
	printf("    is_hint %d\n", minf->is_hint);
	if(minf->is_audio) quicktime_smhd_dump(&(minf->smhd));
	if(minf->is_video) quicktime_vmhd_dump(&(minf->vmhd));
	if(minf->is_hint) {
		/* don't know which format we're using so just dump both */
		quicktime_hmhd_dump(&(minf->hmhd));
		quicktime_gmhd_dump(&(minf->gmhd));
	}
	quicktime_hdlr_dump(&(minf->hdlr));
	quicktime_dinf_dump(&(minf->dinf));
	quicktime_stbl_dump(minf, &(minf->stbl));
}

int quicktime_read_minf(quicktime_t *file, quicktime_minf_t *minf, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
	long pos = quicktime_position(file);

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "vmhd"))
			{ minf->is_video = 1; quicktime_read_vmhd(file, &(minf->vmhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "smhd"))
			{ minf->is_audio = 1; quicktime_read_smhd(file, &(minf->smhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "gmhd"))
			{ minf->is_hint = 1; quicktime_read_gmhd(file, &(minf->gmhd), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "hmhd"))
			{ minf->is_hint = 1; quicktime_read_hmhd(file, &(minf->hmhd), &leaf_atom); }
		else
		if(quicktime_atom_is(&leaf_atom, "hdlr"))
			{ 
				quicktime_read_hdlr(file, &(minf->hdlr)); 
/* Main Actor doesn't write component name */
				quicktime_atom_skip(file, &leaf_atom);
			}
		else
		if(quicktime_atom_is(&leaf_atom, "dinf"))
			{ quicktime_read_dinf(file, &(minf->dinf), &leaf_atom); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);

	quicktime_set_position(file, pos);

	do {
		quicktime_atom_read_header(file, &leaf_atom);

		if(quicktime_atom_is(&leaf_atom, "stbl")) {
			quicktime_read_stbl(file, minf, &(minf->stbl), &leaf_atom);
		} else {
			quicktime_atom_skip(file, &leaf_atom);
		}
	} while(quicktime_position(file) < parent_atom->end);

	return 0;
}

int quicktime_write_minf(quicktime_t *file, quicktime_minf_t *minf)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "minf");

	if(minf->is_video) quicktime_write_vmhd(file, &(minf->vmhd));
	if(minf->is_audio) quicktime_write_smhd(file, &(minf->smhd));
	if(minf->is_hint) {
		if (file->use_mp4) {
			quicktime_write_hmhd(file, &(minf->hmhd));
		} else {
			quicktime_write_gmhd(file, &(minf->gmhd));
		}
	}
	quicktime_write_hdlr(file, &(minf->hdlr));
	quicktime_write_dinf(file, &(minf->dinf));
	quicktime_write_stbl(file, minf, &(minf->stbl));

	quicktime_atom_write_footer(file, &atom);
}
