#ifndef QUICKTIME_H
#define QUICKTIME_H

#ifdef __cplusplus
extern "C" {
#endif
#include "mpeg4ip.h"

#include <stdio.h>
#include <stdlib.h>


#include "rtphint.h"
#include "private.h"
#include "funcprotos.h"
#include "sizes.h"


/* This is the reference for all your library entry points. */

/* Color models.  Not all compression formats support QUICKTIME_BGR8880. */
/* You must verify the support of QUICKTIME_BGR8880 with quicktime_test_colormodel. */
#define QUICKTIME_RGB888   0
#define QUICKTIME_BGR8880  1
#define QUICKTIME_RGBA8880 2

/* ===================== compression formats =================== // */
/* The compression formats are the formats supported natively by Quicktime 4 Linux. */

/* DV works only for decoding. */
#define QUICKTIME_DV "dvc "

/* RGB uncompressed.  Allows alpha */
#define QUICKTIME_RAW  "raw "

/* Jpeg Photo */
#define QUICKTIME_JPEG "jpeg"

/* Motion JPEG-A. */
#define QUICKTIME_MJPA "mjpa"

/* YUV9.  What on earth for? */
#define QUICKTIME_YUV9 "YVU9"

/* RTjpeg, proprietary but fast? */
#define QUICKTIME_RTJ0 "RTJ0"

/* YUV 4:2:2 */
#define QUICKTIME_YUV2 "yuv2"

/* YUV 4:2:0  NOT COMPATIBLE WITH STANDARD QUICKTIME */
#define QUICKTIME_YUV4 "yuv4"

/* Concatenated png images.  Allows alpha */
#define QUICKTIME_PNG "png "

/* Audio formats */

/* Unsigned 8 bit */
#ifndef QUICKTIME_RAW
#define QUICKTIME_RAW "raw "
#endif

/* IMA4 */
#define QUICKTIME_IMA4 "ima4"

/* Twos compliment 8, 16, 24 */
#define QUICKTIME_TWOS "twos"

/* ulaw */
#define QUICKTIME_ULAW "ulaw"

/* =========================== public interface ========================= // */

/* return 1 if the file is a quicktime file */
int quicktime_check_sig(const char *path);

/* call this first to open the file and create all the objects */
quicktime_t* quicktime_open(char *filename, int rd, int wr, int append);

/* make the quicktime file streamable */
int quicktime_make_streamable(char *in_path, char *out_path);

/* Set various options in the file. */
int quicktime_set_time_scale(quicktime_t *file, int time_scale);
int quicktime_set_copyright(quicktime_t *file, char *string);
int quicktime_set_name(quicktime_t *file, char *string);
int quicktime_set_info(quicktime_t *file, char *string);
int quicktime_get_time_scale(quicktime_t *file);
char* quicktime_get_copyright(quicktime_t *file);
char* quicktime_get_name(quicktime_t *file);
char* quicktime_get_info(quicktime_t *file);

/* Read all the information about the file. */
/* Requires a MOOV atom be present in the file. */
/* If no MOOV atom exists return 1 else return 0. */
int quicktime_read_info(quicktime_t *file);

/* set up tracks in a new file after opening and before writing */
/* returns the number of quicktime tracks allocated */
/* audio is stored two channels per quicktime track */
int quicktime_set_audio(quicktime_t *file, int channels, long sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);

int quicktime_set_audio_hint(quicktime_t *file, int track, char *payloadName, u_int* pPayloadNumber, int maxPktSize);

/* Samplerate can be set after file is created */
int quicktime_set_framerate(quicktime_t *file, float framerate);

/* video is stored one layer per quicktime track */
int quicktime_set_video(quicktime_t *file, int tracks, int frame_w, int frame_h, float frame_rate, int time_scale, char *compressor);

int quicktime_set_video_hint(quicktime_t *file, int track, char *payloadName, u_int* pPayloadNumber, int maxPktSize);

/* routines for setting various video parameters */
/* should be called after set_video */
int quicktime_set_jpeg(quicktime_t *file, int quality, int use_float);

/* Set the depth of the track. */
int quicktime_set_depth(quicktime_t *file, int depth, int track);

/* close the file and delete all the objects */
int quicktime_write(quicktime_t *file);
int quicktime_destroy(quicktime_t *file);
int quicktime_close(quicktime_t *file);

/* get length information */
/* channel numbers start on 1 for audio and video */
long quicktime_audio_length(quicktime_t *file, int track);
long quicktime_video_length(quicktime_t *file, int track);

/* get position information */
long quicktime_audio_position(quicktime_t *file, int track);
long quicktime_video_position(quicktime_t *file, int track);

/* get file information */
int quicktime_video_tracks(quicktime_t *file);
int quicktime_audio_tracks(quicktime_t *file);

int quicktime_has_audio(quicktime_t *file);
long quicktime_audio_sample_rate(quicktime_t *file, int track);
int quicktime_audio_bits(quicktime_t *file, int track);
int quicktime_track_channels(quicktime_t *file, int track);
int quicktime_audio_time_scale(quicktime_t *file, int track);
int quicktime_audio_sample_duration(quicktime_t *file, int track);
char* quicktime_audio_compressor(quicktime_t *file, int track);

int quicktime_has_video(quicktime_t *file);
int quicktime_video_width(quicktime_t *file, int track);
int quicktime_video_height(quicktime_t *file, int track);
int quicktime_video_depth(quicktime_t *file, int track);
float quicktime_video_frame_rate(quicktime_t *file, int track);
char* quicktime_video_compressor(quicktime_t *file, int track);
int quicktime_video_time_scale(quicktime_t *file, int track);

int quicktime_video_frame_time(quicktime_t *file, int track, long frame,
	long *start_time, int *duration);

int quicktime_get_iod_audio_profile_level(quicktime_t *file);
int quicktime_set_iod_audio_profile_level(quicktime_t *file, int id);
int quicktime_get_iod_video_profile_level(quicktime_t *file);
int quicktime_set_iod_video_profile_level(quicktime_t *file, int id);

int quicktime_get_mp4_video_decoder_config(quicktime_t *file, int track, u_char** ppBuf, int* pBufSize);
int quicktime_set_mp4_video_decoder_config(quicktime_t *file, int track, u_char* pBuf, int bufSize);
int quicktime_get_mp4_audio_decoder_config(quicktime_t *file, int track, u_char** ppBuf, int* pBufSize);
int quicktime_set_mp4_audio_decoder_config(quicktime_t *file, int track, u_char* pBuf, int bufSize);

char* quicktime_get_session_sdp(quicktime_t *file);
int quicktime_set_session_sdp(quicktime_t *file, char* sdpString);

int quicktime_add_audio_sdp(quicktime_t *file, char* sdpString, int track, int hintTrack);

int quicktime_add_video_sdp(quicktime_t *file, char* sdpString, int track, int hintTrack);

int quicktime_set_audio_hint_max_rate(quicktime_t *file, int granularity, int maxBitRate, int audioTrack, int hintTrack);

int quicktime_set_video_hint_max_rate(quicktime_t *file, int granularity, int maxBitRate, int videoTrack, int hintTrack);

/* number of bytes of raw data in this frame */
long quicktime_frame_size(quicktime_t *file, long frame, int track);

/* get the quicktime track and channel that the audio channel belongs to */
/* channels and tracks start on 0 */
int quicktime_channel_location(quicktime_t *file, int *quicktime_track, int *quicktime_channel, int channel);

/* file positioning */
int quicktime_seek_end(quicktime_t *file);
int quicktime_seek_start(quicktime_t *file);

/* set position of file descriptor relative to a track */
int quicktime_set_audio_position(quicktime_t *file, long sample, int track);
int quicktime_set_video_position(quicktime_t *file, long frame, int track);

/* ========================== Access to raw data follows. */
/* write data for one quicktime track */
/* the user must handle conversion to the channels in this track */
int quicktime_write_audio(quicktime_t *file, char *audio_buffer, long samples, int track);
int quicktime_write_audio_frame(quicktime_t *file, unsigned char *audio_buffer, long bytes, int track);
int quicktime_write_audio_hint(quicktime_t *file, unsigned char *hintBuffer, 
	long hintBufferSize, int audioTrack, int hintTrack, long hintSampleDuration);

int quicktime_write_video_frame(quicktime_t *file, unsigned char *video_buffer, long bytes, int track, u_char isKeyFrame, long duration, long renderingOffset);
int quicktime_write_video_hint(quicktime_t *file, unsigned char *hint_buffer, long hintBufferSize, int videoTrack, int hintTrack, long hintSampleDuration, u_char isKeyFrame);

/* for writing a frame using a library that needs a file descriptor */
int quicktime_write_frame_init(quicktime_t *file, int track); /* call before fwrite */
FILE* quicktime_get_fd(quicktime_t *file);     /* return a file descriptor */
int quicktime_write_frame_end(quicktime_t *file, int track); /* call after fwrite */

/* For reading and writing audio to a file descriptor. */
int quicktime_write_audio_end(quicktime_t *file, int track, long samples); /* call after fwrite */

/* Read an entire chunk. */
/* read the number of bytes starting at the byte_start in the specified chunk */
/* You must provide enough space to store the chunk. */
int quicktime_read_chunk(quicktime_t *file, char *output, int track, long chunk, long byte_start, long byte_len);

/* read raw data */
long quicktime_read_audio(quicktime_t *file, char *audio_buffer, long samples, int track);
long quicktime_read_audio_frame(quicktime_t *file, unsigned char *audio_buffer, int maxBytes, int track);
long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track);

