#ifndef FUNCPROTOS_H
#define FUNCPROTOS_H

/* atom handling routines */
long quicktime_atom_read_size(char *data);
u_int64_t quicktime_atom_read_size64(char *data);

quicktime_trak_t* quicktime_add_track(quicktime_moov_t *moov);
quicktime_trak_t* quicktime_find_track_by_id(quicktime_moov_t *moov, int trackId);

/* initializers for every atom */
int quicktime_matrix_init(quicktime_matrix_t *matrix);
int quicktime_edts_init_table(quicktime_edts_t *edts);
int quicktime_edts_init(quicktime_edts_t *edts);
int quicktime_elst_init(quicktime_elst_t *elst);
int quicktime_elst_init_all(quicktime_elst_t *elst);
int quicktime_elst_table_init(quicktime_elst_table_t *table); /* initialize a table */
int quicktime_tkhd_init(quicktime_tkhd_t *tkhd);
int quicktime_tkhd_init_video(quicktime_t *file, quicktime_tkhd_t *tkhd, int frame_w, int frame_h);
int quicktime_ctab_init(quicktime_ctab_t *ctab);
int quicktime_mjqt_init(quicktime_mjqt_t *mjqt);
int quicktime_mjht_init(quicktime_mjht_t *mjht);
int quicktime_stsd_table_init(quicktime_stsd_table_t *table);
int quicktime_stsd_init(quicktime_stsd_t *stsd);
int quicktime_stsd_init_table(quicktime_stsd_t *stsd);
int quicktime_stsd_init_video(quicktime_t *file, quicktime_stsd_t *stsd, int frame_w, int frame_h, float frame_rate, char *compression);
int quicktime_stsd_init_audio(quicktime_t *file, quicktime_stsd_t *stsd, int channels, int sample_rate, int bits, char *compressor);
int quicktime_stts_init(quicktime_stts_t *stts);
int quicktime_stts_init_table(quicktime_stts_t *stts);
int quicktime_stts_init_video(quicktime_t *file, quicktime_stts_t *stts, int time_scale, float frame_rate);
int quicktime_stts_init_audio(quicktime_t *file, quicktime_stts_t *stts, int time_scale, int sample_duration);
int quicktime_stts_init_hint(quicktime_t *file, quicktime_stts_t *stts, int sample_duration);
int quicktime_stss_init(quicktime_stss_t *stss);
int quicktime_stss_init_common(quicktime_t *file, quicktime_stss_t *stss);
int quicktime_stsc_init(quicktime_stsc_t *stsc);
int quicktime_stsc_init_video(quicktime_t *file, quicktime_stsc_t *stsc);
int quicktime_stsc_init_audio(quicktime_t *file, quicktime_stsc_t *stsc);
int quicktime_stsz_init(quicktime_stsz_t *stsz);
int quicktime_stsz_init_video(quicktime_t *file, quicktime_stsz_t *stsz);
int quicktime_stsz_init_audio(quicktime_t *file, quicktime_stsz_t *stsz, int sample_size);
int quicktime_stco_init(quicktime_stco_t *stco);
int quicktime_stco_init_common(quicktime_t *file, quicktime_stco_t *stco);
int quicktime_stbl_init(quicktime_stbl_t *tkhd);
int quicktime_stbl_init_video(quicktime_t *file, quicktime_stbl_t *stbl, int frame_w, int frame_h, int time_scale, float frame_rate, char *compressor);
int quicktime_stbl_init_audio(quicktime_t *file, quicktime_stbl_t *stbl, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int quicktime_stbl_init_hint(quicktime_t *file, quicktime_stbl_t *stbl, quicktime_trak_t *refTrak, int maxPktSize, int timeScale, int sampleDuration);
int quicktime_vmhd_init(quicktime_vmhd_t *vmhd);
int quicktime_vmhd_init_video(quicktime_t *file, quicktime_vmhd_t *vmhd, int frame_w, int frame_h, float frame_rate);
int quicktime_smhd_init(quicktime_smhd_t *smhd);
int quicktime_dref_table_init(quicktime_dref_table_t *table);
int quicktime_dref_init_all(quicktime_dref_t *dref);
int quicktime_dref_init(quicktime_dref_t *dref);
int quicktime_dinf_init_all(quicktime_dinf_t *dinf);
int quicktime_dinf_init(quicktime_dinf_t *dinf);
int quicktime_minf_init(quicktime_minf_t *minf);
int quicktime_minf_init_video(quicktime_t *file, quicktime_minf_t *minf, int frame_w, int frame_h, int time_scale, float frame_rate, char *compressor);
int quicktime_minf_init_audio(quicktime_t *file, quicktime_minf_t *minf, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int quicktime_minf_init_hint(quicktime_t *file, quicktime_minf_t *minf, quicktime_trak_t *refTrak, int maxPktSize, int timeScale, int sampleDuration);
int quicktime_mdhd_init(quicktime_mdhd_t *mdhd);
int quicktime_mdhd_init_video(quicktime_t *file, quicktime_mdhd_t *mdhd, int time_scale);
int quicktime_mdhd_init_audio(quicktime_t *file, quicktime_mdhd_t *mdhd, int time_scale);
int quicktime_mdia_init(quicktime_mdia_t *mdia);
int quicktime_mdia_init_video(quicktime_t *file, quicktime_mdia_t *mdia, int frame_w, int frame_h, float frame_rate, int time_scale, char *compressor);
int quicktime_mdia_init_audio(quicktime_t *file, quicktime_mdia_t *mdia, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int quicktime_mdia_init_hint(quicktime_t *file, quicktime_mdia_t *mdia, quicktime_trak_t *refTrak, int maxPktSize, int time_scale, int sampleDuration);
int quicktime_trak_init(quicktime_trak_t *trak);
int quicktime_trak_init_video(quicktime_t *file, quicktime_trak_t *trak, int frame_w, int frame_h, float frame_rate, int time_scale, char *compressor);
int quicktime_trak_init_audio(quicktime_t *file, quicktime_trak_t *trak, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int quicktime_trak_init_hint(quicktime_t *file, quicktime_trak_t *trak, quicktime_trak_t *refTrak, int maxPktSize, int time_scale, int sample_duration);
int quicktime_tref_init(quicktime_tref_t *tref);
int quicktime_tref_init_hint(quicktime_tref_t *tref, quicktime_trak_t *refTrak);
int quicktime_udta_init(quicktime_udta_t *udta);
int quicktime_mvhd_init(quicktime_mvhd_t *mvhd);
int quicktime_moov_init(quicktime_moov_t *moov);
int quicktime_mdat_init(quicktime_mdat_t *mdat);
int quicktime_init(quicktime_t *file);
int quicktime_hdlr_init(quicktime_hdlr_t *hdlr);
int quicktime_hdlr_init_video(quicktime_hdlr_t *hdlr);
int quicktime_hdlr_init_audio(quicktime_hdlr_t *hdlr);
int quicktime_hdlr_init_data(quicktime_hdlr_t *hdlr);

/* utilities for reading data types */
int quicktime_read_data(quicktime_t *file, char *data, int size);
int quicktime_write_data(quicktime_t *file, char *data, int size);
int quicktime_read_pascal(quicktime_t *file, char *data);
int quicktime_write_pascal(quicktime_t *file, char *data);
float quicktime_read_fixed32(quicktime_t *file);
int quicktime_write_fixed32(quicktime_t *file, float number);
float quicktime_read_fixed16(quicktime_t *file);
int quicktime_write_fixed16(quicktime_t *file, float number);
u_int64_t quicktime_read_int64(quicktime_t *file);
int quicktime_write_int64(quicktime_t *file, u_int64_t number);
long quicktime_read_int32(quicktime_t *file);
int quicktime_write_int32(quicktime_t *file, long number);
long quicktime_read_int24(quicktime_t *file);
int quicktime_write_int24(quicktime_t *file, long number);
int quicktime_read_int16(quicktime_t *file);
int quicktime_write_int16(quicktime_t *file, int number);
int quicktime_read_char(quicktime_t *file);
int quicktime_write_char(quicktime_t *file, char x);
int quicktime_read_char32(quicktime_t *file, char *string);
int quicktime_write_char32(quicktime_t *file, char *string);
int quicktime_copy_char32(char *output, char *input);
long quicktime_position(quicktime_t *file);
int quicktime_read_mp4_descr_length(quicktime_t *file);
int quicktime_write_mp4_descr_length(quicktime_t *file, int length, bool compact);

/* Most codecs don't specify the actual number of bits on disk in the stbl. */
/* Convert the samples to the number of bytes for reading depending on the codec. */
long quicktime_samples_to_bytes(quicktime_trak_t *track, long samples);


/* chunks always start on 1 */
/* samples start on 0 */

/* queries for every atom */
/* the starting sample in the given chunk */
long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk);

/* number of samples in the chunk */
long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk);

/* the byte offset from mdat start of the chunk */
long quicktime_chunk_to_offset(quicktime_trak_t *trak, long chunk);

/* the chunk of any offset from mdat start */
long quicktime_offset_to_chunk(long *chunk_offset, quicktime_trak_t *trak, long offset);

/* the total number of samples in the track depending on the access mode */
long quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak);

/* total bytes between the two samples */
long quicktime_sample_range_size(quicktime_trak_t *trak, long chunk_sample, long sample);

/* update the position pointers in all the tracks after a set_position */
int quicktime_update_positions(quicktime_t *file);

/* converting between mdat offsets to samples */
long quicktime_sample_to_offset(quicktime_trak_t *trak, long sample);
long quicktime_offset_to_sample(quicktime_trak_t *trak, long offset);

quicktime_trak_t* quicktime_add_trak(quicktime_moov_t *moov);
int quicktime_delete_trak(quicktime_moov_t *moov, quicktime_trak_t *trak);
int quicktime_get_timescale(float frame_rate);

/* update all the tables after writing a buffer */
/* set sample_size to 0 if no sample size should be set */
int quicktime_update_tables(quicktime_t *file, 
							quicktime_trak_t *trak, 
							long offset, 
							long chunk, 
							long sample, 
							long samples, 
							long sample_size,
							long sample_duration,
							u_char isSyncSample,
							long renderingOffset);
unsigned long quicktime_current_time();

#endif
