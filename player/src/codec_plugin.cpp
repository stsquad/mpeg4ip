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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * codec_plugin.cpp - read and process for plugins
 */

#ifndef _WIN32
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>
#endif

#include "codec_plugin_private.h"
#include "player_util.h"
#include "our_config_file.h"
#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "audio.h"
#include "video.h"

#include "rtp_plugin.h"
/*
 * Portability hacks
 */
#ifdef _WIN32
#define LIBRARY_HANDLE    HINSTANCE
#define DLL_GET_SYM       GetProcAddress
#define DLL_CLOSE         FreeLibrary
#define DLL_ERROR         GetLastError
#define PLAYER_PLUGIN_DIR "."
typedef struct dir_list_t {
  WIN32_FIND_DATA dptr;
  HANDLE shandle;
} dir_list_t;

#else
#define LIBRARY_HANDLE    void *
#define DLL_GET_SYM       dlsym
#define DLL_CLOSE         dlclose
#define DLL_ERROR         dlerror
// PLAYER_PLUGIN_DIR is set via configure.in
typedef struct dir_list_t {
  DIR *dptr;
  char fname[PATH_MAX];
} dir_list_t;
#endif

/*
 * codec_plugin_list_t is how we store the codec links
 */
typedef struct codec_plugin_list_t {
  struct codec_plugin_list_t *next_codec;
  LIBRARY_HANDLE dl_handle;
  codec_plugin_t *codec;
} codec_plugin_list_t;

static codec_plugin_list_t *audio_codecs, *video_codecs;

typedef struct rtp_plugin_list_t {
  struct rtp_plugin_list_t *next_rtp_plugin;
  LIBRARY_HANDLE dl_handle;
  rtp_plugin_t *rtp_plugin;
} rtp_plugin_list_t;

static rtp_plugin_list_t *rtp_plugins;

static void close_file_search (dir_list_t *ptr)
{
#ifndef _WIN32
  closedir(ptr->dptr);
#endif
}
/*
 * portable way to find the next file.  In unix, we're looking for
 * any .so files.  In windows, we've already started looking for .dll
 */
static const char *find_next_file (dir_list_t *ptr,
				   const char *name)
{
#ifndef _WIN32
  struct dirent *fptr;

  while ((fptr = readdir(ptr->dptr)) != NULL) {
    int len = strlen(fptr->d_name);
    if (len > 3 && strcmp(fptr->d_name + len - 3, ".so") == 0) {
      sprintf(ptr->fname, "%s/%s", 
	      name, fptr->d_name);
      return (ptr->fname);
    }
  }
  return NULL;
#else
  if (FindNextFile(ptr->shandle, &ptr->dptr) == FALSE)
    return NULL;
  return ptr->dptr.cFileName;
#endif
}

/*
 * get_first_file
 * start directory search for plugin files
 */
static const char *get_first_file (dir_list_t *ptr, 
				   const char *name)
{
#ifndef _WIN32
  ptr->dptr = opendir(name);
  if (ptr->dptr == NULL) 
    return NULL;
  return (find_next_file(ptr, name));
#else
  ptr->shandle = FindFirstFile("*.dll", &ptr->dptr);
  if (ptr->shandle == INVALID_HANDLE_VALUE) return NULL;
  return ptr->dptr.cFileName;
#endif
}

/*
 * initialize_plugins
 * Start search for plugins - classify them as audio or video
 */
