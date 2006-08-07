/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2002-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * codec_plugin.h - audio/video plugin definitions for player
 */
#ifndef __CODEC_PLUGIN_H__
#define __CODEC_PLUGIN_H__ 1

#include <mpeg4ip_config_set.h>
#include <sdp.h>
/*
 * When you change the plugin version, you should add a "HAVE_PLUGIN_VERSION"
 * for easier makes
 */
#define PLUGIN_VERSION "1.1"
#define HAVE_PLUGIN_VERSION_0_8 1
#define HAVE_PLUGIN_VERSION_0_9 1
#define HAVE_PLUGIN_VERSION_0_A 1
#define HAVE_PLUGIN_VERSION_0_B 1
#define HAVE_PLUGIN_VERSION_1_0 1
// version 1.1 for sdp redos
#define HAVE_PLUGIN_VERSION_1_1 1

/*
 * frame_timestamp_t structure is the method that the bytestreams will
 * pass timestamps to the codecs.
 * msec_timestamp is the timestamp in milliseconds
 * audio_freq_timestamp is the timestamp in the specified audio_freq
 * audio_freq is the timescale used for audio_freq_timestamp
 * timestamp_is_pts is used to indicate if a timestamp is a presentation or
 *    decode timestamp.  The plugin must translate a presentation timestamp
 *    (from say a B frame) to the actual timestamp.  Most file bytestreams
 *    use a decode timestamp.
 */
typedef struct frame_timestamp_t {
  uint64_t msec_timestamp;
  uint32_t audio_freq_timestamp;
  uint32_t audio_freq;
  bool timestamp_is_pts;
} frame_timestamp_t;

/***************************************************************************
 *  Audio callbacks from plugin to renderer
 ***************************************************************************/
/*
 * audio_configure_f - audio configuration - called when initializing
 * audio output.
 * Inputs:
 *   ifptr - handle passed when created
 *   freq - frequency in samples per second
 *   chans - number of channels
 *   format - audio format definitions from lib/SDL/include/SDL_audio.h
 *   max_samples - number of samples required after processing each frame
 *     Use a 0 for unknown or variable size.
 *     variable size must use audio_load_buffer interface
 * Outputs:
 *   nothing
 */

typedef void (*audio_configure_f)(void *ifptr,
				  int freq,
				  int chans,
				  audio_format_t format,
				  uint32_t max_samples);

/*
 * audio_get_buffer_f - get an audio ring buffer to fill
 *  called before decoding a frame
 * Inputs: ifptr - pointer to handle
 *    freq_ts - timestamp in samples (at the audio frequency) that
 *         corresponds with ts.  This lets us easily check if the samples
 *         are consecutive without converting to msec and back again.
 *    ts - timestamp of audio packet
 * Outputs: unsigned char pointer to buffer to write to.
 */
typedef uint8_t *(*audio_get_buffer_f)(void *ifptr,
				       uint32_t freq_ts,
				       uint64_t ts);
/*
 * audio_filled_buffer_f - routine to call after decoding
 *  audio frame into a buffer gotten above.
 * Inputs:
 *    ifptr - pointer to handle
 */
typedef void (*audio_filled_buffer_f)(void *ifptr);

/*
 * audio_load_buffer_f - load local audio buffer with a variable number of
 * bytes
 * Inputs:
 *    ifptr - pointer to handle
 *    from - pointer to from buffer
 *    bytes - number of bytes (not samples) in buffer
 *    freq_ts - timestamp in samples (at the audio frequency) that
 *         corresponds with ts.  This lets us easily check if the samples
 *         are consecutive without converting to msec and back again.
 *    ts - timestamp of start of buffer
 */
typedef void (*audio_load_buffer_f)(void *ifptr,
				    const uint8_t *from,
				    uint32_t bytes,
				    uint32_t freq_ts,
				    uint64_t ts);
/*
 * audio_vft_t - virtual function table for audio events
 */
