#include "quicktime.h"


int quicktime_stsd_init(quicktime_stsd_t *stsd)
{
	stsd->version = 0;
	stsd->flags = 0;
	stsd->total_entries = 0;
}

int quicktime_stsd_init_table(quicktime_stsd_t *stsd)
{
	if(!stsd->total_entries)
	{
		stsd->total_entries = 1;
		stsd->table = (quicktime_stsd_table_t*)malloc(sizeof(quicktime_stsd_table_t) * stsd->total_entries);
		quicktime_stsd_table_init(&(stsd->table[0]));
	}
}

int quicktime_stsd_init_video(quicktime_t *file, 
								quicktime_stsd_t *stsd, 
								int frame_w,
								int frame_h, 
								float frame_rate,
								char *compression)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
	table = &(stsd->table[0]);

	quicktime_copy_char32(table->format, compression);
	table->width = frame_w;
	table->height = frame_h;
	table->frames_per_sample = 1;
	table->depth = 24;
	table->ctab_id = 65535;
}

int quicktime_stsd_init_audio(quicktime_t *file, 
							quicktime_stsd_t *stsd, 
							int channels,
							int sample_rate, 
							int bits, 
							char *compressor)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
	table = &(stsd->table[0]);

	quicktime_copy_char32(table->format, compressor);
	table->channels = channels;
	table->sample_size = bits;
	table->sample_rate = sample_rate;
}

int quicktime_stsd_init_hint(quicktime_t *file, 
							quicktime_stsd_t *stsd, 
							int maxPktSize,
							int timeScale)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
	table = &(stsd->table[0]);

	quicktime_copy_char32(table->format, "rtp ");
	table->version = 1;
	table->revision = 1;
	table->maxPktSize = maxPktSize;
	table->tims.timeScale = timeScale;
}

int quicktime_stsd_delete(quicktime_stsd_t *stsd)
{
	int i;
	if(stsd->total_entries)
	{
		for(i = 0; i < stsd->total_entries; i++)
			quicktime_stsd_table_delete(&(stsd->table[i]));
		free(stsd->table);
	}

	stsd->total_entries = 0;
}

int quicktime_stsd_dump(void *minf_ptr, quicktime_stsd_t *stsd)
{
	int i;
	printf("     sample description\n");
	printf("      version %d\n", stsd->version);
	printf("      flags %d\n", stsd->flags);
	printf("      total_entries %d\n", stsd->total_entries);
	
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_stsd_table_dump(minf_ptr, &(stsd->table[i]));
	}
}

int quicktime_read_stsd(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_t *stsd)
{
	int i;
	quicktime_atom_t leaf_atom;

	stsd->version = quicktime_read_char(file);
	stsd->flags = quicktime_read_int24(file);
	stsd->total_entries = quicktime_read_int32(file);
	stsd->table = (quicktime_stsd_table_t*)malloc(sizeof(quicktime_stsd_table_t) * stsd->total_entries);
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_stsd_table_init(&(stsd->table[i]));
		quicktime_read_stsd_table(file, minf, &(stsd->table[i]));
	}
}

int quicktime_write_stsd(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_t *stsd)
{
	quicktime_atom_t atom;
	int i;
	quicktime_atom_write_header(file, &atom, "stsd");

	quicktime_write_char(file, stsd->version);
	quicktime_write_int24(file, stsd->flags);
	quicktime_write_int32(file, stsd->total_entries);
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_write_stsd_table(file, minf, stsd->table);
	}

	quicktime_atom_write_footer(file, &atom);
}