void initialize_plugins (void)
{
  LIBRARY_HANDLE handle;
  codec_plugin_t *cptr;
  rtp_plugin_t *rptr;
  dir_list_t dir;
  const char *fname;

  audio_codecs = video_codecs = NULL;
  rtp_plugins = NULL;
  fname = get_first_file(&dir, PLAYER_PLUGIN_DIR);

  while (fname != NULL) {
#ifdef _WIN32
    handle = LoadLibrary(fname);
#else
    handle = dlopen(fname, RTLD_LAZY);
#endif
    if (handle != NULL) {
      cptr = (codec_plugin_t *)DLL_GET_SYM(handle, "mpeg4ip_codec_plugin");
      if (cptr != NULL) {
	if (strcmp(cptr->c_version, PLUGIN_VERSION) == 0) {
	  codec_plugin_list_t *p;
	  p = (codec_plugin_list_t *)malloc(sizeof(codec_plugin_list_t));
	  if (p == NULL) exit(-1);
			  
	  p->codec = cptr;
	  p->dl_handle = handle;
	  p->next_codec = NULL;
			  
	  if (strcmp(cptr->c_type, "audio") == 0) {
	    p->next_codec = audio_codecs;
	    audio_codecs = p;
	    message(LOG_INFO, "plugin", "Adding audio plugin %s %s", 
		    cptr->c_name, fname);
	  } else if (strcmp(cptr->c_type, "video") == 0) {
	    p->next_codec = video_codecs;
	    video_codecs = p;
	    message(LOG_INFO, "plugin", "Adding video plugin %s %s", 
		    cptr->c_name, fname);
	  } else {
	    free(p);
	    message(LOG_CRIT, "plugin", "Unknown plugin type %s in plugin %s", 
		    cptr->c_type, fname);
	  }
	} else {
	  message(LOG_ERR, "plugin", "Plugin %s(%s) has wrong version %s", 
		  fname, cptr->c_type, cptr->c_version);
	}
      }
      rptr = (rtp_plugin_t *)DLL_GET_SYM(handle, RTP_PLUGIN_EXPORT_NAME_STR);
      if (rptr != NULL) {
	if (strcmp(rptr->version, RTP_PLUGIN_VERSION) == 0) {
	  rtp_plugin_list_t *p;
	  p = MALLOC_STRUCTURE(rtp_plugin_list_t);
	  if (p == NULL) exit(-1);
			  
	  p->rtp_plugin = rptr;
	  p->dl_handle = handle;
	  p->next_rtp_plugin = rtp_plugins;
	  rtp_plugins = p;
	  message(LOG_INFO, "plugin", "Adding RTP plugin %s %s", 
		  rptr->name, fname);
	} else {
	  message(LOG_CRIT, "plugin", "Plugin %s (%s) has wrong version %s", 
		  rptr->name, fname, rptr->version);
	}
      }
      if (rptr == NULL && cptr == NULL) {
	message(LOG_ERR, "plugin", "Can't find export point in plugin %s", 
		fname);
      }
    } else {
      message(LOG_ERR, "plugin", "Can't dlopen plugin %s - %s", fname,
	      DLL_ERROR());
    }
    fname = find_next_file(&dir, PLAYER_PLUGIN_DIR);
  }
  close_file_search(&dir);
}

/*
 * check_for_audio_codec
 * search the list of audio codecs for one that matches the parameters
 */
codec_plugin_t *check_for_audio_codec (const char *compressor,
				       format_list_t *fptr,
				       int audio_type,
				       int profile, 
				       const uint8_t *userdata,
				       uint32_t userdata_size)
{
  codec_plugin_list_t *aptr;
  int best_value = 0;
  codec_plugin_t *ret;

  ret = NULL;
  aptr = audio_codecs;
  while (aptr != NULL) {
    if (aptr->codec->c_compress_check != NULL) {
      int temp;
      temp = (aptr->codec->c_compress_check)(message,
					     compressor,
					     audio_type,
					     profile, 
					     fptr,
					     userdata,
					     userdata_size);
      if (temp > best_value) {
	best_value = temp;
	ret = aptr->codec;
      }
    }
    aptr = aptr->next_codec;
  }
  if (ret != NULL) {
    message(LOG_DEBUG, "plugin", 
	    "Found matching audio plugin %s", ret->c_name);
  }
  return (ret);
}

/*
 * check_for_video_codec
 * search the list of video plugins  for one that matches the parameters
 */
codec_plugin_t *check_for_video_codec (const char *compressor,
				       format_list_t *fptr,
				       int type,
				       int profile, 
				       const uint8_t *userdata,
				       uint32_t userdata_size)
{
  codec_plugin_list_t *vptr;
  int best_value = 0;
  codec_plugin_t *ret;

  ret = NULL;
  vptr = video_codecs;
  while (vptr != NULL) {
    if (vptr->codec->c_compress_check != NULL) {
      int temp;
      temp = (vptr->codec->c_compress_check)(message,
					     compressor,
					     type,
					     profile, 
					     fptr,
					     userdata,
					     userdata_size);
      if (temp > 0 &&
	  config.get_config_value(CONFIG_USE_MPEG4_ISO_ONLY) &&
	  strcmp(vptr->codec->c_name, "MPEG4 ISO") == 0) {
	temp = INT_MAX;
      }

      if (temp > best_value) {
	best_value = temp;
	ret = vptr->codec;
      }
    }
    vptr = vptr->next_codec;
  }
  if (ret != NULL) {
    message(LOG_DEBUG, "plugin", 
	    "Found matching video plugin %s", ret->c_name);
  }
  return (ret);
}

