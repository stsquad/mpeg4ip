#include "quicktime.h"


int quicktime_make_streamable(char *in_path, char *out_path)
{
	quicktime_t file, *old_file, new_file;
	int moov_exists = 0, mdat_exists = 0, result, atoms = 1;
	long mdat_start, mdat_size;
	quicktime_atom_t leaf_atom;
	long moov_length;
	
	quicktime_init(&file);

/* find the moov atom in the old file */
	
	if(!(file.stream = fopen(in_path, "rb")))
	{
		perror("quicktime_make_streamable");
		return 1;
	}

	fseek(file.stream, 0, SEEK_END);
	file.total_length = ftell(file.stream);
	fseek(file.stream, 0, SEEK_SET);

/* get the locations of moov and mdat atoms */
	do
	{
/*printf("%x\n", quicktime_position(&file)); */
		result = quicktime_atom_read_header(&file, &leaf_atom);

		if(!result)
		{
			if(quicktime_atom_is(&leaf_atom, "moov"))
			{
				moov_exists = atoms;
				moov_length = leaf_atom.size;
			}
			else
			if(quicktime_atom_is(&leaf_atom, "mdat"))
			{
				mdat_start = quicktime_position(&file) - HEADER_LENGTH;
				mdat_size = leaf_atom.size;
				mdat_exists = atoms;
			}

			quicktime_atom_skip(&file, &leaf_atom);

			atoms++;
		}
	}while(!result && quicktime_position(&file) < file.total_length);

	fclose(file.stream);

	if(!moov_exists)
	{
		printf("quicktime_make_streamable: no moov atom\n");
		return 1;
	}

	if(!mdat_exists)
	{
		printf("quicktime_make_streamable: no mdat atom\n");
		return 1;
	}

/* copy the old file to the new file */
	if(moov_exists && mdat_exists)
	{
/* moov wasn't the first atom */
		if(moov_exists > 1)
		{
			char *buffer;
			long buf_size = 1000000;

			result = 0;

/* read the header proper */
			if(!(old_file = quicktime_open(in_path, 1, 0, 0)))
			{
				return 1;
			}

			quicktime_shift_offsets(&(old_file->moov), moov_length);

/* open the output file */
			if(!(new_file.stream = fopen(out_path, "wb")))
			{
				perror("quicktime_make_streamable");
				result =  1;
			}
			else
			{
/* set up some flags */
				new_file.wr = 1;
				new_file.rd = 0;
				quicktime_write_moov(&new_file, &(old_file->moov));

				quicktime_set_position(old_file, mdat_start);

				if(!(buffer = calloc(1, buf_size)))
				{
					result = 1;
					printf("quicktime_make_streamable: out of memory\n");
				}
				else
				{
					while(quicktime_position(old_file) < mdat_start + mdat_size && !result)
					{
						if(quicktime_position(old_file) + buf_size > mdat_start + mdat_size)
							buf_size = mdat_start + mdat_size - quicktime_position(old_file);

						if(!quicktime_read_data(old_file, buffer, buf_size)) result = 1;
						if(!result)
						{
							if(!quicktime_write_data(&new_file, buffer, buf_size)) result = 1;
						}
					}
					free(buffer);
				}
				fclose(new_file.stream);
			}
			quicktime_close(old_file);
		}
		else
		{
			printf("quicktime_make_streamable: header already at 0 offset\n");
			return 0;
		}
	}
	
	return 0;
}

int quicktime_set_time_scale(quicktime_t *file, int time_scale)
{
	file->moov.mvhd.time_scale = time_scale;
}

int quicktime_set_copyright(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.copyright), &(file->moov.udta.copyright_len), string);
}

int quicktime_set_name(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.name), &(file->moov.udta.name_len), string);
}

int quicktime_set_info(quicktime_t *file, char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.info), &(file->moov.udta.info_len), string);
}

int quicktime_get_time_scale(quicktime_t *file)
{
	return file->moov.mvhd.time_scale;
}

char* quicktime_get_copyright(quicktime_t *file)
{
	return file->moov.udta.copyright;
}

char* quicktime_get_name(quicktime_t *file)
{
	return file->moov.udta.name;
}

char* quicktime_get_info(quicktime_t *file)
{
	return file->moov.udta.info;
}

int quicktime_get_iod_audio_profile_level(quicktime_t *file)
{
	return file->moov.iods.audioProfileId;
}

int quicktime_set_iod_audio_profile_level(quicktime_t *file, int id)
{
	quicktime_iods_set_audio_profile(&file->moov.iods, id);
}

int quicktime_get_iod_video_profile_level(quicktime_t *file)
{
	return file->moov.iods.videoProfileId;
}

int quicktime_set_iod_video_profile_level(quicktime_t *file, int id)
{
	quicktime_iods_set_video_profile(&file->moov.iods, id);
}

int quicktime_video_tracks(quicktime_t *file)
{
	int i, result = 0;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		if(file->moov.trak[i]->mdia.minf.is_video) result++;
	}
	return result;
}

int quicktime_audio_tracks(quicktime_t *file)
{
	int i, result = 0;
	quicktime_minf_t *minf;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		minf = &(file->moov.trak[i]->mdia.minf);
		if(minf->is_audio)
			result++;
	}
	return result;
}