typedef struct audio_vft_t {
  lib_message_func_t log_msg;
  audio_configure_f audio_configure;
  audio_get_buffer_f audio_get_buffer;
  audio_filled_buffer_f audio_filled_buffer;
  audio_load_buffer_f audio_load_buffer;
  CConfigSet *pConfig;
} audio_vft_t;

/*****************************************************************************
 * Video callbacks from plugin to renderer
 *****************************************************************************/
#define VIDEO_FORMAT_YUV 1

/*
 * video_configure_f - configure video sizes
 * Inputs: ifptr - pointer to handle passed
 *         w - width in pixels
 *         h - height in pixels
 *         format - right now, only VIDEO_FORMAT_YUV
 *         aspect_ratio - 0.0 for default, set for value (value will
 *           adjust so display_w = h * aspect_ratio;
 * Outputs: none
 */
typedef void (*video_configure_f)(void *ifptr,
				  int w,
				  int h,
				  int format,
				  double aspect_ratio);
/*
 * video_get_buffer_f - request y, u and v buffers before decoding
 * Inputs: ifptr - handle
 * Outputs: y - pointer to y buffer
 *          u - pointer to u buffer
 *          v - pointer to v buffer
 * return value: 0 - no buffer
 *               1 - valid buffer
 * Note: will wait for return until buffer ready
 */
typedef int (*video_get_buffer_f)(void *ifptr,
				  uint8_t **y,
				  uint8_t **u,
				  uint8_t **v);
/*
 * video_filled_buffer_f - indicates we've filled buffer gotten above
 * Inputs - ifptr - handle
 *          display_time - rendering time in msec.
 */
typedef void (*video_filled_buffer_f)(void *ifptr,
				      uint64_t display_time);
/*
 * video_have_frame_f - instead of using video_get_buffer and
 *   video_filled_buffer, can use this instead if buffer is stored locally
 * Inputs: ifptr - handle
 *         y - pointer to y data
 *         u - pointer to u data
 *         v - pointer to v data
 *         m_pixelw_y - width of each row in y above (might not be width)
 *         m_pixelw_uv - width of each row in u and v
 *         display_time - render time in msec
 */
typedef void (*video_have_frame_f)(void *ifptr,
				   const uint8_t *y,
				   const uint8_t *u,
				   const uint8_t *v,
				   int m_pixelw_y,
				   int m_pixelw_uv,
				   uint64_t display_time);

/*
 * video_vft_t - video virtual function table
 */
typedef struct video_vft_t {
  lib_message_func_t log_msg;
  video_configure_f video_configure;
  video_get_buffer_f video_get_buffer;
  video_filled_buffer_f video_filled_buffer;
  video_have_frame_f video_have_frame;
  CConfigSet *pConfig;
} video_vft_t;

/*************************************************************************
 * Text callbacks
 *************************************************************************/
typedef void (*text_configure_f)(void *ifptr, uint32_t display_type, 
				   void *display_configuration);

typedef void (*text_have_frame_f)(void *ifptr, 
				  uint64_t display_time,
				  uint32_t display_type,
				  void *display_structure);
typedef struct text_vft_t {
  lib_message_func_t log_msg;
  text_configure_f text_configure;
  text_have_frame_f text_have_frame;
  CConfigSet *pConfig;
} text_vft_t;
/**************************************************************************
 *  Routines plugin must provide
 **************************************************************************/
typedef struct video_info_t {
  int height;
  int width;
} video_info_t;

typedef struct audio_info_t {
  int freq;
  int chans;
  int bitspersample;
} audio_info_t;

/*
 * The codec data returned must start with this structure
 */
typedef struct codec_data_t {
  void *ifptr;
  union {
    video_vft_t *video_vft;
    audio_vft_t *audio_vft;
    text_vft_t *text_vft;
  } v;
} codec_data_t;

/*
 * These are the values passed for the stream types
 */
