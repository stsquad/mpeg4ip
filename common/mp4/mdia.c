#include "quicktime.h"


int quicktime_mdia_init(quicktime_mdia_t *mdia)
{
	quicktime_mdhd_init(&(mdia->mdhd));
	quicktime_hdlr_init(&(mdia->hdlr));
	quicktime_minf_init(&(mdia->minf));
}

int quicktime_mdia_init_video(quicktime_t *file, 
								quicktime_mdia_t *mdia,
								int frame_w,
								int frame_h, 
								float frame_rate,
								int time_scale,
								char *compressor)
{
	quicktime_mdhd_init_video(file, &(mdia->mdhd), time_scale);
	quicktime_minf_init_video(file, &(mdia->minf), frame_w, frame_h, mdia->mdhd.time_scale, frame_rate, compressor);
	quicktime_hdlr_init_video(&(mdia->hdlr));
}

int quicktime_mdia_init_audio(quicktime_t *file, 
							quicktime_mdia_t *mdia, 
							int channels,
							int sample_rate, 
							int bits, 
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	quicktime_mdhd_init_audio(file, &(mdia->mdhd), time_scale);
	quicktime_minf_init_audio(file, &(mdia->minf), channels, sample_rate, bits, sample_size, time_scale, sample_duration, compressor);
	quicktime_hdlr_init_audio(&(mdia->hdlr));
}

int quicktime_mdia_init_hint(quicktime_t *file, 
							quicktime_mdia_t *mdia, 
							quicktime_trak_t *refTrak,
							int maxPktSize,
							int time_scale, 
							int sampleDuration)
{
	quicktime_mdhd_init_hint(file, &(mdia->mdhd), refTrak, time_scale);
	quicktime_minf_init_hint(file, &(mdia->minf), refTrak, 
		maxPktSize, time_scale, sampleDuration);
	quicktime_hdlr_init_hint(&(mdia->hdlr));
}

int quicktime_mdia_delete(quicktime_mdia_t *mdia)
{
	quicktime_mdhd_delete(&(mdia->mdhd));
	quicktime_hdlr_delete(&(mdia->hdlr));
	quicktime_minf_delete(&(mdia->minf));
}

int quicktime_mdia_dump(quicktime_mdia_t *mdia)
{
	printf("  media\n");
	quicktime_mdhd_dump(&(mdia->mdhd));
	quicktime_hdlr_dump(&(mdia->hdlr));
	quicktime_minf_dump(&(mdia->minf));
}

int quicktime_read_mdia(quicktime_t *file, quicktime_mdia_t *mdia, quicktime_atom_t *trak_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "mdhd"))
			{ quicktime_read_mdhd(file, &(mdia->mdhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "hdlr"))
			{
				quicktime_read_hdlr(file, &(mdia->hdlr)); 
/* Main Actor doesn't write component name */
				quicktime_atom_skip(file, &leaf_atom);
/*printf("quicktime_read_mdia %ld\n", quicktime_position(file)); */
			}
		else
		if(quicktime_atom_is(&leaf_atom, "minf"))
			{ quicktime_read_minf(file, &(mdia->minf), &leaf_atom); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < trak_atom->end);

	return 0;
}

int quicktime_write_mdia(quicktime_t *file, quicktime_mdia_t *mdia)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "mdia");

	quicktime_write_mdhd(file, &(mdia->mdhd));
	quicktime_write_hdlr(file, &(mdia->hdlr));
	quicktime_write_minf(file, &(mdia->minf));

	quicktime_atom_write_footer(file, &atom);
}