int quicktime_set_audio(quicktime_t *file, 
						int channels,
						long sample_rate,
						int bits,
						int sample_size,
						int time_scale,
						int sample_duration,	
						char *compressor)
{
	int i, j;
	quicktime_trak_t *trak;

	/* delete any existing tracks */
	for(i = 0; i < file->total_atracks; i++) {
		for (j = 0; j < file->atracks[i].totalHintTracks; j++) {
			quicktime_delete_trak(&(file->moov), 
				file->atracks[i].hintTracks[j]);
			free(file->atracks[i].hintTracks[j]);
			file->atracks[i].hintTracks[j] = NULL;
			file->total_hint_tracks--;
		}
		quicktime_delete_audio_map(&(file->atracks[i]));
		quicktime_delete_trak(&(file->moov), file->atracks[i].track);
	}
	free(file->atracks);
	file->atracks = NULL;	
	file->total_atracks = 0;

	if(channels) {
		/* Fake the bits parameter for some formats. */
		if(quicktime_match_32(compressor, QUICKTIME_ULAW) ||
			quicktime_match_32(compressor, QUICKTIME_IMA4)) bits = 16;

		file->atracks = (quicktime_audio_map_t*)
			calloc(1, sizeof(quicktime_audio_map_t));
		trak = quicktime_add_track(&(file->moov));
		quicktime_trak_init_audio(file, trak, channels, sample_rate, bits, 
			sample_size, time_scale, sample_duration, compressor);
		quicktime_init_audio_map(&(file->atracks[0]), trak);
		file->atracks[file->total_atracks].track = trak;
		file->atracks[file->total_atracks].channels = channels;
		file->atracks[file->total_atracks].current_position = 0;
		file->atracks[file->total_atracks].current_chunk = 1;
		file->total_atracks++;
	}
	return 1;   /* Return the number of tracks created */
}

int quicktime_set_video(quicktime_t *file, 
						int tracks, 
						int frame_w, 
						int frame_h,
						float frame_rate,
						int time_scale,
						char *compressor)
{
	int i, j;
	quicktime_trak_t *trak;

	/* delete any existing tracks */
	for(i = 0; i < file->total_vtracks; i++) {
		for (j = 0; j < file->vtracks[i].totalHintTracks; j++) {
			quicktime_delete_trak(&(file->moov), 
				file->vtracks[i].hintTracks[j]);
			file->vtracks[i].hintTracks[j] = NULL;
			file->total_hint_tracks--;
		}
		quicktime_delete_video_map(&(file->vtracks[i]));
		quicktime_delete_trak(&(file->moov), file->vtracks[i].track);
	}
	free(file->vtracks);
	file->vtracks = NULL;	
	file->total_vtracks = 0;

	if (tracks > 0) {
		file->total_vtracks = tracks;
		file->vtracks = (quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t) * file->total_vtracks);
		for(i = 0; i < tracks; i++)
		{
			trak = quicktime_add_track(&(file->moov));
			quicktime_trak_init_video(file, trak, frame_w, frame_h, frame_rate,
				time_scale, compressor);
			quicktime_init_video_map(&(file->vtracks[i]), trak);
		}
	}
	return 0;
}

int quicktime_set_audio_hint(quicktime_t *file, int audioTrack,
							char *payloadName, u_int* pPayloadNumber, 
							int maxPktSize)
{
	quicktime_audio_map_t* pMap = NULL;
	quicktime_trak_t* hintTrak = NULL;
	int timeScale;
	float frameRate;
	int sampleDuration;
	char rtpMapBuf[128];
	char sdpBuf[256];

	/* check our arguments */
	if (file == NULL) {
		return -1;
	}
	if (audioTrack < 0 || audioTrack > file->total_atracks) {
		return -1;
	}
	if (payloadName == NULL) {
		return -1;
	}

	pMap = &file->atracks[audioTrack];

	timeScale = quicktime_audio_time_scale(file, audioTrack);
	if (timeScale == 0) {
		return -1;
	}
	sampleDuration = file->atracks[audioTrack].track->mdia.minf.stbl.stts.table[0].sample_duration;
	
	/* add the hint track */
	hintTrak = quicktime_add_track(&(file->moov));
	if (*pPayloadNumber == 0) {
		(*pPayloadNumber) = 96 + file->total_hint_tracks++;
	}

	/* initialize it to reference the audio track */
	quicktime_trak_init_hint(file, hintTrak, pMap->track, 
		maxPktSize, timeScale, sampleDuration);

	/* set the payload info */
	hintTrak->hint_udta.hinf.payt.payloadNumber = *pPayloadNumber;
	sprintf(rtpMapBuf, "%s/%u", payloadName, timeScale);
	strcpy(hintTrak->hint_udta.hinf.payt.rtpMapString, rtpMapBuf);

	/* set the SDP media section */
	sprintf(sdpBuf, 
		"m=audio 0 RTP/AVP %u\015\012a=rtpmap:%u %s\015\012a=control:trackID=%u\015\012", 
		*pPayloadNumber, *pPayloadNumber, rtpMapBuf, hintTrak->tkhd.track_id);
	quicktime_sdp_set(&(hintTrak->hint_udta.hnti.sdp), sdpBuf);

	pMap->hintTracks[pMap->totalHintTracks] = hintTrak;
	pMap->hintPositions[pMap->totalHintTracks] = 0;
	pMap->totalHintTracks++;
	return (pMap->totalHintTracks - 1);
}