#define STREAM_TYPE_RTP "RTP"
#define STREAM_TYPE_MPEG2_TRANSPORT_STREAM "MPEG2 TRANSPORT"
#define STREAM_TYPE_AVI_FILE "AVI FILE"
#define STREAM_TYPE_MPEG_FILE "MPEG FILE"
#define STREAM_TYPE_MP4_FILE "MP4 FILE"
#define STREAM_TYPE_QT_FILE "QT FILE"
/*
 * ac_create_f - audio codec plugin creation routine
 * Inputs: 
 *         stream_type - stream type of file
 *         compressor - pointer to codec.  
 *         type - video type.  valid for .mp4 files
 *         profile - video profile level - valid for .mp4 files
 *         sdp_media - pointer to session description information for stream
 *         audio - pointer to audio information
 *         user_data - pointer to user data
 *         userdata_size - size of user data
 *         if_vft - pointer to audio vft to use
 *         ifptr - handle to use for audio callbacks
 * Returns - must return a handle that contains codec_data_t.
 */
typedef codec_data_t *(*ac_create_f)(const char *stream_type,
				     const char *compressor, 
				     int type, 
				     int profile, 
				     format_list_t *sdp_media,
				     audio_info_t *audio,
				     const uint8_t *user_data,
				     uint32_t userdata_size,
				     audio_vft_t *if_vft,
				     void *ifptr);

/*
 * tc_create_f - text codec plugin creation routine
 * Inputs: 
 *         stream_type - stream type of file
 *         compressor - pointer to codec.  
 *         sdp_media - pointer to session description information for stream
 *         user_data - pointer to user data
 *         userdata_size - size of user data
 *         if_vft - pointer to video vft to use
 *         ifptr - handle to use for video callbacks
 * Returns - must return a handle that contains codec_data_t.
 */
typedef codec_data_t *(*tc_create_f)(const char *stream_type,
				     const char *compressor, 
				     format_list_t *sdp_media,
				     const uint8_t *user_data,
				     uint32_t userdata_size,
				     text_vft_t *if_vft,
				     void *ifptr);
/*
 * vc_create_f - video codec plugin creation routine
 * Inputs: 
 *         stream_type - stream type of file
 *         compressor - pointer to codec.  
 *         type - video type.  valid for .mp4 files
 *         profile - video profile level - valid for .mp4 files
 *         sdp_media - pointer to session description information for stream
 *         video - pointer to video information
 *         user_data - pointer to user data
 *         userdata_size - size of user data
 *         if_vft - pointer to video vft to use
 *         ifptr - handle to use for video callbacks
 * Returns - must return a handle that contains codec_data_t.
 */
typedef codec_data_t *(*vc_create_f)(const char *stream_type,
				     const char *compressor, 
				     int type, 
				     int profile, 
				     format_list_t *sdp_media,
				     video_info_t *video,
				     const uint8_t *user_data,
				     uint32_t userdata_size,
				     video_vft_t *if_vft,
				     void *ifptr);

/*
 * c_close_f - close plugin - free all data, including ptr
 */
typedef void (*c_close_f)(codec_data_t *ptr);
/*
 * c_do_pause_f - called when a pause has taken place.  Next data may not
 * match previous - skip may occur
 */
typedef void (*c_do_pause_f)(codec_data_t *ptr);

/*
 * c_decode_frame_f - ah, the money callback.  decode the frame
 * Inputs: ptr - pointer to codec handle
 *         ts - pointer to frame_timestamp_t for this frame
 *         from_rtp - if it's from RTP - may not be needed
 *         buffer - pointer to frame to decode (can be guaranteed that there
 *           is a complete frame - maybe more than 1
 *         buflen - length of buffer
 * Outputs:
 *         sync_frame - 1 if a special frame (for example, an I frame for
 *	              video)
 * Returns:
 *        -1 - couldn't decode in whole frame
 *       <1-buflen> - number of bytes decoded
 */
typedef int (*c_decode_frame_f)(codec_data_t *ptr,
				frame_timestamp_t *ts,
				int from_rtp,
				int *sync_frame,
				uint8_t *buffer,
				uint32_t buflen, 
				void *userdata);

typedef int (*c_video_frame_is_sync_f)(codec_data_t *ptr,
				       uint8_t *buffer,
				       uint32_t buflen,
				       void *userdata);