/* for reading frame using a library that needs a file descriptor */
/* Frame caching doesn't work here. */
int quicktime_read_frame_init(quicktime_t *file, int track);
int quicktime_read_frame_end(quicktime_t *file, int track);

/* for RTP hints */
void quicktime_init_hint_sample(u_char* hintBuf, u_int* hintBufSize);
void quicktime_add_hint_packet(u_char* hintBuf, u_int* hintBufSize,
					u_int payloadNumber, u_int rtpSeqNum);
void quicktime_add_hint_immed_data(u_char* hintBuf, u_int* hintBufSize, 
					u_char* data, u_int length);
void quicktime_add_hint_sample_data(u_char* hintBuf, u_int* hintBufSize, 
					u_int fromSampleNum, u_int offset, u_int length);

void quicktime_set_hint_timestamp_offset(u_char* hintBuf, u_int* hintBufSize, int offset);
void quicktime_set_hint_Mbit(u_char* hintBuf);
void quicktime_set_hint_Bframe(u_char* hintBuf);
void quicktime_set_hint_repeat(u_char* hintBuf);
void quicktime_set_hint_repeat_offset(u_char* hintBuf, u_int32_t offset);

int quicktime_dump_hint_sample(u_char* hintBuf);
int quicktime_dump_hint_packet(u_char* hintBuf);
int quicktime_dump_hint_data(u_char* hintBuf);