int quicktime_set_video_hint(quicktime_t *file, int videoTrack, 
						char *payloadName, u_int* pPayloadNumber, 
						int maxPktSize)
{
	quicktime_video_map_t* pMap = NULL;
	quicktime_trak_t* hintTrak = NULL;
	float frameRate;
	int timeScale;
	int sampleDuration;
	char rtpMapBuf[128];
	char sdpBuf[1024];

	/* check our arguments */
	if (file == NULL) {
		return -1;
	}
	if (videoTrack < 0 || videoTrack > file->total_vtracks) {
		return -1;
	}
	if (payloadName == NULL) {
		return -1;
	}

	frameRate = quicktime_video_frame_rate(file, videoTrack);
	if (frameRate == 0.0) {
		return -1;
	}
	timeScale = quicktime_video_time_scale(file, videoTrack);
	if (timeScale == 0) {
		return -1;
	}
	sampleDuration = timeScale / frameRate;
	if (sampleDuration == 0) {
		return -1;
	}

	/* add the hint track */
	hintTrak = quicktime_add_track(&(file->moov));
	if (*pPayloadNumber == 0) {
		(*pPayloadNumber) = 96 + file->total_hint_tracks++;
	}

	pMap = &file->vtracks[videoTrack];

	/* initialize it to reference the video track */
	quicktime_trak_init_hint(file, hintTrak, pMap->track, 
		maxPktSize, timeScale, sampleDuration);

	/* set the payload info */
	hintTrak->hint_udta.hinf.payt.payloadNumber = *pPayloadNumber;
	sprintf(rtpMapBuf, "%s/90000", payloadName);
	strcpy(hintTrak->hint_udta.hinf.payt.rtpMapString, rtpMapBuf);

	/* set the SDP media section */
	sprintf(sdpBuf, 
		"m=video 0 RTP/AVP %u\015\012a=rtpmap:%u %s\015\012a=control:trackID=%u\015\012", 
		*pPayloadNumber, *pPayloadNumber, rtpMapBuf, hintTrak->tkhd.track_id);
	quicktime_sdp_set(&(hintTrak->hint_udta.hnti.sdp), sdpBuf);

	pMap->hintTracks[pMap->totalHintTracks] = hintTrak;
	pMap->hintPositions[pMap->totalHintTracks] = 0;
	pMap->totalHintTracks++;
	return (pMap->totalHintTracks - 1);
}

char* quicktime_get_session_sdp(quicktime_t *file)
{
	return file->moov.udta.hnti.rtp.string;
}

int quicktime_set_session_sdp(quicktime_t *file, char* sdpString)
{
	return quicktime_rtp_set(&(file->moov.udta.hnti.rtp), sdpString);
}

int quicktime_add_audio_sdp(quicktime_t *file, char* sdpString, int track, int hintTrack)
{
	quicktime_trak_t* hintTrak = 
		file->atracks[track].hintTracks[hintTrack];

	quicktime_sdp_append(&(hintTrak->hint_udta.hnti.sdp), sdpString);
}

int quicktime_add_video_sdp(quicktime_t *file, char* sdpString, int track, int hintTrack)
{
	quicktime_trak_t* hintTrak = 
		file->vtracks[track].hintTracks[hintTrack];

	quicktime_sdp_append(&(hintTrak->hint_udta.hnti.sdp), sdpString);
}

static int quicktime_set_media_hint_max_rate(quicktime_t *file, 
	int granularity, int maxBitRate, quicktime_trak_t* hintTrak)
{
	hintTrak->hint_udta.hinf.maxr.granularity = granularity;
	hintTrak->hint_udta.hinf.maxr.maxBitRate = maxBitRate;

	hintTrak->mdia.minf.hmhd.maxbitrate = maxBitRate;
	/* Give upper bound on MP4 max bitrate for 1 minute window */
	hintTrak->mdia.minf.hmhd.slidingavgbitrate = 
		maxBitRate * (60000 / granularity);

	return 0;
}

int quicktime_set_audio_hint_max_rate(quicktime_t *file, 
	int granularity, int maxBitRate, int audioTrack, int hintTrack)
{
	quicktime_trak_t* hintTrak = 
		file->atracks[audioTrack].hintTracks[hintTrack];

	return quicktime_set_media_hint_max_rate(file,
		granularity, maxBitRate, hintTrak);
}

int quicktime_set_video_hint_max_rate(quicktime_t *file, 
	int granularity, int maxBitRate, int videoTrack, int hintTrack)
{
	quicktime_trak_t* hintTrak = 
		file->vtracks[videoTrack].hintTracks[hintTrack];

	return quicktime_set_media_hint_max_rate(file,
		granularity, maxBitRate, hintTrak);
}

int quicktime_set_framerate(quicktime_t *file, float framerate)
{
	int i;
	int new_time_scale, new_sample_duration;
	new_time_scale = quicktime_get_timescale(framerate);
	new_sample_duration = (int)((float)new_time_scale / framerate + 0.5);

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.mdhd.time_scale = new_time_scale;
		file->vtracks[i].track->mdia.minf.stbl.stts.table[0].sample_duration = new_sample_duration;
	}
}

quicktime_trak_t* quicktime_add_track(quicktime_moov_t *moov)
{
	quicktime_trak_t *trak;
	trak = moov->trak[moov->total_tracks] = calloc(1, sizeof(quicktime_trak_t));
	quicktime_trak_init(trak);
	trak->tkhd.track_id = moov->mvhd.next_track_id;
	moov->mvhd.next_track_id++;
	moov->total_tracks++;
	return trak;
}

/* ============================= Initialization functions */

int quicktime_init(quicktime_t *file)
{
	memset(file, 0, sizeof(quicktime_t));
	quicktime_mdat_init(&(file->mdat));
	quicktime_moov_init(&(file->moov));
	file->cpus = 1;
	return 0;
}

int quicktime_delete(quicktime_t *file)
{
	int i;
	if(file->total_atracks) 
	{
		for(i = 0; i < file->total_atracks; i++)
			quicktime_delete_audio_map(&(file->atracks[i]));
		free(file->atracks);
	}
	if(file->total_vtracks)
	{
		for(i = 0; i < file->total_vtracks; i++)
			quicktime_delete_video_map(&(file->vtracks[i]));
		free(file->vtracks);
	}
	file->total_atracks = 0;
	file->total_vtracks = 0;
	if(file->preload_size)
	{
		free(file->preload_buffer);
		file->preload_size = 0;
	}
	quicktime_moov_delete(&(file->moov));
	quicktime_mdat_delete(&(file->mdat));
	return 0;
}

/* =============================== Optimization functions */

int quicktime_set_cpus(quicktime_t *file, int cpus)
{
	if(cpus > 0) file->cpus = cpus;
	return 0;
}

int quicktime_set_preload(quicktime_t *file, long preload)
{
	if(!file->preload_size)
	{
		file->preload_size = preload;
		file->preload_buffer = calloc(1, preload);
		file->preload_start = 0;
		file->preload_end = 0;
		file->preload_ptr = 0;
	}
}