typedef int (*c_print_status_f)(codec_data_t *ptr, 
				char *buffer, 
				uint32_t buflen);
/*
 * c_compress_check_f - see if a plugin can decode the bit stream
 *  note - create function from above must be called afterwards
 * Inputs - msg - can use for debug messages
 *   stream_type - see above.
 *   compressor - pointer to codec.  
 *   type - video type.  valid for .mp4 files
 *   profile - video profile level - valid for .mp4 files
 *   fptr - pointer to sdp data
 *   userdata - pointer to user data to check out - might have VOL header,
 *     for example
 *   userdata_size - size of userdata in bytes
 * Return Value - -1 for not handled.
 *                number - weighted value of how well decoding can do.
 */
typedef int (*c_compress_check_f)(lib_message_func_t msg,
				  const char *stream_type,
				  const char *compressor,
				  int type,
				  int profile,
				  format_list_t *fptr,
				  const uint8_t *userdata,
				  uint32_t userdata_size,
				  CConfigSet *pConfig);

/*
 * c_raw_file_check_f - see if this codec can handle raw files
 * Note - this could be designed a bit better - like a 2 stage check
 *   and create.
 * Inputs: msg - for debug messags
 *         filename - name of file (duh)
 * Outputs - max_time 0.0 if not seekable, otherwise time
 *           desc[4] - 4 slots for descriptions
 */
typedef codec_data_t *(*c_raw_file_check_f)(lib_message_func_t msg,
					    const char *filename,
					    double *max_time,
					    char *desc[4],
					    CConfigSet *pConfig);

/*
 * c_raw_file_next_frame_f - get a data buffer with a full frame of data
 * Inputs: your_data - handle
 * Outputs: buffer - pointer to buffer
 *          ts - pointer to frame_timestamp_t structure to fill in.
 * Return value - number of bytes (0 for no frame)
 */
typedef int (*c_raw_file_next_frame_f)(codec_data_t *your_data,
				       uint8_t **buffer,
				       frame_timestamp_t *ts);

/*
 * c_raw_file_used_for_frame_f - indicates number of bytes decoded
 * by decoder
 */
typedef void (*c_raw_file_used_for_frame_f)(codec_data_t *your_data,
					    uint32_t bytes);

/*
 * c_raw_file_seek_to_f - seek to ts in msec
 */
typedef int (*c_raw_file_seek_to_f)(codec_data_t *your_data,
				    uint64_t ts);

/*
 * c_raw_file_skip_frame_f - indicates that we should skip the next frame
 * used for video raw frames only
 */
typedef int (*c_raw_file_skip_frame_f)(codec_data_t *ptr);

/*
 * c_raw_file_has_eof_f - return indication of end of file reached
 */
typedef int (*c_raw_file_has_eof_f)(codec_data_t *ptr);

typedef struct codec_plugin_t {
  const char *c_name;
  const char *c_type;
  const char *c_version;
  ac_create_f  ac_create;
  vc_create_f  vc_create;
  // add vc_create_f here and below
  c_do_pause_f            c_do_pause;
  c_decode_frame_f        c_decode_frame;
  c_close_f               c_close;
  c_compress_check_f      c_compress_check;
  c_raw_file_check_f      c_raw_file_check;
  c_raw_file_next_frame_f c_raw_file_next_frame;
  c_raw_file_used_for_frame_f c_raw_file_used_for_frame;
  c_raw_file_seek_to_f    c_raw_file_seek_to;
  c_raw_file_skip_frame_f c_skip_frame;
  c_raw_file_has_eof_f    c_raw_file_has_eof;
  c_video_frame_is_sync_f c_video_frame_is_sync;
  c_print_status_f        c_print_status;
  SConfigVariable         *c_variable_list;
  uint32_t                c_variable_list_count;
  tc_create_f   tc_create;
} codec_plugin_t;

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif


/*
 * Use this for an audio plugin without raw file support
 */