int quicktime_get_hint_size(u_char* hintBuf);
int quicktime_get_hint_info(u_char* hintBuf, u_int hintBufSize, quicktime_hint_info_t* pHintInfo);

/* ===================== Access to built in codecs follows. */

/* If the codec for this track is supported in the library return 1. */
int quicktime_supported_video(quicktime_t *file, int track);
int quicktime_supported_audio(quicktime_t *file, int track);

/* The codec can generate the color model */
int quicktime_test_colormodel(quicktime_t *file, 
		int colormodel, 
		int track);

/* Decode or encode the frame into a frame buffer. */
/* All the frame buffers passed to these functions are unsigned char */
/* rows with 3 bytes per pixel.  The byte order per 3 byte pixel is */
/* RGB. */
int quicktime_decode_video(quicktime_t *file, unsigned char **row_pointers, int track);
int quicktime_encode_video(quicktime_t *file, unsigned char **row_pointers, int track);
long quicktime_decode_scaled(quicktime_t *file, 
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h,
	int color_model,             /* One of the color models defined above */
	unsigned char **row_pointers, 
	int track);

/* Decode or encode audio for a single channel into the buffer. */
/* Pass a buffer for the _i or the _f argument if you want int16 or float data. */
/* Notice that encoding requires an array of pointers to each channel. */
int quicktime_decode_audio(quicktime_t *file, QUICKTIME_INT16 *output_i, float *output_f, long samples, int channel);
int quicktime_encode_audio(quicktime_t *file, QUICKTIME_INT16 **input_i, float **input_f, long samples);

/* Dump the file structures for the currently opened file. */
int quicktime_dump(quicktime_t *file);

/* Specify the number of cpus to utilize. */
int quicktime_set_cpus(quicktime_t *file, int cpus);

/* Specify whether to read contiguously or not. */
/* preload is the number of bytes to read ahead. */
int quicktime_set_preload(quicktime_t *file, long preload);

/* Test the 32 bit overflow */
int quicktime_test_position(quicktime_t *file);

#ifdef __cplusplus
}
#endif

#endif