int quicktime_get_timescale(float frame_rate)
{
	int timescale = 600;
/* Encode the 29.97, 23.976, 59.94 framerates as per DV freaks */
	if(frame_rate - (int)frame_rate != 0) timescale = (int)(frame_rate * 1001 + 0.5);
	else
	if((600 / frame_rate) - (int)(600 / frame_rate) != 0) timescale = (int)(frame_rate * 100 + 0.5);
	return timescale;
}

int quicktime_seek_end(quicktime_t *file)
{
	quicktime_set_position(file, file->mdat.size + file->mdat.start);
/*printf("quicktime_seek_end %ld\n", file->mdat.size + file->mdat.start); */
	quicktime_update_positions(file);
	return 0;
}

int quicktime_seek_start(quicktime_t *file)
{
	quicktime_set_position(file, file->mdat.start + HEADER_LENGTH);
	quicktime_update_positions(file);
	return 0;
}

long quicktime_audio_length(quicktime_t *file, int track)
{
	if(file->total_atracks > 0) 
		return quicktime_track_samples(file, file->atracks[track].track);

	return 0;
}

long quicktime_video_length(quicktime_t *file, int track)
{
/*printf("quicktime_video_length %d %d\n", quicktime_track_samples(file, file->vtracks[track].track), track); */
	if(file->total_vtracks > 0)
		return quicktime_track_samples(file, file->vtracks[track].track);
	return 0;
}

long quicktime_audio_position(quicktime_t *file, int track)
{
	return file->atracks[track].current_position;
}

long quicktime_video_position(quicktime_t *file, int track)
{
	return file->vtracks[track].current_position;
}

int quicktime_update_positions(quicktime_t *file)
{
/* Used for routines that change the positions of all tracks, like */
/* seek_end and seek_start but not for routines that reposition one track, like */
/* set_audio_position. */

	long mdat_offset = quicktime_position(file) - file->mdat.start;
	long sample, chunk, chunk_offset;
	int i;

	if(file->total_atracks)
	{
		sample = quicktime_offset_to_sample(file->atracks[0].track, mdat_offset);
		chunk = quicktime_offset_to_chunk(&chunk_offset, file->atracks[0].track, mdat_offset);
		for(i = 0; i < file->total_atracks; i++)
		{
			file->atracks[i].current_position = sample;
			file->atracks[i].current_chunk = chunk;
		}
	}

	if(file->total_vtracks)
	{
		sample = quicktime_offset_to_sample(file->vtracks[0].track, mdat_offset);
		chunk = quicktime_offset_to_chunk(&chunk_offset, file->vtracks[0].track, mdat_offset);
		for(i = 0; i < file->total_vtracks; i++)
		{
			file->vtracks[i].current_position = sample;
			file->vtracks[i].current_chunk = chunk;
		}
	}
	return 0;
}

int quicktime_set_audio_position(quicktime_t *file, long sample, int track)
{
	long offset, chunk_sample, chunk;
	quicktime_trak_t *trak;

	if(file->total_atracks)
	{
		trak = file->atracks[track].track;
		file->atracks[track].current_position = sample;
		quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
		file->atracks[track].current_chunk = chunk;
		offset = quicktime_sample_to_offset(trak, sample);
		quicktime_set_position(file, offset);
		/*quicktime_update_positions(file); */
	}

	return 0;
}

int quicktime_set_video_position(quicktime_t *file, long frame, int track)
{
	long offset, chunk_sample, chunk;
	quicktime_trak_t *trak;

	if(file->total_vtracks)
	{
		trak = file->vtracks[track].track;
		file->vtracks[track].current_position = frame;
		quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, frame);
		file->vtracks[track].current_chunk = chunk;
		offset = quicktime_sample_to_offset(trak, frame);
		quicktime_set_position(file, offset);
		/*quicktime_update_positions(file); */
	}
	return 0;
}

int quicktime_has_audio(quicktime_t *file)
{
	if(quicktime_audio_tracks(file)) return 1;
	return 0;
}

long quicktime_audio_sample_rate(quicktime_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_rate;
	return 0;
}

int quicktime_audio_bits(quicktime_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_size;

	return 0;
}

int quicktime_audio_time_scale(quicktime_t *file, int track)
{
	if(file->total_atracks) {
		return file->atracks[track].track->mdia.mdhd.time_scale;
	}
	return 0;
}

int quicktime_audio_sample_duration(quicktime_t *file, int track)
{
	if(file->total_atracks) {
		return file->atracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration;
	}
	return 0;
}

char* quicktime_audio_compressor(quicktime_t *file, int track)
{
  if (file->atracks[track].track == NULL)
    return (NULL);
	return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int quicktime_track_channels(quicktime_t *file, int track)
{
	if(track < file->total_atracks)
		return file->atracks[track].channels;

	return 0;
}

int quicktime_channel_location(quicktime_t *file, int *quicktime_track, int *quicktime_channel, int channel)
{
	int current_channel = 0, current_track = 0;
	*quicktime_channel = 0;
	*quicktime_track = 0;
	for(current_channel = 0, current_track = 0; current_track < file->total_atracks; )
	{
		if(channel >= current_channel)
		{
			*quicktime_channel = channel - current_channel;
			*quicktime_track = current_track;
		}

		current_channel += file->atracks[current_track].channels;
		current_track++;
	}
	return 0;
}

int quicktime_has_video(quicktime_t *file)
{
	if(quicktime_video_tracks(file)) return 1;
	return 0;
}

int quicktime_video_width(quicktime_t *file, int track)
{
	if(file->total_vtracks) {
		int width = 
			file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].width;
		if (width) {
			return width;
		}
		return file->vtracks[track].track->tkhd.track_width;
	}
	return 0;
}

int quicktime_video_height(quicktime_t *file, int track)
{
	if(file->total_vtracks) {
		int height = file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].height;
		if (height) {
			return height;
		}
		return file->vtracks[track].track->tkhd.track_height;
	}
	return 0;
}