#define AUDIO_CODEC_PLUGIN(name, \
                           create, \
			   do_pause, \
                           decode, \
                           print_status, \
			   close,\
			   compress_check, \
			   config_variables, \
			   config_variables_count) \
extern "C" { codec_plugin_t DLL_EXPORT mpeg4ip_codec_plugin = { \
   name, \
   "audio", \
   PLUGIN_VERSION, \
   create, \
   NULL, \
   do_pause, \
   decode, \
   close,\
   compress_check, \
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, print_status, \
   config_variables, config_variables_count, NULL,}; }

/*
 * Use this for audio plugin that has raw file support
 */
#define AUDIO_CODEC_WITH_RAW_FILE_PLUGIN(name, \
                                         create, \
			                 do_pause, \
                                         decode, \
                                         print_status, \
			                 close,\
			                 compress_check, \
			                 raw_file_check, \
			                 raw_file_next_frame, \
                                         raw_file_used_for_frame, \
			                 raw_file_seek_to, \
			                 raw_file_has_eof, \
					 config_variables, \
                                         config_variables_count)\
extern "C" { codec_plugin_t DLL_EXPORT mpeg4ip_codec_plugin = { \
   name, \
   "audio", \
   PLUGIN_VERSION, \
   create, \
   NULL, \
   do_pause, \
   decode, \
   close,\
   compress_check, \
   raw_file_check, \
   raw_file_next_frame, \
   raw_file_used_for_frame, \
   raw_file_seek_to, \
   NULL, \
   raw_file_has_eof, NULL, print_status, \
   config_variables, config_variables_count, NULL, }; }

/*
 * Use this for text plugins
 */
#define TEXT_CODEC_PLUGIN(name, \
                           create, \
			   do_pause, \
                           decode, \
                           print_status, \
			   close,\
			   compress_check, \
			   config_variables, \
			   config_variables_count) \
extern "C" { codec_plugin_t DLL_EXPORT mpeg4ip_codec_plugin = { \
   name, \
   "text", \
   PLUGIN_VERSION, \
   NULL, \
   NULL, \
   do_pause, \
   decode, \
   close,\
   compress_check, \
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, print_status, \
   config_variables, config_variables_count, create,}; }
/*
 * Use this for a video codec without raw file support
 */
#define VIDEO_CODEC_PLUGIN(name, \
                           create, \
			   do_pause, \
                           decode, \
                           print_status, \
			   close,\
			   compress_check,\
			   video_frame_is_sync, \
                           config_variables, \
                           config_variables_count ) \
extern "C" { codec_plugin_t DLL_EXPORT mpeg4ip_codec_plugin = { \
   name, \
   "video", \
   PLUGIN_VERSION, \
   NULL, \
   create, \
   do_pause, \
   decode, \
   close,\
   compress_check, \
   NULL, NULL, NULL, NULL, NULL, NULL, video_frame_is_sync, print_status, \
   config_variables, config_variables_count, NULL,}; }

/*
 * Use this for video codec with raw file support
 */
#define VIDEO_CODEC_WITH_RAW_FILE_PLUGIN(name, \
                                         create, \
			                 do_pause, \
                                         decode, \
                                         print_status, \
			                 close,\
			                 compress_check, \
                                         video_frame_is_sync, \
			                 raw_file_check, \
			                 raw_file_next_frame, \
                                         raw_file_used_for_frame, \
			                 raw_file_seek_to, \
			                 raw_file_skip_frame, \
                                         raw_file_has_eof, \
                                         config_variables, \
					 config_variables_count) \
extern "C" { codec_plugin_t DLL_EXPORT mpeg4ip_codec_plugin = { \
   name, \
   "video", \
   PLUGIN_VERSION, \
   NULL, \
   create, \
   do_pause, \
   decode, \
   close,\
   compress_check, \
   raw_file_check, \
   raw_file_next_frame, \
   raw_file_used_for_frame, \
   raw_file_seek_to, \
   raw_file_skip_frame,\
   raw_file_has_eof, \
   video_frame_is_sync, \
   print_status, \
   config_variables, \
   config_variables_count, NULL,\
}; }
     
     
#endif