rtp_check_return_t check_for_rtp_plugins (format_list_t *fptr,
					  uint8_t rtp_payload_type,
					  rtp_plugin_t **rtp_plugin)
{
  rtp_plugin_list_t *r;
  rtp_plugin_t *rptr;
  rtp_check_return_t ret;

  *rtp_plugin = NULL;
  r = rtp_plugins;

  while (r != NULL) {
    rptr = r->rtp_plugin;

    if (rptr->rtp_plugin_check != NULL) {
      ret = (rptr->rtp_plugin_check)(message, fptr, rtp_payload_type);
      if (ret != RTP_PLUGIN_NO_MATCH) {
	*rtp_plugin = rptr;
	return ret;
      }
    }
    r = r->next_rtp_plugin;
  }

  return RTP_PLUGIN_NO_MATCH;
}

/*
 * video_codec_check_for_raw_file
 * goes through list of video codecs to see if "name" has a raw file
 * match
 */
int video_codec_check_for_raw_file (CPlayerSession *psptr,
				    const char *name)
{
  codec_plugin_list_t *vptr;
  codec_data_t *cifptr;
  char *desc[4];
  double maxtime;
  int slen = strlen(name);

  desc[0] = NULL;
  desc[1] = NULL;
  desc[2] = NULL;
  desc[3] = NULL;

  vptr = video_codecs;
  
  while (vptr != NULL) {
    if (vptr->codec->c_raw_file_check != NULL) {
      if (config.get_config_value(CONFIG_USE_MPEG4_ISO_ONLY) != 0 &&
	  strcmp("MPEG4 ISO", vptr->codec->c_name) != 0 && 
	  strcmp(".divx", name + slen - 5) == 0) {
	vptr = vptr->next_codec;
	continue;
      }
							   
      cifptr = vptr->codec->c_raw_file_check(message,
					     name,
					     &maxtime,
					     desc);
      if (cifptr != NULL) {
	player_debug_message("Found raw file codec %s", vptr->codec->c_name);
	CPlayerMedia *mptr;
   
	/*
	 * Create the player media, and the bytestream
	 */
	mptr = new CPlayerMedia(psptr);

	COurInByteStreamFile *fbyte;
	fbyte = new COurInByteStreamFile(vptr->codec,
					 cifptr,
					 maxtime);
	mptr->create(fbyte, TRUE);
	mptr->set_plugin_data(vptr->codec, cifptr, get_video_vft(), NULL);

	for (int ix = 0; ix < 4; ix++) 
	  if (desc[ix] != NULL) 
	    psptr->set_session_desc(ix, desc[ix]);

	if (maxtime != 0.0) {
	  psptr->session_set_seekable(1);
	}

	return 0;
      }
    } 
    vptr = vptr->next_codec;
  }
  return -1;
}

/*
 * audio_codec_check_for_raw_file
 * goes through the list of audio codecs, looking for raw file
 * support
 */
int audio_codec_check_for_raw_file (CPlayerSession *psptr,
				    const char *name)
{
  codec_plugin_list_t *aptr;
  codec_data_t *cifptr;
  char *desc[4];
  double maxtime;

  desc[0] = NULL;
  desc[1] = NULL;
  desc[2] = NULL;
  desc[3] = NULL;

  aptr = audio_codecs;
  while (aptr != NULL) {

    if (aptr->codec->c_raw_file_check != NULL) {
      cifptr = aptr->codec->c_raw_file_check(message,
					     name,
					     &maxtime,
					     desc);
      if (cifptr != NULL) {
	CPlayerMedia *mptr;
      
	mptr = new CPlayerMedia(psptr);

	COurInByteStreamFile *fbyte;
	fbyte = new COurInByteStreamFile(aptr->codec,
					 cifptr,
					 maxtime);
	mptr->create(fbyte, FALSE);
	mptr->set_plugin_data(aptr->codec, cifptr, NULL, get_audio_vft());

	for (int ix = 0; ix < 4; ix++) 
	  if (desc[ix] != NULL) 
	    psptr->set_session_desc(ix, desc[ix]);

	if (maxtime != 0.0) {
	  psptr->session_set_seekable(1);
	}

	return 0;
      }
    }
					   
    aptr = aptr->next_codec;
  }
  return -1;
}

/*
 * close_plugins
 * closes and frees plugin lists
 */
void close_plugins (void) 
{
  codec_plugin_list_t *p;
  rtp_plugin_list_t *r;
  while (audio_codecs != NULL) {
    p = audio_codecs;
    DLL_CLOSE(p->dl_handle);
    audio_codecs = audio_codecs->next_codec;
    free(p);
  }
  while (video_codecs != NULL) {
    p = video_codecs;
    DLL_CLOSE(p->dl_handle);
    video_codecs = video_codecs->next_codec;
    free(p);
  }
  while (rtp_plugins != NULL) {
    r = rtp_plugins;
    DLL_CLOSE(r->dl_handle);
    rtp_plugins = r->next_rtp_plugin;
    free(r);
  }
}

/* end file codec_plugin.cpp */