int quicktime_video_depth(quicktime_t *file, int track)
{
	if(file->total_vtracks)
		return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].depth;
	return 0;
}

int quicktime_set_depth(quicktime_t *file, int depth, int track)
{
	int i;

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].depth = depth;
	}
}

float quicktime_video_frame_rate(quicktime_t *file, int track)
{
  float ret = 0;
  int num = 0;
  
  if(file->total_vtracks) {
    ret = file->vtracks[track].track->mdia.mdhd.time_scale;
    if (file->vtracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration == 0)
      num = 1;
    ret /= 
	file->vtracks[track].track->mdia.minf.stbl.stts.table[num].sample_duration;
  }
  return ret;
}

char* quicktime_video_compressor(quicktime_t *file, int track)
{
  if (file->vtracks[track].track == NULL)
    return (NULL);
  
	return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int quicktime_video_time_scale(quicktime_t *file, int track)
{
	if(file->total_vtracks) {
		return file->vtracks[track].track->mdia.mdhd.time_scale;
	}
	return 0;
}

int quicktime_video_frame_time(quicktime_t *file, int track, long frame,
	long *start, int *duration)
{
	quicktime_stts_t *stts;
	int i;
	long f;

	if (file->total_vtracks == 0) {
		return 0;
	}
	stts = &(file->vtracks[track].track->mdia.minf.stbl.stts);

	if (frame < file->last_frame) {
		/* we need to reset our cached values */
		file->last_frame = 0;
		file->last_start = 0;
		file->last_stts_index = 0;
	}

	i = file->last_stts_index;
	f = file->last_frame;
	*start = file->last_start;

	while (i < stts->total_entries) {
		if (f + stts->table[i].sample_count <= frame) {
			*start += stts->table[i].sample_duration 
				* stts->table[i].sample_count;
			f += stts->table[i].sample_count;
			i++;

		} else {
			/* cache the results for future use */
			file->last_stts_index = i;
			file->last_frame = f;
			file->last_start = *start;

			*start += stts->table[i].sample_duration * (frame - f);
			*duration = stts->table[i].sample_duration;

			return 1;
		}
	}

	/* error */
	return 0;
}

int quicktime_get_mp4_video_decoder_config(quicktime_t *file, int track, u_char** ppBuf, int* pBufSize)
{
	quicktime_esds_t* esds;

	if (!file->total_vtracks) {
		return 0;
	}
	esds = &file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return quicktime_esds_get_decoder_config(esds, ppBuf, pBufSize);
}

int quicktime_set_mp4_video_decoder_config(quicktime_t *file, int track, u_char* pBuf, int bufSize)
{
	quicktime_esds_t* esds;

	if (!file->total_vtracks) {
		return 0;
	}
	esds = &file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return quicktime_esds_set_decoder_config(esds, pBuf, bufSize);
}

int quicktime_get_mp4_audio_decoder_config(quicktime_t *file, int track, u_char** ppBuf, int* pBufSize)
{
	quicktime_esds_t* esds;

	if (!file->total_atracks) {
		return 0;
	}
	esds = &file->atracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return quicktime_esds_get_decoder_config(esds, ppBuf, pBufSize);
}

int quicktime_set_mp4_audio_decoder_config(quicktime_t *file, int track, u_char* pBuf, int bufSize)
{
	quicktime_esds_t* esds;

	if (!file->total_atracks) {
		return 0;
	}
	esds = &file->atracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return quicktime_esds_set_decoder_config(esds, pBuf, bufSize);
}

long quicktime_samples_to_bytes(quicktime_trak_t *track, long samples)
{
	return samples
		* track->mdia.minf.stbl.stsd.table[0].channels
		* track->mdia.minf.stbl.stsd.table[0].sample_size / 8;
}

int quicktime_write_audio(quicktime_t *file, char *audio_buffer, long samples, int track)
{
	long offset;
	int result;
	long bytes;

/* Defeat 32 bit file size limit. */
	if(quicktime_test_position(file)) return 1;

/* write chunk for 1 track */
	bytes = samples * quicktime_audio_bits(file, track) / 8 * file->atracks[track].channels;
	offset = quicktime_position(file);
	result = quicktime_write_data(file, audio_buffer, bytes);

	if(result) result = 0; else result = 1; /* defeat fwrite's return */
	quicktime_update_tables(file, 
						file->atracks[track].track, 
						offset, 
						file->atracks[track].current_chunk, 
						file->atracks[track].current_position, 
						samples, 
						0,
						0,
						0,
						0);
	file->atracks[track].current_position += samples;
	file->atracks[track].current_chunk++;
	return result;
}

int quicktime_write_audio_frame(quicktime_t *file, unsigned char *audio_buffer, long bytes, int track)
{
	long offset = quicktime_position(file);
	int result = 0;

	/* Defeat 32 bit file size limit. */
	if(quicktime_test_position(file)) return 1;

	result = quicktime_write_data(file, audio_buffer, bytes);
	if(result) result = 0; else result = 1;

	quicktime_update_tables(file,
						file->atracks[track].track,
						offset,
						file->atracks[track].current_chunk,
						file->atracks[track].current_position,
						1, 
						bytes,
						0,
						0,
						0);
	file->atracks[track].current_position += 1;
	file->atracks[track].current_chunk++;
	return result;
}

int quicktime_write_video_frame(quicktime_t *file, 
								unsigned char *video_buffer, 
								long bytes, 
								int track, 
								u_char isKeyFrame,
								long duration,
								long renderingOffset)
{
	long offset = quicktime_position(file);
	int result = 0;

	/* Defeat 32 bit file size limit. */
	if(quicktime_test_position(file)) return 1;

	result = quicktime_write_data(file, video_buffer, bytes);
	if(result) result = 0; else result = 1;

	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes,
						duration,
						isKeyFrame,
						renderingOffset);
	file->vtracks[track].current_position += 1;
	file->vtracks[track].current_chunk++;
	return result;
}

static int quicktime_write_media_hint(quicktime_t* file,
				u_char* hintBuffer,
				long hintBufferSize,
				quicktime_trak_t* hintTrak,
				long* pSampleNumber,
				long hintSampleDuration,
				u_char isSyncSample)
{

	long offset = quicktime_position(file);
	quicktime_hint_info_t hintInfo;
	int result = 0;

	/* handle 32 bit file size limit */
	if (quicktime_test_position(file)) {
		return 1;
	}

	/* get info about this hint */
	quicktime_get_hint_info(hintBuffer, hintBufferSize, &hintInfo);

	/* update overall info */
	hintTrak->hint_udta.hinf.trpy.numBytes += hintInfo.trpy; 
	hintTrak->hint_udta.hinf.nump.numPackets += hintInfo.nump;
	hintTrak->hint_udta.hinf.tpyl.numBytes += hintInfo.tpyl;
	hintTrak->hint_udta.hinf.dmed.numBytes += hintInfo.dmed;
	hintTrak->hint_udta.hinf.drep.numBytes += hintInfo.drep;
	hintTrak->hint_udta.hinf.dimm.numBytes += hintInfo.dimm;
	hintTrak->hint_udta.hinf.tmin.milliSecs = 
		MAX(hintInfo.tmin, hintTrak->hint_udta.hinf.tmin.milliSecs);
	hintTrak->hint_udta.hinf.tmax.milliSecs = 
		MAX(hintInfo.tmax, hintTrak->hint_udta.hinf.tmax.milliSecs);
	hintTrak->hint_udta.hinf.pmax.numBytes = 
		MAX(hintInfo.pmax, hintTrak->hint_udta.hinf.pmax.numBytes);

	hintTrak->mdia.minf.hmhd.maxPDUsize = 
		hintTrak->hint_udta.hinf.pmax.numBytes;
	hintTrak->mdia.minf.hmhd.avgPDUsize = 
		hintTrak->hint_udta.hinf.trpy.numBytes
		/ hintTrak->hint_udta.hinf.nump.numPackets;

	/* write the hint data */
	result = quicktime_write_data(file, hintBuffer, hintBufferSize);
	result = (result ? 0 : 1);
	
	quicktime_update_tables(file, hintTrak, offset, *pSampleNumber + 1,
		*pSampleNumber, 1, hintBufferSize, hintSampleDuration, isSyncSample, 0);
	(*pSampleNumber)++;
	return result;
}

int quicktime_write_audio_hint(quicktime_t *file, 
				unsigned char *hintBuffer, 
				long hintBufferSize, 
				int audioTrack,
				int hintTrack,
				long hintSampleDuration)
{
	quicktime_trak_t* hintTrak = 
		file->atracks[audioTrack].hintTracks[hintTrack];

	return quicktime_write_media_hint(file, hintBuffer, hintBufferSize,
		hintTrak, &(file->atracks[audioTrack].hintPositions[hintTrack]),
		hintSampleDuration, 0);
}

int quicktime_write_video_hint(quicktime_t *file, 
				unsigned char *hintBuffer, 
				long hintBufferSize, 
				int videoTrack,
				int hintTrack,
				long hintSampleDuration,
				u_char isKeyFrame)
{
	quicktime_trak_t* hintTrak = 
		file->vtracks[videoTrack].hintTracks[hintTrack];

	return quicktime_write_media_hint(file, hintBuffer, hintBufferSize,
		hintTrak, &(file->vtracks[videoTrack].hintPositions[hintTrack]), 
		hintSampleDuration, isKeyFrame);
}

FILE* quicktime_get_fd(quicktime_t *file)
{
	if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
	return file->stream;
}

int quicktime_write_frame_init(quicktime_t *file, int track)
{
	if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
	file->offset = quicktime_position(file);
	return 0;
}

int quicktime_write_frame_end(quicktime_t *file, int track)
{
	long bytes;
	file->file_position = ftell(file->stream);
	bytes = quicktime_position(file) - file->offset;
	quicktime_update_tables(file,
						file->vtracks[track].track,
						file->offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes,
						0,
						0,
						0);
	file->vtracks[track].current_position += 1;
	file->vtracks[track].current_chunk++;
	return 0;
}

int quicktime_write_audio_init(quicktime_t *file, int track)
{
	return quicktime_write_frame_init(file, track);
}

int quicktime_write_audio_end(quicktime_t *file, int track, long samples)
{
	long bytes;
	file->file_position = ftell(file->stream);
	bytes = quicktime_position(file) - file->offset;
	quicktime_update_tables(file, 
						file->atracks[track].track,
						file->offset,
						file->atracks[track].current_chunk,
						file->atracks[track].current_position,
						samples,
						bytes, 
						0,
						0,
						0);
	file->atracks[track].current_position += samples;
	file->atracks[track].current_chunk++;
	return 0;
}


long quicktime_read_audio(quicktime_t *file, char *audio_buffer, long samples, int track)
{
	long chunk_sample, chunk;
	int result = 1, track_num;
	quicktime_trak_t *trak = file->atracks[track].track;
	long fragment_len, chunk_end;
	long position = file->atracks[track].current_position;
	long start = position, end = position + samples;
	long bytes, total_bytes = 0;
	long buffer_offset;

	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, position);
	buffer_offset = 0;

	while(position < end && result)
	{
		quicktime_set_audio_position(file, position, track);
		fragment_len = quicktime_chunk_samples(trak, chunk);
		chunk_end = chunk_sample + fragment_len;
		fragment_len -= position - chunk_sample;
		if(position + fragment_len > chunk_end) fragment_len = chunk_end - position;
		if(position + fragment_len > end) fragment_len = end - position;

		bytes = quicktime_samples_to_bytes(trak, fragment_len);
		result = quicktime_read_data(file, &audio_buffer[buffer_offset], bytes);

		total_bytes += bytes;
		position += fragment_len;
		chunk_sample = position;
		buffer_offset += bytes;
		chunk++;
	}

	file->atracks[track].current_position = position;
	if(!result) return 0;
	return total_bytes;
}

int quicktime_read_chunk(quicktime_t *file, char *output, int track, long chunk, long byte_start, long byte_len)
{
	quicktime_set_position(file, quicktime_chunk_to_offset(file->atracks[track].track, chunk) + byte_start);
	if(quicktime_read_data(file, output, byte_len)) return 0;
	else
	return 1;
}

long quicktime_frame_size(quicktime_t *file, long frame, int track)
{
	int bytes;
	quicktime_trak_t *trak = file->vtracks[track].track;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		bytes = trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	{
		bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
	}

	return bytes;
}

long quicktime_audio_frame_size(quicktime_t *file, long frame, int track)
{
	int bytes;
	quicktime_trak_t *trak = file->atracks[track].track;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		bytes = trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	{
		bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
	}

	return bytes;
}

long quicktime_read_audio_frame(quicktime_t *file, unsigned char *audio_buffer,  int maxBytes, int track)
{
	long bytes;
	int result = 0;
	quicktime_trak_t *trak = file->atracks[track].track;

	bytes = quicktime_audio_frame_size(file, 
		file->atracks[track].current_position, track);

	if (bytes > maxBytes) {
		return -bytes;
	}

	quicktime_set_audio_position(file, 
		file->atracks[track].current_position, track);

	result = quicktime_read_data(file, audio_buffer, bytes);

	file->atracks[track].current_position++;

	if (!result)
		return 0;
	return bytes;
}

long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track)
{
	long bytes;
	int result = 0;

	quicktime_trak_t *trak = file->vtracks[track].track;
	bytes = quicktime_frame_size(file, file->vtracks[track].current_position, track);

	if(!file->vtracks[track].frames_cached)
	{
		quicktime_set_video_position(file, file->vtracks[track].current_position, track);
		result = quicktime_read_data(file, video_buffer, bytes);
	}
	else
	{
		int i;
		unsigned char *cached_frame;

		if(file->vtracks[track].current_position >= file->vtracks[track].frames_cached) result = 1;
		if(!result)
		{
			cached_frame = file->vtracks[track].frame_cache[file->vtracks[track].current_position];

			for(i = 0; i < bytes; i++)
				video_buffer[i] = cached_frame[i];
		}
	}
	file->vtracks[track].current_position++;

	if(!result) return 0;
	return bytes;
}

int quicktime_read_frame_init(quicktime_t *file, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_set_video_position(file, file->vtracks[track].current_position, track);
	if(ftell(file->stream) != file->file_position) fseek(file->stream, file->file_position, SEEK_SET);
	return 0;
}

int quicktime_read_frame_end(quicktime_t *file, int track)
{
	file->file_position = ftell(file->stream);
	file->vtracks[track].current_position++;
	return 0;
}

int quicktime_init_video_map(quicktime_video_map_t *vtrack, quicktime_trak_t *trak)
{
	vtrack->track = trak;
	vtrack->current_position = 0;
	vtrack->current_chunk = 1;
	vtrack->frame_cache = 0;
	vtrack->frames_cached = 0;
	return 0;
}

int quicktime_delete_video_map(quicktime_video_map_t *vtrack)
{
	int i;
	if(vtrack->frames_cached)
	{
		for(i = 0; i < vtrack->frames_cached; i++)
		{
			free(vtrack->frame_cache[i]);
		}
		free(vtrack->frame_cache);
		vtrack->frames_cached = 0;
	}
	return 0;
}

int quicktime_init_audio_map(quicktime_audio_map_t *atrack, quicktime_trak_t *trak)
{
	atrack->track = trak;
	atrack->channels = trak->mdia.minf.stbl.stsd.table[0].channels;
	atrack->current_position = 0;
	atrack->current_chunk = 1;
	return 0;
}

int quicktime_delete_audio_map(quicktime_audio_map_t *atrack)
{
	int i;
	return 0;
}

int quicktime_read_info(quicktime_t *file)
{
	int result = 0, found_moov = 0;
	int i, j, k, m, channel, trak_channel, track;
	long start_position = quicktime_position(file);
	quicktime_atom_t leaf_atom;
	quicktime_trak_t *trak;
	char avi_test[4];

/* Check for Microsoft AVI */
	quicktime_read_char32(file, avi_test);
	if(quicktime_match_32(avi_test, "RIFF"))
	{
		quicktime_read_char32(file, avi_test);
		quicktime_read_char32(file, avi_test);
		if(quicktime_match_32(avi_test, "AVI "))
			file->use_avi = 1;
	}

	quicktime_set_position(file, 0);

	do
	{
		result = quicktime_atom_read_header(file, &leaf_atom);
		if(!result)
		{
			if(quicktime_atom_is(&leaf_atom, "mdat")) {
				quicktime_read_mdat(file, &(file->mdat), &leaf_atom);
			} else if(quicktime_atom_is(&leaf_atom, "moov")) {
				quicktime_read_moov(file, &(file->moov), &leaf_atom);
				found_moov = 1;
			} else {
				quicktime_atom_skip(file, &leaf_atom);
			}
		}
	}while(!result && quicktime_position(file) < file->total_length);

/* go back to the original position */
	quicktime_set_position(file, start_position);

	if(found_moov) {

		/* get tables for all the different tracks */
		file->total_atracks = quicktime_audio_tracks(file);
		file->atracks = (quicktime_audio_map_t*)calloc(1, 
			sizeof(quicktime_audio_map_t) * file->total_atracks);

		for(i = 0, track = 0; i < file->total_atracks; i++) {
			while(!file->moov.trak[track]->mdia.minf.is_audio)
				track++;
			quicktime_init_audio_map(&(file->atracks[i]), file->moov.trak[track]);
		}

		file->total_vtracks = quicktime_video_tracks(file);
		file->vtracks = (quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t) * file->total_vtracks);

		for(track = 0, i = 0; i < file->total_vtracks; i++)
		{
			while(!file->moov.trak[track]->mdia.minf.is_video)
				track++;

			quicktime_init_video_map(&(file->vtracks[i]), file->moov.trak[track]);
		}

		/* for all tracks */
		for (track = 0; track < file->moov.total_tracks; track++) {

			/* check if it's a hint track */
			if (!file->moov.trak[track]->mdia.minf.is_hint) {
				continue;
			}

			/* it is, so for each reference */
			for (j = 0; j < file->moov.trak[track]->tref.hint.numTracks; j++) {

				/* get the reference track id */
				long refTrackId = file->moov.trak[track]->tref.hint.trackIds[j]; 
				/* check each audio track */
				for(k = 0; k < file->total_atracks; k++) {
					if (file->atracks[k].track->tkhd.track_id == refTrackId) {
						int m = file->atracks[k].totalHintTracks++;
						file->atracks[k].hintTracks[m] = file->moov.trak[track];
						file->atracks[k].hintPositions[m] = 0;
						file->moov.trak[track]->tref.hint.traks[j] = 
							file->atracks[k].track;
						file->total_hint_tracks++;
						break;
					}
				}

				/* check each video track */
				for(k = 0; k < file->total_vtracks; k++) {
					if (file->vtracks[k].track->tkhd.track_id == refTrackId) {
						int m = file->vtracks[k].totalHintTracks++;
						file->vtracks[k].hintTracks[m] = file->moov.trak[track];
						file->vtracks[k].hintPositions[m] = 0;
						file->moov.trak[track]->tref.hint.traks[j] = 
							file->vtracks[k].track;
						file->total_hint_tracks++;
						break;
					}
				}
			}
		}
	}

	if(found_moov) 
		return 0; 
	else 
		return 1;
}


int quicktime_dump(quicktime_t *file)
{
	printf("quicktime_dump\n");
	printf("movie data\n");
	printf(" size %ld\n", file->mdat.size);
	printf(" start %ld\n", file->mdat.start);
	quicktime_moov_dump(&(file->moov));
	return 0;
}


/* ================================== Entry points ========================== */

int quicktime_check_sig(const char *path)
{
	quicktime_t file;
	quicktime_atom_t leaf_atom;
	int result1 = 0, result2 = 0;

	quicktime_init(&file);
	if(!(file.stream = fopen(path, "rb"))) 
	{
		perror("quicktime_check_sig");
		return 0;
	}

	fseek(file.stream, 0, SEEK_END);
	file.total_length = ftell(file.stream);
	fseek(file.stream, 0, SEEK_SET);

	do
	{
		result1 = quicktime_atom_read_header(&file, &leaf_atom);

		if(!result1)
		{
/* just want the "moov" atom */
			if(quicktime_atom_is(&leaf_atom, "moov"))
			{
				result2 = 1;
			}
			else
				quicktime_atom_skip(&file, &leaf_atom);
		}
	}while(!result1 && !result2 && quicktime_position(&file) < file.total_length);

	fclose(file.stream);

	quicktime_delete(&file);
	return result2;
}

quicktime_t* quicktime_open(char *filename, int rd, int wr, int append)
{
	quicktime_t *new_file = malloc(sizeof(quicktime_t));
	char flags[10];
	int exists = 0;

	quicktime_init(new_file);
	new_file->wr = wr;
	new_file->rd = rd;
	new_file->mdat.start = 0;

	if (!strcmp(&filename[strlen(filename)-4], ".mp4")) {
		new_file->use_mp4 = TRUE;
	} else {
		new_file->use_mp4 = FALSE;
	}

	if(rd && (new_file->stream = fopen(filename, "rb")))
	{
		exists = 1; 
		fclose(new_file->stream); 
		new_file->stream = NULL;
	}


	if(rd && !wr) 
		sprintf(flags, "rb");
	else if(!rd && wr) 
		sprintf(flags, "wb");
	else if(rd && wr)
	{
		if(exists) 
			sprintf(flags, "rb+");
		else
			sprintf(flags, "wb+");
	}

	if(!(new_file->stream = fopen(filename, flags)))
	{
		perror("quicktime_open");
		free(new_file);
		return 0;
	}


	if(rd && exists)
	{
		fseek(new_file->stream, 0, SEEK_END);
		new_file->total_length = ftell(new_file->stream);

		fseek(new_file->stream, 0, SEEK_SET);

		if(quicktime_read_info(new_file))
		{
			quicktime_close(new_file);
			new_file = 0;
		}
	}

	if(wr) {
		if(!exists || !append) {
			/* start the data atom */
			quicktime_write_int32(new_file, 0);
			quicktime_write_char32(new_file, "mdat");
		} else {
			quicktime_set_position(new_file,
				 new_file->mdat.start + new_file->mdat.size);
			fseek(new_file->stream, 
				 new_file->mdat.start + new_file->mdat.size, SEEK_SET);
		}
	}
	return new_file;
}

int quicktime_write(quicktime_t *file)
{
	int result = -1;

	if(!file->wr) {
		return result;
	}

	quicktime_write_mdat(file, &(file->mdat));
	quicktime_write_moov(file, &(file->moov));

	result = fclose(file->stream);
	file->stream = NULL;
	return result;
}

int quicktime_destroy(quicktime_t *file)
{
	quicktime_delete(file);
	free(file);
	return 0;
}

int quicktime_close(quicktime_t *file)
{
	int result;
	if(file->wr)
	{
		quicktime_write_mdat(file, &(file->mdat));
		quicktime_write_moov(file, &(file->moov));
	}
	result = fclose(file->stream);

	quicktime_delete(file);
	free(file);
	return result;
}
